use maplibre_native_ffi_core::{self as maplibre_core, handle::NativeHandle};

use crate::{Error, Result};

#[derive(Debug)]
enum ConcurrentHandleState<T> {
    Live(T),
    Closing,
    Closed,
}

static CLOSE_FINISHED: std::sync::Condvar = std::sync::Condvar::new();

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
            state: std::sync::Mutex::new(ConcurrentHandleState::Live(handle)),
            type_name,
        })
    }

    pub(crate) fn live_handle(&self) -> Option<T> {
        match *self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            ConcurrentHandleState::Live(handle) => Some(handle),
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

    pub(crate) fn close_with<R>(&self, close: impl FnOnce(T) -> Result<R>) -> Result<Option<R>> {
        let mut close = Some(close);
        loop {
            let mut state = self
                .state
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            match *state {
                ConcurrentHandleState::Live(handle) => {
                    *state = ConcurrentHandleState::Closing;
                    drop(state);
                    let result = close.take().expect("close callback runs once")(handle);
                    let mut state = self
                        .state
                        .lock()
                        .unwrap_or_else(|poisoned| poisoned.into_inner());
                    *state = if result.is_ok() {
                        ConcurrentHandleState::Closed
                    } else {
                        ConcurrentHandleState::Live(handle)
                    };
                    CLOSE_FINISHED.notify_all();
                    return result.map(Some);
                }
                ConcurrentHandleState::Closing => {
                    state = CLOSE_FINISHED
                        .wait(state)
                        .unwrap_or_else(|poisoned| poisoned.into_inner());
                    drop(state);
                }
                ConcurrentHandleState::Closed => return Ok(None),
            }
        }
    }

    pub(crate) fn leak_for_report(&self) {
        let handle = {
            let mut state = self
                .state
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            match *state {
                ConcurrentHandleState::Live(handle) => {
                    *state = ConcurrentHandleState::Closed;
                    Some(handle)
                }
                ConcurrentHandleState::Closing | ConcurrentHandleState::Closed => None,
            }
        };
        let Some(handle) = handle else { return };
        maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
            type_name: self.type_name,
            id: handle.to_raw(),
        });
    }
}

pub(crate) fn closed_handle_error(type_name: &'static str) -> Error {
    Error::invalid_argument(format!("{type_name} is closed"))
}

pub(crate) fn out_handle<T: NativeHandle>(
    out: maplibre_core::ptr::OutHandle<T>,
    type_name: &'static str,
) -> Result<T> {
    out.into_live(type_name)
}

#[cfg(test)]
mod tests {
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::{Arc, Barrier, mpsc};
    use std::time::Duration;

    use maplibre_native_ffi_sys as sys;

    use super::*;

    #[test]
    fn concurrent_close_calls_native_once_and_waits_for_the_winner() {
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
            sender
                .send(second_handle.close_with(|_| Ok(())).unwrap())
                .unwrap();
        });
        assert!(receiver.recv_timeout(Duration::from_millis(20)).is_err());
        finish.wait();

        assert_eq!(first.join().unwrap(), Some(()));
        assert_eq!(receiver.recv_timeout(Duration::from_secs(1)).unwrap(), None);
        second.join().unwrap();
        assert_eq!(calls.load(Ordering::Relaxed), 1);
        assert!(handle.is_closed());
    }
}
