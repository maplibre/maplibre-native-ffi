use std::ffi::CStr;
use std::os::raw::{c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

use napi::bindgen_prelude::{BigInt, Env, Result};
use napi::threadsafe_function::{ThreadsafeFunction, ThreadsafeFunctionCallMode};
use napi_derive::napi;

use crate::error;

#[napi(object)]
pub struct RenderBackends {
    pub raw_mask: u32,
    pub metal: bool,
    pub vulkan: bool,
    pub opengl: bool,
}

#[napi(object)]
pub struct OpenGLContextProviders {
    pub raw_mask: u32,
    pub wgl: bool,
    pub egl: bool,
}

#[napi(object)]
pub struct NetworkStatusValue {
    pub kind: String,
    pub raw: u32,
}

#[napi(object)]
pub struct LogRecord {
    pub severity: String,
    pub raw_severity: u32,
    pub event: String,
    pub raw_event: u32,
    pub code: i64,
    pub message: String,
}

#[napi(object)]
pub struct NativeLeakReport {
    pub handle_type: String,
    pub address: BigInt,
}

struct NativeLeakReportEntry {
    handle_type: String,
    address: usize,
}

static LOG_CALLBACK: OnceLock<Mutex<Option<Arc<LogCallbackState>>>> = OnceLock::new();
static LOG_CALLBACK_IDS: AtomicU64 = AtomicU64::new(1);
static NATIVE_LEAK_REPORTS: OnceLock<Mutex<Vec<NativeLeakReportEntry>>> = OnceLock::new();

struct LogCallbackState {
    id: u64,
    callback: Arc<ThreadsafeFunction<LogRecord>>,
}

#[napi(js_name = "cVersion")]
pub fn c_version() -> u32 {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    unsafe { maplibre_native_sys::mln_c_version() }
}

#[napi(js_name = "threadLastErrorMessage")]
pub fn thread_last_error_message() -> String {
    // SAFETY: The C API returns a process-owned null-terminated diagnostic
    // pointer for the current thread, or null defensively on older builds.
    let message = unsafe { maplibre_native_sys::mln_thread_last_error_message() };
    copy_log_message(message)
}

#[napi(js_name = "supportedRenderBackends")]
pub fn supported_render_backends() -> RenderBackends {
    // SAFETY: mln_supported_render_backend_mask takes no arguments and returns a
    // value mask. Unknown future bits are preserved in raw_mask.
    let raw_mask = unsafe { maplibre_native_sys::mln_supported_render_backend_mask() };
    RenderBackends {
        raw_mask,
        metal: raw_mask & maplibre_native_sys::MLN_RENDER_BACKEND_FLAG_METAL != 0,
        vulkan: raw_mask & maplibre_native_sys::MLN_RENDER_BACKEND_FLAG_VULKAN != 0,
        opengl: raw_mask & maplibre_native_sys::MLN_RENDER_BACKEND_FLAG_OPENGL != 0,
    }
}

#[napi(js_name = "supportedOpenGLContextProviders")]
pub fn supported_opengl_context_providers() -> OpenGLContextProviders {
    // SAFETY: mln_opengl_supported_context_provider_mask takes no arguments and
    // returns a value mask. Unknown future bits are preserved in raw_mask.
    let raw_mask = unsafe { maplibre_native_sys::mln_opengl_supported_context_provider_mask() };
    OpenGLContextProviders {
        raw_mask,
        wgl: raw_mask & maplibre_native_sys::MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL != 0,
        egl: raw_mask & maplibre_native_sys::MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL != 0,
    }
}

#[napi(js_name = "networkStatus")]
pub fn network_status() -> Result<NetworkStatusValue> {
    let mut raw_status = 0;
    // SAFETY: raw_status points to valid writable storage for one u32.
    maplibre_native_core::check(unsafe {
        maplibre_native_sys::mln_network_status_get(&mut raw_status)
    })
    .map_err(error::from_core)?;
    Ok(network_status_from_raw(raw_status))
}

#[napi(js_name = "setNetworkStatus")]
pub fn set_network_status(status: String) -> Result<()> {
    let raw_status = match status.as_str() {
        "online" => maplibre_native_sys::MLN_NETWORK_STATUS_ONLINE,
        "offline" => maplibre_native_sys::MLN_NETWORK_STATUS_OFFLINE,
        other => {
            return Err(error::invalid_argument(format!(
                "network status must be 'online' or 'offline', got '{other}'"
            )));
        }
    };

    // SAFETY: The raw enum value is passed by value. The C API validates the
    // enum domain and reports invalid values as MLN_STATUS_INVALID_ARGUMENT.
    maplibre_native_core::check(unsafe { maplibre_native_sys::mln_network_status_set(raw_status) })
        .map_err(error::from_core)
}

#[napi(js_name = "nativeSetLogCallback")]
pub fn native_set_log_callback(env: Env, callback: ThreadsafeFunction<LogRecord>) -> Result<()> {
    let state = Arc::new(LogCallbackState {
        id: LOG_CALLBACK_IDS.fetch_add(1, Ordering::Relaxed),
        callback: Arc::new(callback),
    });
    env.add_env_cleanup_hook(state.id, clear_log_callback_if_current)?;
    let user_data = Arc::as_ptr(&state).cast_mut().cast::<c_void>();
    let previous = {
        let mut slot = log_callback_slot()
            .lock()
            .map_err(|_| error::invalid_argument("log callback state lock is poisoned"))?;
        maplibre_native_core::check(unsafe {
            maplibre_native_sys::mln_log_set_callback(Some(log_trampoline), user_data)
        })
        .map_err(error::from_core)?;
        slot.replace(state)
    };
    drop(previous);
    Ok(())
}

#[napi(js_name = "nativeClearLogCallback")]
pub fn native_clear_log_callback() -> Result<()> {
    let previous = {
        let mut slot = log_callback_slot()
            .lock()
            .map_err(|_| error::invalid_argument("log callback state lock is poisoned"))?;
        clear_native_log_callback()?;
        slot.take()
    };
    drop(previous);
    Ok(())
}

#[napi(js_name = "nativeSetAsyncLogSeverityMask")]
pub fn native_set_async_log_severity_mask(mask: u32) -> Result<()> {
    maplibre_native_core::check(unsafe {
        maplibre_native_sys::mln_log_set_async_severity_mask(mask)
    })
    .map_err(error::from_core)
}

#[napi(js_name = "nativeDefaultAsyncLogSeverityMask")]
pub fn native_default_async_log_severity_mask() -> u32 {
    maplibre_native_sys::MLN_LOG_SEVERITY_MASK_DEFAULT
}

#[napi(js_name = "nativeTakeLeakReports")]
pub fn native_take_leak_reports() -> Result<Vec<NativeLeakReport>> {
    let reports = native_leak_reports()
        .lock()
        .map_err(|_| error::invalid_argument("native leak report lock is poisoned"))?
        .drain(..)
        .map(|report| NativeLeakReport {
            handle_type: report.handle_type,
            address: BigInt::from(report.address as u64),
        })
        .collect();
    Ok(reports)
}

#[napi(js_name = "nativeLogSeverityMaskBit")]
pub fn native_log_severity_mask_bit(severity: String) -> Result<u32> {
    match severity.as_str() {
        "info" => Ok(maplibre_native_sys::MLN_LOG_SEVERITY_MASK_INFO),
        "warning" => Ok(maplibre_native_sys::MLN_LOG_SEVERITY_MASK_WARNING),
        "error" => Ok(maplibre_native_sys::MLN_LOG_SEVERITY_MASK_ERROR),
        other => Err(error::invalid_argument(format!(
            "log severity must be 'info', 'warning', or 'error', got '{other}'"
        ))),
    }
}

extern "C" fn log_trampoline(
    user_data: *mut c_void,
    severity: u32,
    event: u32,
    code: i64,
    message: *const c_char,
) -> u32 {
    if user_data.is_null() {
        return 0;
    }

    let _ = catch_unwind(AssertUnwindSafe(|| {
        let state_ptr = user_data.cast::<LogCallbackState>();
        unsafe { Arc::increment_strong_count(state_ptr) };
        let state = unsafe { Arc::from_raw(state_ptr) };
        state.callback.call(
            Ok(LogRecord {
                severity: log_severity_name(severity).to_owned(),
                raw_severity: severity,
                event: log_event_name(event).to_owned(),
                raw_event: event,
                code,
                message: copy_log_message(message),
            }),
            ThreadsafeFunctionCallMode::NonBlocking,
        );
    }));
    0
}

pub(crate) fn report_native_handle_leak(handle_type: &str, address: usize) {
    if let Ok(mut reports) = native_leak_reports().lock() {
        reports.push(NativeLeakReportEntry {
            handle_type: handle_type.to_owned(),
            address,
        });
    }
}

fn native_leak_reports() -> &'static Mutex<Vec<NativeLeakReportEntry>> {
    NATIVE_LEAK_REPORTS.get_or_init(|| Mutex::new(Vec::new()))
}

fn log_callback_slot() -> &'static Mutex<Option<Arc<LogCallbackState>>> {
    LOG_CALLBACK.get_or_init(|| Mutex::new(None))
}

fn clear_log_callback_if_current(id: u64) {
    let Ok(mut slot) = log_callback_slot().lock() else {
        return;
    };
    if slot.as_ref().is_some_and(|state| state.id == id) && clear_native_log_callback().is_ok() {
        drop(slot.take());
    }
}

fn clear_native_log_callback() -> Result<()> {
    maplibre_native_core::check(unsafe { maplibre_native_sys::mln_log_clear_callback() })
        .map_err(error::from_core)
}

fn copy_log_message(message: *const c_char) -> String {
    if message.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(message) }
            .to_string_lossy()
            .into_owned()
    }
}

fn log_severity_name(severity: u32) -> &'static str {
    match severity {
        maplibre_native_sys::MLN_LOG_SEVERITY_INFO => "info",
        maplibre_native_sys::MLN_LOG_SEVERITY_WARNING => "warning",
        maplibre_native_sys::MLN_LOG_SEVERITY_ERROR => "error",
        _ => "unknown",
    }
}

fn log_event_name(event: u32) -> &'static str {
    match event {
        maplibre_native_sys::MLN_LOG_EVENT_GENERAL => "general",
        maplibre_native_sys::MLN_LOG_EVENT_SETUP => "setup",
        maplibre_native_sys::MLN_LOG_EVENT_SHADER => "shader",
        maplibre_native_sys::MLN_LOG_EVENT_PARSE_STYLE => "parseStyle",
        maplibre_native_sys::MLN_LOG_EVENT_PARSE_TILE => "parseTile",
        maplibre_native_sys::MLN_LOG_EVENT_RENDER => "render",
        maplibre_native_sys::MLN_LOG_EVENT_STYLE => "style",
        maplibre_native_sys::MLN_LOG_EVENT_DATABASE => "database",
        maplibre_native_sys::MLN_LOG_EVENT_HTTP_REQUEST => "httpRequest",
        maplibre_native_sys::MLN_LOG_EVENT_SPRITE => "sprite",
        maplibre_native_sys::MLN_LOG_EVENT_IMAGE => "image",
        maplibre_native_sys::MLN_LOG_EVENT_OPENGL => "openGl",
        maplibre_native_sys::MLN_LOG_EVENT_JNI => "jni",
        maplibre_native_sys::MLN_LOG_EVENT_ANDROID => "android",
        maplibre_native_sys::MLN_LOG_EVENT_CRASH => "crash",
        maplibre_native_sys::MLN_LOG_EVENT_GLYPH => "glyph",
        maplibre_native_sys::MLN_LOG_EVENT_TIMING => "timing",
        _ => "unknown",
    }
}

fn network_status_from_raw(raw: u32) -> NetworkStatusValue {
    let kind = match raw {
        maplibre_native_sys::MLN_NETWORK_STATUS_ONLINE => "online".to_owned(),
        maplibre_native_sys::MLN_NETWORK_STATUS_OFFLINE => "offline".to_owned(),
        _ => "unknown".to_owned(),
    };
    NetworkStatusValue { kind, raw }
}
