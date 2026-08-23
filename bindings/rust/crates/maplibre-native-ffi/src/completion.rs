use std::any::TypeId;
use std::ffi::c_void;
use std::future::Future;
use std::pin::Pin;
use std::sync::{Arc, Condvar, Mutex};
use std::task::{Context, Poll, Waker};
use std::time::Duration;

use maplibre_native_ffi_core as core;
use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};

/// Metadata reported when an ordered command commits.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CommandCompletion {
    pub disposition: crate::CommandDisposition,
    pub generation: u64,
    pub raw_status: i32,
    pub diagnostic: String,
}

struct State<T> {
    result: Mutex<Option<Result<T>>>,
    terminal: Mutex<Option<(i32, String)>>,
    retained: Mutex<Option<Arc<dyn std::any::Any + Send + Sync>>>,
    ready: Condvar,
    waker: Mutex<Option<Waker>>,
}

type Converter<T> = Box<dyn FnOnce(&sys::mln_completion_result) -> Result<T> + Send>;

struct Bridge<T> {
    state: Arc<State<T>>,
    convert: Mutex<Option<Converter<T>>>,
    accept_error_status: bool,
}

/// A one-shot native completion exposed as a Rust future.
pub struct NativeFuture<T> {
    state: Arc<State<T>>,
}

impl<T> std::fmt::Debug for NativeFuture<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("NativeFuture")
            .field("ready", &self.is_ready())
            .finish()
    }
}

impl<T> NativeFuture<T> {
    pub(crate) fn retain<U>(&self, value: Arc<U>)
    where
        U: Send + Sync + 'static,
    {
        let terminal = self
            .state
            .terminal
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if terminal.is_none() {
            *self
                .state
                .retained
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner()) = Some(value);
        }
    }

    /// Reports whether native has delivered the one terminal result.
    pub fn is_ready(&self) -> bool {
        self.state
            .result
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .is_some()
    }

    /// Blocks for at most `timeout`, for hosts that must pump another service
    /// loop while waiting rather than run an async executor.
    pub fn wait(&self, timeout: Duration) -> Result<bool> {
        let result = self
            .state
            .result
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if result.is_some() {
            return Ok(true);
        }
        Ok(self
            .state
            .ready
            .wait_timeout_while(result, timeout, |result| result.is_none())
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .0
            .is_some())
    }

    /// Consumes a completed future without an async executor.
    pub fn take(&self) -> Result<T> {
        let mut result = self
            .state
            .result
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        result.take().ok_or_else(|| {
            Error::new(
                crate::ErrorKind::InvalidState,
                None,
                "native future has not completed",
            )
        })?
    }

    #[cfg(test)]
    pub(crate) fn is_completed(&self) -> Result<bool> {
        Ok(self.is_ready())
    }

    #[cfg(test)]
    pub(crate) fn terminal_status(&self) -> Result<sys::mln_status> {
        let terminal = self
            .state
            .terminal
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        match terminal.as_ref() {
            Some((status, _)) => Ok(*status),
            None => Err(Error::new(
                crate::ErrorKind::NotReady,
                None,
                "native future has not completed",
            )),
        }
    }

    #[cfg(test)]
    pub(crate) fn diagnostic(&self) -> Result<String> {
        let terminal = self
            .state
            .terminal
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        match terminal.as_ref() {
            Some((_, diagnostic)) => Ok(diagnostic.clone()),
            None => Err(Error::new(
                crate::ErrorKind::NotReady,
                None,
                "native future has not completed",
            )),
        }
    }

    #[cfg(test)]
    pub(crate) fn finish(&self) -> Result<T> {
        self.take()
    }

    #[cfg(test)]
    pub(crate) fn release(self) {}
}

#[cfg(test)]
pub(crate) fn blocking<T>(future: Result<NativeFuture<T>>) -> T {
    let future = future.expect("native submission failed");
    assert!(
        future
            .wait(Duration::from_secs(30))
            .expect("native wait failed")
    );
    future.take().expect("native completion failed")
}

impl<T> Future for NativeFuture<T> {
    type Output = Result<T>;

    fn poll(self: Pin<&mut Self>, context: &mut Context<'_>) -> Poll<Self::Output> {
        let mut result = self
            .state
            .result
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if let Some(result) = result.take() {
            return Poll::Ready(result);
        }
        *self
            .state
            .waker
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner()) = Some(context.waker().clone());
        Poll::Pending
    }
}

unsafe extern "C" fn complete<T>(
    user_data: *mut c_void,
    result: *const sys::mln_completion_result,
) {
    if user_data.is_null() || result.is_null() {
        return;
    }
    // SAFETY: native owns Bridge from successful submission through release.
    let bridge = unsafe { &*user_data.cast::<Bridge<T>>() };
    // SAFETY: completion result is borrowed for this callback.
    let raw = unsafe { &*result };
    *bridge
        .state
        .terminal
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner()) =
        Some((raw.status, copy_diagnostic(raw)));
    let converted = if raw.status == sys::MLN_STATUS_OK || bridge.accept_error_status {
        let convert = bridge
            .convert
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        match convert {
            Some(convert) => convert(raw),
            None => Err(Error::new(
                crate::ErrorKind::InvalidState,
                None,
                "native completion ran more than once",
            )),
        }
    } else {
        let diagnostic = if raw.diagnostic.data.is_null() || raw.diagnostic.size == 0 {
            String::new()
        } else {
            // SAFETY: diagnostic is borrowed and valid for this callback.
            String::from_utf8_lossy(unsafe {
                std::slice::from_raw_parts(raw.diagnostic.data.cast::<u8>(), raw.diagnostic.size)
            })
            .into_owned()
        };
        Err(Error::from_status_and_diagnostic(raw.status, diagnostic))
    };
    *bridge
        .state
        .result
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner()) = Some(converted);
    bridge.state.ready.notify_all();
    if let Some(waker) = bridge
        .state
        .waker
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
        .take()
    {
        waker.wake();
    }
}

unsafe extern "C" fn release<T>(user_data: *mut c_void) {
    if !user_data.is_null() {
        // SAFETY: native calls release once after accepted completion delivery.
        let bridge = unsafe { Box::from_raw(user_data.cast::<Bridge<T>>()) };
        bridge
            .state
            .retained
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        drop(bridge);
    }
}

pub(crate) fn submit<T, S, C>(submit: S, convert: C) -> Result<NativeFuture<T>>
where
    T: Send + 'static,
    S: FnOnce(*const sys::mln_completion) -> sys::mln_status,
    C: FnOnce(&sys::mln_completion_result) -> Result<T> + Send + 'static,
{
    let state = Arc::new(State {
        result: Mutex::new(None),
        terminal: Mutex::new(None),
        retained: Mutex::new(None),
        ready: Condvar::new(),
        waker: Mutex::new(None),
    });
    let bridge = Box::new(Bridge {
        state: Arc::clone(&state),
        convert: Mutex::new(Some(Box::new(convert))),
        accept_error_status: TypeId::of::<T>() == TypeId::of::<CommandCompletion>(),
    });
    let bridge = Box::into_raw(bridge);
    let completion = sys::mln_completion {
        size: std::mem::size_of::<sys::mln_completion>() as u32,
        callback: Some(complete::<T>),
        user_data: bridge.cast(),
        release_user_data: Some(release::<T>),
    };
    let status = submit(&completion);
    if status != sys::MLN_STATUS_OK {
        // SAFETY: rejected submissions invoke neither callback and retain no pointer.
        drop(unsafe { Box::from_raw(bridge) });
        return core::check(status).map(|()| unreachable!());
    }
    Ok(NativeFuture { state })
}

/// An already-complete future, for a call that finished without submitting.
pub(crate) fn ready<T>(value: T) -> NativeFuture<T> {
    NativeFuture {
        state: Arc::new(State {
            result: Mutex::new(Some(Ok(value))),
            terminal: Mutex::new(None),
            retained: Mutex::new(None),
            ready: Condvar::new(),
            waker: Mutex::new(None),
        }),
    }
}

pub(crate) fn unit(result: &sys::mln_completion_result) -> Result<()> {
    if !result.value.is_null() || result.value_count != 0 {
        return Err(Error::new(
            crate::ErrorKind::NativeError,
            None,
            "unit completion returned a value",
        ));
    }
    Ok(())
}

pub(crate) fn command(result: &sys::mln_completion_result) -> Result<CommandCompletion> {
    unit(result)?;
    Ok(CommandCompletion {
        disposition: crate::CommandDisposition::from_native(result.disposition),
        generation: result.generation,
        raw_status: result.status,
        diagnostic: copy_diagnostic(result),
    })
}

fn copy_diagnostic(result: &sys::mln_completion_result) -> String {
    if result.diagnostic.data.is_null() || result.diagnostic.size == 0 {
        return String::new();
    }
    // SAFETY: the diagnostic is borrowed for this callback and copied here.
    String::from_utf8_lossy(unsafe {
        std::slice::from_raw_parts(result.diagnostic.data.cast::<u8>(), result.diagnostic.size)
    })
    .into_owned()
}

pub(crate) fn copy_value<T: Copy>(result: &sys::mln_completion_result) -> Result<T> {
    if result.value.is_null() || result.value_count != 1 {
        return Err(Error::new(
            crate::ErrorKind::NativeError,
            None,
            "native completion returned no value",
        ));
    }
    // SAFETY: the submitting API defines value as one T for this callback.
    Ok(unsafe { result.value.cast::<T>().read_unaligned() })
}

pub(crate) fn copy_slice<T: Copy>(result: &sys::mln_completion_result) -> Result<Vec<T>> {
    if result.value_count == 0 {
        return Ok(Vec::new());
    }
    if result.value.is_null() {
        return Err(Error::new(
            crate::ErrorKind::NativeError,
            None,
            "native completion returned a null slice",
        ));
    }
    // SAFETY: the submitting API defines value_count T elements for this callback.
    Ok(
        unsafe {
            std::slice::from_raw_parts(result.value.cast::<T>(), result.value_count).to_vec()
        },
    )
}

pub(crate) fn optional_value<T: Copy>(result: &sys::mln_completion_result) -> Result<Option<T>> {
    if result.value_count == 0 {
        return Ok(None);
    }
    copy_value(result).map(Some)
}

pub(crate) fn buffer(result: &sys::mln_completion_result) -> Result<Vec<u8>> {
    let view = copy_value::<sys::mln_buffer_view>(result)?;
    // SAFETY: the view is borrowed for this completion callback.
    unsafe { core::string::copy_string_view_bytes(view) }
}

pub(crate) fn optional_buffer(result: &sys::mln_completion_result) -> Result<Option<Vec<u8>>> {
    match optional_value::<sys::mln_buffer_view>(result)? {
        Some(view) => {
            // SAFETY: the view is borrowed for this completion callback.
            unsafe { core::string::copy_string_view_bytes(view) }.map(Some)
        }
        None => Ok(None),
    }
}

pub(crate) fn string(result: &sys::mln_completion_result) -> Result<String> {
    let bytes = buffer(result)?;
    String::from_utf8(bytes).map_err(|error| {
        Error::new(
            crate::ErrorKind::NativeError,
            None,
            format!("native completion returned invalid UTF-8: {error}"),
        )
    })
}

#[cfg(test)]
mod tests {
    use std::sync::atomic::{AtomicBool, Ordering};

    use super::*;

    struct DropProbe(Arc<AtomicBool>);

    impl Drop for DropProbe {
        fn drop(&mut self) {
            self.0.store(true, Ordering::Release);
        }
    }

    #[test]
    fn native_bridge_retains_value_after_future_is_dropped() {
        let mut callback = None;
        let mut user_data = std::ptr::null_mut();
        let mut release_user_data = None;
        let future = submit(
            |completion| {
                // SAFETY: submit borrows a complete descriptor for this call.
                let completion = unsafe { &*completion };
                callback = completion.callback;
                user_data = completion.user_data;
                release_user_data = completion.release_user_data;
                sys::MLN_STATUS_OK
            },
            unit,
        )
        .unwrap();

        let dropped = Arc::new(AtomicBool::new(false));
        future.retain(Arc::new(DropProbe(Arc::clone(&dropped))));
        drop(future);
        assert!(!dropped.load(Ordering::Acquire));

        let mut result = unsafe { std::mem::zeroed::<sys::mln_completion_result>() };
        result.size = std::mem::size_of::<sys::mln_completion_result>() as u32;
        result.status = sys::MLN_STATUS_OK;
        // SAFETY: the captured descriptor owns user_data through release.
        unsafe { callback.unwrap()(user_data, &result) };
        assert!(!dropped.load(Ordering::Acquire));
        // SAFETY: release belongs to the same accepted descriptor.
        unsafe { release_user_data.unwrap()(user_data) };
        assert!(dropped.load(Ordering::Acquire));
    }
}
