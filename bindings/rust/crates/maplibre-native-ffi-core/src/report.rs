//! The process-wide reporter for failures that no caller can receive.

use std::io::Write;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::{Arc, Mutex};

use crate::Error;
use crate::handle::NativeHandleLeak;

/// A failure that the binding contained because no caller could receive it.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum Report {
    /// A handle that a destructor could not destroy. The handle stays live.
    /// Explicit `close` reports the same failure through the normal error
    /// path.
    LeakedHandle(NativeHandleLeak),
    /// An error that a callback returned or failed to decode. Native received
    /// the callback's failure value instead.
    CallbackError {
        /// The C callback type that failed, such as
        /// `mln_resource_transform_callback`.
        callback: &'static str,
        /// The error that native could not receive.
        error: Error,
    },
}

/// Receives each [`Report`].
pub type Reporter = Box<dyn Fn(Report) + Send + Sync>;

type SharedReporter = Arc<dyn Fn(Report) + Send + Sync>;

static REPORTER: Mutex<Option<SharedReporter>> = Mutex::new(None);

/// Installs the process-wide reporter, replacing any previous one, and returns
/// whether one was installed. With no reporter, each report goes to standard
/// error.
///
/// The reporter may run on any thread, including a native callback thread, so
/// it should return quickly. A callback error reaches it on that callback's
/// stack, where the binding refuses every native call. A panic it raises is
/// discarded.
pub fn set_reporter(reporter: Option<Reporter>) -> bool {
    let mut slot = REPORTER
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let replaced = slot.is_some();
    *slot = reporter.map(Arc::from);
    replaced
}

/// Delivers a report to the installed reporter, or to standard error. Never
/// panics: it is called from `Drop` and from native callbacks, where unwinding
/// would abort.
pub fn report(report: Report) {
    // The reporter runs outside the lock, so it may replace itself.
    let reporter = REPORTER
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
        .clone();
    let _ = catch_unwind(AssertUnwindSafe(|| match reporter {
        Some(reporter) => reporter(report),
        None => {
            let line = match report {
                Report::LeakedHandle(leak) => format!(
                    "Leaked {} native handle {:#x}; close it explicitly.",
                    leak.type_name, leak.id
                ),
                Report::CallbackError { callback, error } => {
                    format!("{callback} failed and native received its fallback: {error}")
                }
            };
            let _ = writeln!(std::io::stderr(), "maplibre-native-ffi: {line}");
        }
    }));
}
