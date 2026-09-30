use maplibre_native_ffi_core::{self as maplibre_core, handle::NativeHandle};

use crate::{Error, Result};

#[derive(Debug)]
enum ConcurrentHandleState<T> {
    Live(T, usize),
    Closing,
    Closed,
}

#[derive(Debug)]
pub(crate) struct ConcurrentNativeHandle<T: NativeHandle> {
    state: std::sync::Mutex<ConcurrentHandleState<T>>,
    type_name: &'static str,
}

impl<T: NativeHandle> ConcurrentNativeHandle<T> {
    /// Takes ownership of a native handle whose registry and control state are
    /// safe to inspect from any thread.
    ///
    /// # Safety
    ///
    /// `handle` must be a live owned handle of the matching native type.
    pub(crate) unsafe fn from_handle(handle: T, type_name: &'static str) -> Result<Self> {
        if handle.to_raw() == 0 {
            return Err(Error::invalid_argument(format!(
                "{type_name} handle must not be zero"
            )));
        }
        Ok(Self {
            state: std::sync::Mutex::new(ConcurrentHandleState::Live(handle, 0)),
            type_name,
        })
    }

    pub(crate) fn live_handle(&self) -> Option<T> {
        match *self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            ConcurrentHandleState::Live(handle, _) => Some(handle),
            ConcurrentHandleState::Closing | ConcurrentHandleState::Closed => None,
        }
    }

    pub(crate) fn is_closed(&self) -> bool {
        matches!(
            *self
                .state
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner()),
            ConcurrentHandleState::Closed
        )
    }

    pub(crate) fn read_handle(&self) -> Result<NativeRead<'_, T>> {
        let mut state = lock(&self.state);
        match &mut *state {
            ConcurrentHandleState::Live(native, readers) => {
                *readers += 1;
                Ok(NativeRead {
                    owner: self,
                    native: *native,
                })
            }
            _ => Err(closed_handle_error(self.type_name)),
        }
    }

    pub(crate) fn close_with<R>(&self, close: impl FnOnce(T) -> Result<R>) -> Result<Option<R>> {
        maplibre_core::callback::check("", 0)?;
        let mut state = lock(&self.state);
        let native = match *state {
            ConcurrentHandleState::Live(native, 0) => native,
            ConcurrentHandleState::Closed => return Ok(None),
            _ => {
                return Err(Error::new(
                    crate::ErrorKind::InvalidState,
                    None,
                    "handle has an active close or borrowed read",
                ));
            }
        };
        *state = ConcurrentHandleState::Closing;
        drop(state);
        let mut reservation = CloseReservation {
            owner: self,
            native,
            accepted: false,
        };
        let result = close(native);
        reservation.accepted = result.is_ok();
        result.map(Some)
    }

    pub(crate) fn finalize_with(&mut self, dispose: impl FnOnce(T) -> Result<()> + Send + 'static) {
        let state = self
            .state
            .get_mut()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let old = std::mem::replace(state, ConcurrentHandleState::Closed);
        if let ConcurrentHandleState::Live(handle, _) = old {
            let id = handle.to_raw();
            let type_name = self.type_name;
            maplibre_core::callback::finalize(move || {
                if dispose(T::from_raw(id)).is_err() {
                    maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                        type_name,
                        id,
                    });
                }
            });
        }
    }
}

pub(crate) struct NativeRead<'a, T: NativeHandle> {
    owner: &'a ConcurrentNativeHandle<T>,
    pub(crate) native: T,
}
impl<T: NativeHandle> Drop for NativeRead<'_, T> {
    fn drop(&mut self) {
        if let ConcurrentHandleState::Live(_, readers) = &mut *lock(&self.owner.state) {
            *readers -= 1;
        }
    }
}
struct CloseReservation<'a, T: NativeHandle> {
    owner: &'a ConcurrentNativeHandle<T>,
    native: T,
    accepted: bool,
}
impl<T: NativeHandle> Drop for CloseReservation<'_, T> {
    fn drop(&mut self) {
        *lock(&self.owner.state) = if self.accepted {
            ConcurrentHandleState::Closed
        } else {
            ConcurrentHandleState::Live(self.native, 0)
        };
    }
}

/// Recovers a mutex guard after a panic poisoned the lock. The state these
/// wrappers guard is a handle or a completion result, which stays consistent
/// across a panic, so a poisoned lock is not a reason to fail a native call.
pub(crate) fn lock<T>(mutex: &std::sync::Mutex<T>) -> std::sync::MutexGuard<'_, T> {
    mutex
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

pub(crate) fn closed_handle_error(type_name: &'static str) -> Error {
    Error::invalid_argument(format!("{type_name} is closed"))
}

#[cfg(test)]
mod tests {
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::{Arc, Barrier, mpsc};
    use std::time::Duration;

    use maplibre_native_ffi_sys as sys;

    use super::*;

    #[test]
    fn concurrent_close_rejects_reentry_and_calls_native_once() {
        let handle = Arc::new(unsafe {
            ConcurrentNativeHandle::from_handle(sys::mln_render_session(1), "test session").unwrap()
        });
        let entered = Arc::new(Barrier::new(2));
        let finish = Arc::new(Barrier::new(2));
        let calls = Arc::new(AtomicUsize::new(0));

        let first_handle = Arc::clone(&handle);
        let first_entered = Arc::clone(&entered);
        let first_finish = Arc::clone(&finish);
        let first_calls = Arc::clone(&calls);
        let first = std::thread::spawn(move || {
            first_handle
                .close_with(|_| {
                    first_calls.fetch_add(1, Ordering::Relaxed);
                    first_entered.wait();
                    first_finish.wait();
                    Ok(())
                })
                .unwrap()
        });

        entered.wait();
        assert!(handle.live_handle().is_none());
        let second_handle = Arc::clone(&handle);
        let (sender, receiver) = mpsc::channel();
        let second = std::thread::spawn(move || {
            sender.send(second_handle.close_with(|_| Ok(()))).unwrap();
        });
        assert_eq!(
            receiver
                .recv_timeout(Duration::from_secs(10))
                .unwrap()
                .unwrap_err()
                .kind(),
            crate::ErrorKind::InvalidState
        );
        finish.wait();

        assert_eq!(first.join().unwrap(), Some(()));

        second.join().unwrap();
        assert_eq!(calls.load(Ordering::Relaxed), 1);
        assert!(handle.is_closed());
    }

    #[test]
    fn a_read_on_another_thread_holds_off_close_until_it_ends() {
        let handle = Arc::new(unsafe {
            ConcurrentNativeHandle::from_handle(sys::mln_event_batch(9), "batch").unwrap()
        });
        let reading = Arc::new(Barrier::new(2));
        let release = Arc::new(Barrier::new(2));

        let reader = std::thread::spawn({
            let handle = Arc::clone(&handle);
            let reading = Arc::clone(&reading);
            let release = Arc::clone(&release);
            move || {
                let read = handle.read_handle().unwrap();
                assert_eq!(read.native.0, 9);
                reading.wait();
                release.wait();
            }
        });

        reading.wait();
        let error = handle
            .close_with::<()>(|_| panic!("close reached native during a read"))
            .unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidState);
        assert_eq!(handle.live_handle().unwrap().0, 9);
        release.wait();
        reader.join().unwrap();

        handle.close_with(|_| Ok(())).unwrap();
        assert!(handle.is_closed());
    }

    #[test]
    fn a_rejected_or_panicking_close_leaves_the_handle_live() {
        let handle = unsafe {
            ConcurrentNativeHandle::from_handle(sys::mln_event_batch(9), "batch").unwrap()
        };
        assert!(
            handle
                .close_with::<()>(|_| Err(Error::invalid_argument("rejected")))
                .is_err()
        );
        assert_eq!(handle.live_handle().unwrap().0, 9);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let _ = handle.close_with::<()>(|_| panic!("host unwind"));
        }));
        assert_eq!(handle.live_handle().unwrap().0, 9);
        handle.close_with(|_| Ok(())).unwrap();
        assert!(handle.is_closed());
    }

    #[test]
    fn a_handle_dropped_in_a_callback_disposes_off_its_stack_and_reports_a_failure() {
        let (sender, leaks) = mpsc::channel();
        crate::set_leak_reporter(Some(Box::new(move |leak| {
            let _ = sender.send((leak, std::thread::current().id()));
        })));
        let id = 0x0d00_0000_0000_0007;
        let mut handle =
            unsafe { ConcurrentNativeHandle::from_handle(sys::mln_map(id), "mln_map").unwrap() };

        // A drop inside a native callback must not dispose on that callback's
        // stack, so the disposal runs on the finalization thread, and its
        // failure reaches the reporter there.
        {
            let _callback = maplibre_core::callback::PolicyScope::enter(&[], 0);
            handle.finalize_with(|_| Err(Error::invalid_argument("dispose refused")));
        }
        let received = leaks.recv_timeout(Duration::from_secs(10));
        crate::set_leak_reporter(None);

        let (leak, thread) = received.expect("the failed disposal was never reported");
        assert_eq!(
            leak,
            crate::NativeHandleLeak {
                type_name: "mln_map",
                id,
            }
        );
        assert_ne!(thread, std::thread::current().id());
        assert!(handle.is_closed());
    }
}
