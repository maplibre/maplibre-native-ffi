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

struct GlobalLogCallbackState {
    current: Option<Arc<CallbackState>>,
    retained: Vec<Arc<CallbackState>>,
}

static LOG_CALLBACK_STATE: Mutex<GlobalLogCallbackState> = Mutex::new(GlobalLogCallbackState {
    current: None,
    retained: Vec::new(),
});

fn lock_log_callback_state() -> MutexGuard<'static, GlobalLogCallbackState> {
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
    // SAFETY: log_callback_trampoline has the C callback ABI. user_data points
    // at replacement, which is retained for the process lifetime below so native
    // and in-flight callbacks never observe a dangling pointer.
    maplibre_core::check(unsafe {
        sys::mln_log_set_callback(Some(log_callback_trampoline), user_data)
    })?;

    let mut state = lock_log_callback_state();
    state.current = Some(replacement.clone());
    state.retained.push(replacement);
    Ok(())
}

/// Clears the process-global MapLibre Native log callback.
pub fn clear_log_callback() -> Result<()> {
    // SAFETY: mln_log_clear_callback takes no arguments and clears native's
    // process-global callback slot.
    maplibre_core::check(unsafe { sys::mln_log_clear_callback() })?;

    lock_log_callback_state().current = None;
    Ok(())
}

/// Configures severities that MapLibre Native may dispatch asynchronously.
pub fn set_async_log_severity_mask(mask: LogSeverityMask) -> Result<()> {
    // SAFETY: mask is passed by value. The C API validates unknown bits and
    // reports them as MLN_STATUS_INVALID_ARGUMENT.
    maplibre_core::check(unsafe { sys::mln_log_set_async_severity_mask(mask.bits()) })
}

/// Restores MapLibre Native's default async log severity mask.
pub fn restore_default_async_log_severity_mask() -> Result<()> {
    set_async_log_severity_mask(LogSeverityMask::DEFAULT)
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
    // user_data and retains every installed Arc for the process lifetime, so the
    // pointer remains valid for native dispatch and in-flight callbacks.
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
        let _ = restore_default_async_log_severity_mask();
    }

    #[test]
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
            state.current.as_ref().unwrap().clone()
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
        assert!(lock_log_callback_state().current.is_none());
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
    fn invalid_utf8_log_messages_are_not_consumed() {
        let _guard = LoggingTestGuard::new();
        set_log_callback(|_| true).unwrap();
        let invalid = b"\xff\0";
        let current = {
            let state = lock_log_callback_state();
            state.current.as_ref().unwrap().clone()
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
    fn log_callback_panics_are_not_consumed() {
        let _guard = LoggingTestGuard::new();
        set_log_callback(|_| panic!("contained panic")).unwrap();

        let message = CString::new("boom").unwrap();
        let current = {
            let state = lock_log_callback_state();
            state.current.as_ref().unwrap().clone()
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
    fn async_log_severity_mask_status_propagates_invalid_bits() {
        let _guard = LoggingTestGuard::new();
        let invalid_mask = LogSeverityMask::from_bits_retain(1 << 31);

        let error = set_async_log_severity_mask(invalid_mask).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
    }

    #[test]
    fn async_log_severity_mask_accepts_known_values() {
        let _guard = LoggingTestGuard::new();

        set_async_log_severity_mask(LogSeverityMask::INFO | LogSeverityMask::ERROR).unwrap();
        restore_default_async_log_severity_mask().unwrap();
    }
}
