#[cfg(test)]
pub(crate) mod test_support {
    use std::sync::{Mutex, MutexGuard};

    use crate::{LogSeverityMask, log_clear_callback, log_set_async_severity_mask};

    static LOGGING_TEST_LOCK: Mutex<()> = Mutex::new(());

    /// Serializes tests that install the process-global log callback and
    /// restores default logging when each one finishes.
    pub(crate) struct LoggingTestGuard {
        _lock: MutexGuard<'static, ()>,
    }

    impl LoggingTestGuard {
        pub(crate) fn new() -> Self {
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
        let _ = log_clear_callback();
        let _ = log_set_async_severity_mask(LogSeverityMask::DEFAULT);
    }
}

#[cfg(test)]
mod tests {
    use super::test_support::LoggingTestGuard;
    use std::sync::{
        Arc,
        atomic::{AtomicUsize, Ordering},
    };
    #[test]

    fn replacing_a_log_callback_releases_the_one_it_replaced() {
        let _guard = LoggingTestGuard::new();
        let first_calls = Arc::new(AtomicUsize::new(0));
        let second_calls = Arc::new(AtomicUsize::new(0));
        let first_callback_calls = Arc::clone(&first_calls);
        crate::log_set_callback(Some(Arc::new(move |_, _, _, _| {
            first_callback_calls.fetch_add(1, Ordering::SeqCst);
            1
        })))
        .unwrap();
        assert_eq!(Arc::strong_count(&first_calls), 2);

        let second_callback_calls = Arc::clone(&second_calls);
        crate::log_set_callback(Some(Arc::new(move |_, _, _, _| {
            second_callback_calls.fetch_add(1, Ordering::SeqCst);
            1
        })))
        .unwrap();
        assert_eq!(Arc::strong_count(&first_calls), 1);
        assert_eq!(first_calls.load(Ordering::SeqCst), 0);
        assert_eq!(second_calls.load(Ordering::SeqCst), 0);

        crate::log_clear_callback().unwrap();
        assert_eq!(Arc::strong_count(&second_calls), 1);
    }
}
