// Ports maplibre-native-ffi src/logging/logging.cpp and src/c_api/logging.cpp
// onto `mln_util::logging::Log`: one process-wide log callback that receives
// every record the core logs.
//
// TODO(ffi): the async severity mask is stored and validated, but every
// record reaches the callback on the thread that logged it (C++ logs the
// selected severities on a background thread).

use crate::delivery::{UserData, deliver};
use crate::diagnostics::{Failure, boundary};
use crate::handles;
use crate::types::*;
use maplibre_native_next::mln_util::logging::{Event, EventSeverity, Log, Observer};
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{Arc, Mutex, Once};

struct Handler {
    callback: mln_log_callback,
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl Handler {
    /// Releases the registration's user_data: now without a queue, else
    /// after the queue's earlier calls.
    fn release(&self) {
        if let Some(release) = self.release_user_data {
            let user_data = self.user_data;
            deliver(self.queue, Box::new(move || {
                // SAFETY: the registration's release, called once.
                unsafe { release(user_data.get()) };
            }));
        }
    }
}

static HANDLER: Mutex<Option<Arc<Handler>>> = Mutex::new(None);
static ASYNC_MASK: AtomicU32 = AtomicU32::new(MLN_LOG_SEVERITY_FLAG_DEFAULT);
static INSTALL: Once = Once::new();

struct FfiObserver;

impl Observer for FfiObserver {
    fn on_record(&self, severity: EventSeverity, event: Event, code: i64, msg: &str) -> bool {
        let Some(handler) = HANDLER.lock().unwrap().clone() else {
            return false;
        };
        let Some(callback) = handler.callback else {
            return false;
        };
        let severity = match severity {
            EventSeverity::Debug => return false,
            EventSeverity::Info => 1,
            EventSeverity::Warning => 2,
            EventSeverity::Error => 3,
        };
        let event = event as u32;
        let message = LentText::new(msg);
        if handler.queue == 0 {
            // SAFETY: the caller registered this callback and user_data; the
            // message lives until it returns.
            return unsafe { callback(handler.user_data.get(), severity, event, code, mln_buffer_view::of_text(&message)) };
        }
        let user_data = handler.user_data;
        deliver(handler.queue, Box::new(move || {
            // SAFETY: as above, on the dispatching thread.
            unsafe { callback(user_data.get(), severity, event, code, mln_buffer_view::of_text(&message)) };
        }));
        true
    }
}

/// # Safety
/// C API contract (logging.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_log_set_callback(handler: *const mln_log_handler, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let Some(handler) = read_sized(handler) else {
                return Err(Failure::invalid_argument("handler must be an mln_log_handler"));
            };
            if handler.callback.is_none() {
                return Err(Failure::invalid_argument("handler.callback must not be null"));
            }
            if handler.queue.0 != 0 {
                handles::queue(handler.queue)?;
            }
            INSTALL.call_once(|| Log::set_observer(Box::new(FfiObserver)));
            let previous = HANDLER.lock().unwrap().replace(Arc::new(Handler {
                callback: handler.callback,
                user_data: UserData(handler.user_data),
                release_user_data: handler.release_user_data,
                queue: handler.queue.0,
            }));
            if let Some(previous) = previous {
                previous.release();
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (logging.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_log_clear_callback(out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let previous = HANDLER.lock().unwrap().take();
            if let Some(previous) = previous {
                previous.release();
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (logging.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_log_set_async_severity_mask(mask: u32, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if mask & !MLN_LOG_SEVERITY_FLAG_ALL != 0 {
                return Err(Failure::invalid_argument("log async severity mask contains unknown bits"));
            }
            ASYNC_MASK.store(mask, Ordering::SeqCst);
            Ok(())
        })
    }
}
