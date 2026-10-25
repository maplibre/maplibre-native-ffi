use std::ffi::c_void;
use std::future::Future;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::pin::Pin;
use std::sync::{Arc, Condvar, Mutex};
use std::task::{Context, Poll, Waker};
use std::time::Duration;

use maplibre_native_ffi_core as core;
use maplibre_native_ffi_sys as sys;

use crate::convert::{FromNative, from_native};
use crate::handle::lock;
use crate::{Error, Result};

/// The generations that a committed command published, which are zero unless
/// it is a map command.
///
/// A command's future resolves to an [`Error`] instead when the command fails
/// or is cancelled, carrying its status, diagnostic, and generation.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CommandCompletion {
    /// The map snapshot generation that the command published.
    pub generation: u64,
    /// The first map render update that includes the command; compare it with
    /// a rendered frame's `map_update_generation`.
    pub render_update_generation: u64,
}

struct State<T> {
    result: Mutex<Option<Result<T>>>,
    ready: Condvar,
    waker: Mutex<Option<Waker>>,
}

type Converter<T> = Box<dyn FnOnce(&sys::mln_completion_result) -> Result<T> + Send>;

struct Bridge<T> {
    state: Arc<State<T>>,
    convert: Mutex<Option<Converter<T>>>,
}

/// A one-shot native completion exposed as a Rust future.
///
/// The future resolves to an [`Error`] when the operation fails, which for a
/// command means it failed or was cancelled.
///
/// Native has accepted the operation by the time the call that returns this
/// future returns, and runs it whether or not anything awaits the result.
/// Dropping the future doesn't cancel the operation; native still delivers and
/// frees its result.
#[must_use = "the operation is already submitted, and dropping its future ignores a failure; \
              use the owner's `discarding()` view to submit a command without one"]
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

    /// Blocks the calling thread for at most `timeout` and returns the result,
    /// or gives the future back when the timeout ends first. This serves hosts
    /// that must pump another service loop between waits rather than run an
    /// async executor.
    pub fn wait(self, timeout: Duration) -> std::result::Result<Result<T>, Self> {
        let result = {
            let result = lock(&self.state.result);
            self.state
                .ready
                .wait_timeout_while(result, timeout, |result| result.is_none())
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .0
                .take()
        };
        result.ok_or(self)
    }
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
    let _scope = core::callback::CallbackScope::enter();
    if user_data.is_null() || result.is_null() {
        return;
    }
    // SAFETY: native owns Bridge from successful submission through release.
    let bridge = unsafe { &*user_data.cast::<Bridge<T>>() };
    // SAFETY: completion result is borrowed for this callback.
    let raw = unsafe { &*result };
    let converted = if raw.status == sys::MLN_STATUS_OK {
        match lock(&bridge.convert).take() {
            Some(convert) => match catch_unwind(AssertUnwindSafe(|| convert(raw))) {
                Ok(result) => result,
                Err(payload) => {
                    // A panic payload can itself panic when dropped across this C boundary.
                    std::mem::forget(payload);
                    Err(Error::new(
                        crate::Status::NativeError,
                        "completion conversion panicked",
                    ))
                }
            },
            None => Err(Error::new(
                crate::Status::InvalidState,
                "native completion ran more than once",
            )),
        }
    } else {
        Err(Error::Status {
            status: crate::Status::from_raw(raw.status),
            diagnostic: copy_diagnostic(raw),
            generation: raw.generation,
        })
    };
    *lock(&bridge.state.result) = Some(converted);
    bridge.state.ready.notify_all();
    let waker = lock(&bridge.state.waker).take();
    if let Some(waker) = waker
        && let Err(payload) = catch_unwind(AssertUnwindSafe(|| waker.wake()))
    {
        crate::callback::report_panic("mln_completion_callback", payload);
    }
}

unsafe extern "C" fn release<T>(user_data: *mut c_void) {
    let _scope = core::callback::CallbackScope::enter();
    if !user_data.is_null() {
        // SAFETY: native calls release once after accepted completion delivery.
        let bridge = unsafe { Box::from_raw(user_data.cast::<Bridge<T>>()) };
        if let Err(payload) = catch_unwind(AssertUnwindSafe(|| drop(bridge))) {
            crate::callback::report_panic("mln_user_data_release", payload);
        }
    }
}

/// Submits one native operation whose completion carries a value or nothing.
/// A failed completion, including a command that fails or is cancelled,
/// resolves the future as an error.
pub(crate) fn submit<T, S, C>(submit: S, convert: C) -> Result<NativeFuture<T>>
where
    T: Send + 'static,
    S: FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    C: FnOnce(&sys::mln_completion_result) -> Result<T> + Send + 'static,
{
    let state = Arc::new(State {
        result: Mutex::new(None),
        ready: Condvar::new(),
        waker: Mutex::new(None),
    });
    let bridge = Box::new(Bridge {
        state: Arc::clone(&state),
        convert: Mutex::new(Some(Box::new(convert))),
    });
    let bridge = Box::into_raw(bridge);
    let completion = sys::mln_completion {
        size: std::mem::size_of::<sys::mln_completion>() as u32,
        callback: Some(complete::<T>),
        user_data: bridge.cast(),
        release_user_data: Some(release::<T>),
        queue: sys::mln_delivery_queue(0),
    };
    if let Err(error) = core::check(|diagnostic| submit(&completion, diagnostic)) {
        // SAFETY: rejected submissions invoke neither callback and retain no pointer.
        drop(unsafe { Box::from_raw(bridge) });
        return Err(error);
    }
    Ok(NativeFuture { state })
}

pub(crate) fn unit(result: &sys::mln_completion_result) -> Result<()> {
    if !result.value.is_null() || result.value_count != 0 {
        return Err(Error::new(
            crate::Status::NativeError,
            "unit completion returned a value",
        ));
    }
    Ok(())
}

pub(crate) fn command(result: &sys::mln_completion_result) -> Result<CommandCompletion> {
    unit(result)?;
    Ok(CommandCompletion {
        generation: result.generation,
        render_update_generation: result.render_update_generation,
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
            crate::Status::NativeError,
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
            crate::Status::NativeError,
            "native completion returned a null slice",
        ));
    }
    // SAFETY: the submitting API defines value_count T elements for this
    // callback, value_size bytes apart.
    unsafe {
        crate::convert::strided_items(
            result.value.cast::<T>(),
            result.value_count,
            result.value_size,
        )
    }
}

/// Copies a completion's one value.
pub(crate) fn value<N: Copy, T: FromNative<N>>(result: &sys::mln_completion_result) -> Result<T> {
    let value = copy_value::<N>(result)?;
    // SAFETY: the value and the storage it points to are borrowed for the
    // completion callback, which runs this converter.
    unsafe { from_native(value) }
}

/// Copies a completion's value, or `None` when it carries no value.
pub(crate) fn optional<N: Copy, T: FromNative<N>>(
    result: &sys::mln_completion_result,
) -> Result<Option<T>> {
    if result.value_count == 0 {
        return Ok(None);
    }
    value::<N, T>(result).map(Some)
}

/// Copies every value of a completion.
pub(crate) fn list<N: Copy, T: FromNative<N>>(
    result: &sys::mln_completion_result,
) -> Result<Vec<T>> {
    copy_slice::<N>(result)?
        .into_iter()
        // SAFETY: as for `value`.
        .map(|value| unsafe { from_native(value) })
        .collect()
}

#[cfg(test)]
mod tests {
    use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

    use super::*;

    #[derive(Debug)]
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
        release_user_data: sys::mln_user_data_release,
    }

    /// A completion result with `status` and nothing else.
    fn result(status: sys::mln_status) -> sys::mln_completion_result {
        // SAFETY: an all-zero completion result is a valid empty result.
        let mut result = unsafe { std::mem::zeroed::<sys::mln_completion_result>() };
        result.size = std::mem::size_of::<sys::mln_completion_result>() as u32;
        result.status = status;
        result
    }

    impl AcceptedSubmission {
        fn deliver(&self, status: sys::mln_status) {
            self.deliver_result(&result(status));
        }

        fn deliver_result(&self, result: &sys::mln_completion_result) {
            // SAFETY: the descriptor owns user_data from acceptance through
            // release, and this is the one completion it delivers.
            unsafe { self.callback.unwrap()(self.user_data, result) };
        }

        fn release(&self) {
            // SAFETY: release belongs to the same accepted descriptor and runs
            // once.
            unsafe { self.release_user_data.unwrap()(self.user_data) };
        }
    }

    /// Accepts a submission without calling native, returning the future and
    /// the descriptor native would own.
    fn accept<T, C>(convert: C) -> (NativeFuture<T>, AcceptedSubmission)
    where
        T: Send + 'static,
        C: FnOnce(&sys::mln_completion_result) -> Result<T> + Send + 'static,
    {
        let mut accepted = None;
        let future = submit(
            |completion, _| {
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
        )
        .unwrap();
        (future, accepted.unwrap())
    }

    /// Returns the result of a future that native has already completed.
    fn completed<T>(future: NativeFuture<T>) -> Result<T> {
        future
            .wait(Duration::ZERO)
            .expect("the future has completed")
    }

    #[test]
    fn polling_a_future_wakes_the_task_that_registered_a_waker() {
        struct CountingWaker(AtomicUsize);

        impl std::task::Wake for CountingWaker {
            fn wake(self: Arc<Self>) {
                self.0.fetch_add(1, Ordering::Release);
            }
        }

        let (future, accepted) = accept(unit);
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

    /// A native build whose element grew reports a value_size wider than this
    /// binding's element, so the copy steps by it. A stride narrower than the
    /// element cannot hold one, so the copy fails.
    #[test]
    fn an_array_result_is_read_at_its_value_size() {
        // Two coordinates, each followed by a member this binding does not know.
        #[repr(C)]
        #[derive(Clone, Copy)]
        struct Wide {
            point: sys::mln_lat_lng,
            newer: f64,
        }
        let wide = [(1.0, 2.0), (3.0, 4.0)].map(|(latitude, longitude)| Wide {
            point: sys::mln_lat_lng {
                latitude,
                longitude,
            },
            newer: -1.0,
        });
        // SAFETY: an all-zero completion result is a valid empty result.
        let mut result = unsafe { std::mem::zeroed::<sys::mln_completion_result>() };
        result.status = sys::MLN_STATUS_OK;
        result.value = wide.as_ptr().cast();
        result.value_count = wide.len();
        result.value_size = std::mem::size_of::<Wide>() as u32;

        let points = copy_slice::<sys::mln_lat_lng>(&result).unwrap();
        assert_eq!(
            points
                .iter()
                .map(|point| (point.latitude, point.longitude))
                .collect::<Vec<_>>(),
            [(1.0, 2.0), (3.0, 4.0)]
        );

        result.value_size = std::mem::size_of::<sys::mln_lat_lng>() as u32 - 1;
        assert_eq!(
            copy_slice::<sys::mln_lat_lng>(&result)
                .unwrap_err()
                .status(),
            crate::Status::NativeError
        );
    }

    #[test]
    fn a_panicking_converter_completes_with_an_error_before_native_release() {
        let dropped = Arc::new(AtomicBool::new(false));
        let probe = DropProbe(Arc::clone(&dropped));
        let (future, accepted) = accept(move |_| -> Result<()> {
            let _probe = probe;
            panic!("converter panic");
        });
        accepted.deliver(sys::MLN_STATUS_OK);
        assert_eq!(
            completed(future).unwrap_err().status(),
            crate::Status::NativeError
        );
        assert!(dropped.load(Ordering::Acquire));
        accepted.release();
    }

    #[test]
    fn a_wait_gives_the_future_back_until_native_completes_it() {
        let (future, accepted) = accept(unit);

        // Nothing has completed, so the wait ends without a result.
        let future = future.wait(Duration::ZERO).unwrap_err();

        accepted.deliver(sys::MLN_STATUS_OK);
        completed(future).unwrap();
        accepted.release();
    }

    #[test]
    fn a_failed_or_cancelled_command_fails_its_wait_with_its_status_diagnostic_and_generation() {
        let diagnostic = b"style parse failed";
        let mut failed = result(sys::MLN_STATUS_INVALID_ARGUMENT);
        failed.disposition = sys::MLN_COMMAND_DISPOSITION_FAILED;
        failed.generation = 7;
        failed.diagnostic = sys::mln_buffer_view {
            data: diagnostic.as_ptr().cast(),
            size: diagnostic.len(),
        };
        let mut cancelled = result(sys::MLN_STATUS_CANCELLED);
        cancelled.disposition = sys::MLN_COMMAND_DISPOSITION_CANCELLED;
        for (delivered, status, diagnostic, generation) in [
            (
                failed,
                crate::Status::InvalidArgument,
                "style parse failed",
                7,
            ),
            (cancelled, crate::Status::Cancelled, "", 0),
        ] {
            let (future, accepted) = accept(command);
            accepted.deliver_result(&delivered);
            let error = completed(future).unwrap_err();
            assert_eq!(error.status(), status);
            assert_eq!(error.diagnostic(), diagnostic);
            assert_eq!(error.generation(), generation);
            accepted.release();
        }

        // Only a committed command resolves, with the generations it published.
        let mut committed = result(sys::MLN_STATUS_OK);
        committed.generation = 8;
        committed.render_update_generation = 3;
        let (future, accepted) = accept(command);
        accepted.deliver_result(&committed);
        assert_eq!(
            completed(future).unwrap(),
            CommandCompletion {
                generation: 8,
                render_update_generation: 3,
            }
        );
        accepted.release();
    }

    #[test]
    fn a_wait_times_out_and_a_dropped_future_leaves_native_its_bridge() {
        let dropped = Arc::new(AtomicBool::new(false));
        let probe = DropProbe(Arc::clone(&dropped));
        let (future, accepted) = accept(move |_| Ok(probe));

        // Nothing has completed, so a bounded wait gives up.
        let future = future.wait(Duration::ZERO).unwrap_err();

        // Dropping the future is how a caller stops waiting. Native still owns
        // the bridge, so the late value lands in it, and native's release
        // frees it.
        drop(future);
        accepted.deliver(sys::MLN_STATUS_OK);
        assert!(!dropped.load(Ordering::Acquire));
        accepted.release();
        assert!(dropped.load(Ordering::Acquire));
    }

    #[test]
    fn a_rejected_submission_frees_the_bridge_and_reports_the_status() {
        let dropped = Arc::new(AtomicBool::new(false));
        let probe = DropProbe(Arc::clone(&dropped));
        let error = submit(
            |_, _| sys::MLN_STATUS_INVALID_STATE,
            move |_| {
                // The converter owns the probe, so freeing the bridge drops it.
                let _ = &probe;
                Ok(())
            },
        )
        .unwrap_err();

        assert_eq!(error.status(), crate::Status::InvalidState);
        assert!(dropped.load(Ordering::Acquire));
    }
}
