use std::sync::Mutex;

pub use maplibre_native_ffi_sys::NativeHandle;

/// A native handle a destructor attempted to destroy and could not. The handle
/// stays live.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct NativeHandleLeak {
    /// The native type name, such as `mln_map`.
    pub type_name: &'static str,
    /// The handle id that was not destroyed.
    pub id: u64,
}

type LeakReporter = Box<dyn Fn(NativeHandleLeak) + Send + Sync>;

static LEAK_REPORTER: Mutex<Option<LeakReporter>> = Mutex::new(None);

/// Installs the process-wide reporter for handles a destructor could not
/// destroy, replacing any previous one, and returns whether one was installed.
/// Explicit `close` reports the same failure through the normal error path.
pub fn set_leak_reporter(reporter: Option<LeakReporter>) -> bool {
    let mut slot = LEAK_REPORTER
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let replaced = slot.is_some();
    *slot = reporter;
    replaced
}

/// Reports a handle a destructor could not destroy. Never panics: it is called
/// from `Drop`, where unwinding would abort.
pub fn report_leak(leak: NativeHandleLeak) {
    let slot = LEAK_REPORTER
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if let Some(reporter) = slot.as_ref() {
        // The reporter is arbitrary caller code running from `Drop`; unwinding
        // through a destructor during another unwind aborts the process.
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| reporter(leak)));
    }
}
