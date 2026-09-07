use std::ffi::c_void;
use std::future::Future;
use std::pin::Pin;
use std::sync::{Arc, Condvar, Mutex};
use std::task::{Context, Poll, Waker};
use std::time::Duration;

use maplibre_native_ffi_core as core;
use maplibre_native_ffi_sys as sys;

use crate::handle::lock;
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
    /// Reports whether native has delivered the one terminal result.
    pub fn is_ready(&self) -> bool {
        lock(&self.state.result).is_some()
    }

    /// Blocks for at most `timeout`, for hosts that must pump another service
    /// loop while waiting rather than run an async executor.
    pub fn wait(&self, timeout: Duration) -> Result<bool> {
        let result = lock(&self.state.result);
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
        let mut result = lock(&self.state.result);
        result.take().ok_or_else(|| {
            Error::new(
                crate::ErrorKind::InvalidState,
                None,
                "native future has not completed",
            )
        })?
    }

    #[cfg(test)]
    pub(crate) fn terminal_status(&self) -> Result<sys::mln_status> {
        let terminal = lock(&self.state.terminal);
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
        let terminal = lock(&self.state.terminal);
        match terminal.as_ref() {
            Some((_, diagnostic)) => Ok(diagnostic.clone()),
            None => Err(Error::new(
                crate::ErrorKind::NotReady,
                None,
                "native future has not completed",
            )),
        }
    }
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
        let mut result = lock(&self.state.result);
        if let Some(result) = result.take() {
            return Poll::Ready(result);
        }
        *lock(&self.state.waker) = Some(context.waker().clone());
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
    let diagnostic = copy_diagnostic(raw);
    *lock(&bridge.state.terminal) = Some((raw.status, diagnostic.clone()));
    let converted = if raw.status == sys::MLN_STATUS_OK || bridge.accept_error_status {
        match lock(&bridge.convert).take() {
            Some(convert) => convert(raw),
            None => Err(Error::new(
                crate::ErrorKind::InvalidState,
                None,
                "native completion ran more than once",
            )),
        }
    } else {
        Err(Error::from_status_and_diagnostic(raw.status, diagnostic))
    };
    *lock(&bridge.state.result) = Some(converted);
    bridge.state.ready.notify_all();
    if let Some(waker) = lock(&bridge.state.waker).take() {
        waker.wake();
    }
}

unsafe extern "C" fn release<T>(user_data: *mut c_void) {
    if !user_data.is_null() {
        // SAFETY: native calls release once after accepted completion delivery.
        let bridge = unsafe { Box::from_raw(user_data.cast::<Bridge<T>>()) };
        drop(bridge);
    }
}

/// Submits one native operation whose completion carries a value or nothing.
/// A failed completion resolves the future as an error.
pub(crate) fn submit<T, S, C>(submit: S, convert: C) -> Result<NativeFuture<T>>
where
    T: Send + 'static,
    S: FnOnce(*const sys::mln_completion) -> sys::mln_status,
    C: FnOnce(&sys::mln_completion_result) -> Result<T> + Send + 'static,
{
    submit_with(submit, convert, false)
}

/// Submits one ordered command whose completion reports its own disposition
/// and status, so a rejected command resolves the future rather than failing
/// it.
pub(crate) fn submit_command<S>(submit: S) -> Result<NativeFuture<CommandCompletion>>
where
    S: FnOnce(*const sys::mln_completion) -> sys::mln_status,
{
    submit_with(submit, command, true)
}

fn submit_with<T, S, C>(submit: S, convert: C, accept_error_status: bool) -> Result<NativeFuture<T>>
where
    T: Send + 'static,
    S: FnOnce(*const sys::mln_completion) -> sys::mln_status,
    C: FnOnce(&sys::mln_completion_result) -> Result<T> + Send + 'static,
{
    let state = Arc::new(State {
        result: Mutex::new(None),
        terminal: Mutex::new(None),
        ready: Condvar::new(),
        waker: Mutex::new(None),
    });
    let bridge = Box::new(Bridge {
        state: Arc::clone(&state),
        convert: Mutex::new(Some(Box::new(convert))),
        accept_error_status,
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
    string_from_bytes(buffer(result)?)
}

pub(crate) fn optional_string(result: &sys::mln_completion_result) -> Result<Option<String>> {
    match optional_buffer(result)? {
        Some(bytes) => string_from_bytes(bytes).map(Some),
        None => Ok(None),
    }
}

fn string_from_bytes(bytes: Vec<u8>) -> Result<String> {
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
    use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

    use super::*;

    struct DropProbe(Arc<AtomicBool>);

    impl Drop for DropProbe {
        fn drop(&mut self) {
            self.0.store(true, Ordering::Release);
        }
    }

    /// One accepted native submission, captured so a test can deliver its
    /// completion by hand instead of driving a live map.
    struct AcceptedSubmission {
        callback: sys::mln_completion_callback,
        user_data: *mut c_void,
        release_user_data: sys::mln_completion_release,
    }

    impl AcceptedSubmission {
        fn deliver(&self, status: sys::mln_status) {
            // SAFETY: the descriptor owns user_data from acceptance through
            // release, and this is the one completion it delivers.
            let mut result = unsafe { std::mem::zeroed::<sys::mln_completion_result>() };
            result.size = std::mem::size_of::<sys::mln_completion_result>() as u32;
            result.status = status;
            result.disposition = if status == sys::MLN_STATUS_OK {
                sys::MLN_COMMAND_DISPOSITION_COMMITTED
            } else {
                sys::MLN_COMMAND_DISPOSITION_FAILED
            };
            // SAFETY: as above.
            unsafe { self.callback.unwrap()(self.user_data, &result) };
        }

        fn release(&self) {
            // SAFETY: release belongs to the same accepted descriptor and runs
            // once.
            unsafe { self.release_user_data.unwrap()(self.user_data) };
        }
    }

    /// Accepts a submission without calling native, returning the future and
    /// the descriptor native would own.
    fn accept<T, C>(convert: C, accept_error_status: bool) -> (NativeFuture<T>, AcceptedSubmission)
    where
        T: Send + 'static,
        C: FnOnce(&sys::mln_completion_result) -> Result<T> + Send + 'static,
    {
        let mut accepted = None;
        let future = submit_with(
            |completion| {
                // SAFETY: submit borrows a complete descriptor for this call.
                let completion = unsafe { &*completion };
                accepted = Some(AcceptedSubmission {
                    callback: completion.callback,
                    user_data: completion.user_data,
                    release_user_data: completion.release_user_data,
                });
                sys::MLN_STATUS_OK
            },
            convert,
            accept_error_status,
        )
        .unwrap();
        (future, accepted.unwrap())
    }

    #[test]
    fn polling_a_future_wakes_the_task_that_registered_a_waker() {
        struct CountingWaker(AtomicUsize);

        impl std::task::Wake for CountingWaker {
            fn wake(self: Arc<Self>) {
                self.0.fetch_add(1, Ordering::Release);
            }
        }

        let (future, accepted) = accept(unit, false);
        let waker = Arc::new(CountingWaker(AtomicUsize::new(0)));
        let context_waker = Waker::from(Arc::clone(&waker));
        let mut context = Context::from_waker(&context_waker);
        let mut future = Box::pin(future);

        assert!(future.as_mut().poll(&mut context).is_pending());
        assert_eq!(waker.0.load(Ordering::Acquire), 0);

        accepted.deliver(sys::MLN_STATUS_OK);
        assert_eq!(waker.0.load(Ordering::Acquire), 1);
        assert!(matches!(
            future.as_mut().poll(&mut context),
            Poll::Ready(Ok(()))
        ));
        accepted.release();
    }

    #[test]
    fn a_future_yields_its_one_result_to_exactly_one_taker() {
        let (future, accepted) = accept(unit, false);

        // Nothing has completed, so there is no result to take yet.
        let error = future.take().unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidState);

        accepted.deliver(sys::MLN_STATUS_OK);
        future.take().unwrap();

        // The result moves out of the future, so a second take finds none.
        let error = future.take().unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidState);
        accepted.release();
    }

    #[test]
    fn a_command_reports_a_rejection_while_other_operations_fail() {
        // A command's completion carries its own disposition and status, so a
        // rejected command resolves rather than failing the future.
        let (command, accepted) = accept(super::command, true);
        accepted.deliver(sys::MLN_STATUS_INVALID_ARGUMENT);
        let completion = command.take().unwrap();
        assert_eq!(completion.disposition, crate::CommandDisposition::Failed);
        assert_eq!(completion.raw_status, sys::MLN_STATUS_INVALID_ARGUMENT);
        accepted.release();

        // Every other operation reports a failed completion as an error.
        let (operation, accepted) = accept(unit, false);
        accepted.deliver(sys::MLN_STATUS_INVALID_ARGUMENT);
        let error = operation.take().unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        accepted.release();
    }

    #[test]
    fn a_rejected_submission_frees_the_bridge_and_reports_the_status() {
        let dropped = Arc::new(AtomicBool::new(false));
        let probe = DropProbe(Arc::clone(&dropped));
        let error = submit(
            |_| sys::MLN_STATUS_INVALID_STATE,
            move |_| {
                // The converter owns the probe, so freeing the bridge drops it.
                let _ = &probe;
                Ok(())
            },
        )
        .unwrap_err();

        assert_eq!(error.kind(), crate::ErrorKind::InvalidState);
        assert!(dropped.load(Ordering::Acquire));
    }
}
