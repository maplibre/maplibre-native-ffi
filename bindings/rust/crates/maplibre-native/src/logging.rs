use std::ffi::{c_char, c_void};
use std::panic::{self, AssertUnwindSafe};
use std::sync::{Arc, Mutex, MutexGuard};

use crate::{Result, sys};
use maplibre_core::LogSeverityMask;
use maplibre_native_core as maplibre_core;

pub use maplibre_core::LogRecord;

type LogCallback = dyn Fn(LogRecord) -> bool + Send + Sync + 'static;

struct CallbackState {
    callback: Box<LogCallback>,
}

static LOG_CALLBACK_STATE: Mutex<Option<Arc<CallbackState>>> = Mutex::new(None);

fn lock_log_callback_state() -> MutexGuard<'static, Option<Arc<CallbackState>>> {
    LOG_CALLBACK_STATE
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

/// Installs or replaces the process-global MapLibre Native log callback.
///
/// MapLibre Native may invoke the callback from logging or worker threads. The
/// callback state must therefore be `Send + Sync + 'static`. The callback
/// should return quickly and avoid calling MapLibre Native APIs. Panics are
/// caught and reported to native logging as "not consumed".
pub fn set_log_callback<F>(callback: F) -> Result<()>
where
    F: Fn(LogRecord) -> bool + Send + Sync + 'static,
{
    let replacement = Arc::new(CallbackState {
        callback: Box::new(callback),
    });
    let user_data = Arc::as_ptr(&replacement).cast_mut().cast::<c_void>();
    let previous = {
        let mut current = lock_log_callback_state();
        // SAFETY: log_callback_trampoline has the C callback ABI. user_data
        // points at replacement, which is retained in current until the native
        // callback is replaced or cleared.
        maplibre_core::check(unsafe {
            sys::mln_log_set_callback(Some(log_callback_trampoline), user_data)
        })?;

        current.replace(replacement)
    };
    drop(previous);
    Ok(())
}

/// Clears the process-global MapLibre Native log callback.
pub fn clear_log_callback() -> Result<()> {
    // SAFETY: mln_log_clear_callback takes no arguments and clears native's
    // process-global callback slot.
    let previous = {
        let mut current = lock_log_callback_state();
        maplibre_core::check(unsafe { sys::mln_log_clear_callback() })?;
        current.take()
    };
    drop(previous);
    Ok(())
}

/// Configures severities that MapLibre Native may dispatch asynchronously.
pub fn set_async_log_severity_mask(mask: LogSeverityMask) -> Result<()> {
    // SAFETY: mask is passed by value. The C API validates unknown bits and
    // reports them as MLN_STATUS_INVALID_ARGUMENT.
    maplibre_core::check(unsafe { sys::mln_log_set_async_severity_mask(mask.bits()) })
}

unsafe extern "C" fn log_callback_trampoline(
    user_data: *mut c_void,
    severity: u32,
    event: u32,
    code: i64,
    message: *const c_char,
) -> u32 {
    if user_data.is_null() {
        return 0;
    }

    // SAFETY: set_log_callback installs Arc::as_ptr(&CallbackState) as
    // user_data and retains the current Arc until native replacement or clear
    // succeeds, so the pointer remains valid for native dispatch.
    let state = unsafe { &*user_data.cast::<CallbackState>() };
    invoke_callback(state, severity, event, code, message)
}

fn invoke_callback(
    state: &CallbackState,
    raw_severity: u32,
    raw_event: u32,
    code: i64,
    message: *const c_char,
) -> u32 {
    // SAFETY: message is supplied by the C logging callback contract as a
    // null-terminated string pointer. Invalid strings are treated as not
    // consumed.
    let Ok(record) = (unsafe {
        maplibre_core::logging::copy_log_record(raw_severity, raw_event, code, message)
    }) else {
        return 0;
    };

    match panic::catch_unwind(AssertUnwindSafe(|| (state.callback)(record))) {
        Ok(true) => 1,
        Ok(false) | Err(_) => 0,
    }
}

#[cfg(test)]
mod tests {
    use std::ffi::{CString, c_void};
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::{Arc, Mutex, MutexGuard};

    use super::*;
    use crate::ErrorKind;
    use maplibre_core::{LogEvent, LogSeverity};

    static LOGGING_TEST_LOCK: Mutex<()> = Mutex::new(());

    struct LoggingTestGuard {
        _lock: MutexGuard<'static, ()>,
    }

    impl LoggingTestGuard {
        fn new() -> Self {
            let guard = Self {
                _lock: LOGGING_TEST_LOCK
                    .lock()
                    .unwrap_or_else(|poisoned| poisoned.into_inner()),
            };
            clear_logging_after_test();
            guard
        }
    }

    impl Drop for LoggingTestGuard {
        fn drop(&mut self) {
            clear_logging_after_test();
        }
    }

    fn clear_logging_after_test() {
        let _ = clear_log_callback();
        let _ = set_async_log_severity_mask(LogSeverityMask::DEFAULT);
    }

    #[test]
    // Spec coverage: BND-120.
    fn log_callback_install_clear_and_trampoline_copy_record() {
        let _guard = LoggingTestGuard::new();
        let calls = Arc::new(AtomicUsize::new(0));
        let test_calls = calls.clone();

        set_log_callback(move |record| {
            test_calls.fetch_add(1, Ordering::SeqCst);
            if record.code == 42 {
                assert_eq!(record.severity, LogSeverity::Warning);
                assert_eq!(record.severity.raw_value(), sys::MLN_LOG_SEVERITY_WARNING);
                assert_eq!(record.event, LogEvent::Render);
                assert_eq!(record.event.raw_value(), sys::MLN_LOG_EVENT_RENDER);
                assert_eq!(record.message, "hello");
                return true;
            }
            false
        })
        .unwrap();

        let baseline_calls = calls.load(Ordering::SeqCst);
        let message = CString::new("hello").unwrap();
        let current = {
            let state = lock_log_callback_state();
            state.as_ref().unwrap().clone()
        };
        let user_data = Arc::as_ptr(&current).cast_mut().cast::<c_void>();
        assert_eq!(
            unsafe {
                log_callback_trampoline(
                    user_data,
                    sys::MLN_LOG_SEVERITY_WARNING,
                    sys::MLN_LOG_EVENT_RENDER,
                    42,
                    message.as_ptr(),
                )
            },
            1
        );
        assert_eq!(calls.load(Ordering::SeqCst), baseline_calls + 1);

        clear_log_callback().unwrap();
        assert!(lock_log_callback_state().is_none());
        assert_eq!(
            unsafe {
                log_callback_trampoline(
                    std::ptr::null_mut(),
                    sys::MLN_LOG_SEVERITY_WARNING,
                    sys::MLN_LOG_EVENT_RENDER,
                    42,
                    message.as_ptr(),
                )
            },
            0
        );
        assert_eq!(calls.load(Ordering::SeqCst), baseline_calls + 1);
    }

    #[test]
    // Spec coverage: BND-120.
    fn log_callback_replacement_invokes_only_replacement() {
        let _guard = LoggingTestGuard::new();
        let first_calls = Arc::new(AtomicUsize::new(0));
        let second_calls = Arc::new(AtomicUsize::new(0));
        let first_callback_calls = Arc::clone(&first_calls);
        set_log_callback(move |_| {
            first_callback_calls.fetch_add(1, Ordering::SeqCst);
            true
        })
        .unwrap();

        let second_callback_calls = Arc::clone(&second_calls);
        set_log_callback(move |_| {
            second_callback_calls.fetch_add(1, Ordering::SeqCst);
            true
        })
        .unwrap();

        let current = {
            let state = lock_log_callback_state();
            state.as_ref().unwrap().clone()
        };
        let message = CString::new("replacement").unwrap();
        assert_eq!(
            invoke_callback(
                &current,
                sys::MLN_LOG_SEVERITY_INFO,
                sys::MLN_LOG_EVENT_GENERAL,
                7,
                message.as_ptr(),
            ),
            1
        );

        assert_eq!(first_calls.load(Ordering::SeqCst), 0);
        assert_eq!(second_calls.load(Ordering::SeqCst), 1);

        clear_log_callback().unwrap();
        assert!(lock_log_callback_state().is_none());
    }

    #[test]
    // Spec coverage: BND-121.
    fn invalid_utf8_log_messages_are_not_consumed() {
        let _guard = LoggingTestGuard::new();
        set_log_callback(|_| true).unwrap();
        let invalid = b"\xff\0";
        let current = {
            let state = lock_log_callback_state();
            state.as_ref().unwrap().clone()
        };

        assert_eq!(
            invoke_callback(
                &current,
                sys::MLN_LOG_SEVERITY_ERROR,
                sys::MLN_LOG_EVENT_GENERAL,
                0,
                invalid.as_ptr().cast(),
            ),
            0
        );

        clear_log_callback().unwrap();
    }

    #[test]
    // Spec coverage: BND-121.
    fn log_callback_panics_are_not_consumed() {
        let _guard = LoggingTestGuard::new();
        set_log_callback(|_| panic!("contained panic")).unwrap();

        let message = CString::new("boom").unwrap();
        let current = {
            let state = lock_log_callback_state();
            state.as_ref().unwrap().clone()
        };

        assert_eq!(
            invoke_callback(
                &current,
                sys::MLN_LOG_SEVERITY_ERROR,
                sys::MLN_LOG_EVENT_GENERAL,
                0,
                message.as_ptr(),
            ),
            0
        );

        clear_log_callback().unwrap();
    }

    #[test]
    // Spec coverage: BND-020.
    fn async_log_severity_mask_status_propagates_invalid_bits() {
        let _guard = LoggingTestGuard::new();
        let invalid_mask = LogSeverityMask::from_bits_retain(1 << 31);

        let error = set_async_log_severity_mask(invalid_mask).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
    }

    #[test]
    // Spec coverage: BND-120.
    fn async_log_severity_mask_accepts_known_values() {
        let _guard = LoggingTestGuard::new();

        set_async_log_severity_mask(LogSeverityMask::INFO | LogSeverityMask::ERROR).unwrap();
        set_async_log_severity_mask(LogSeverityMask::DEFAULT).unwrap();
    }
}
