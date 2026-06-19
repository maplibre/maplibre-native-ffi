use std::ffi::CString;
use std::os::raw::{c_char, c_void};

#[cfg(target_os = "android")]
use std::sync::Mutex;
#[cfg(target_os = "android")]
use std::sync::atomic::{AtomicBool, Ordering};

#[cfg(target_os = "android")]
static TLS_VERIFIER_INITIALIZED: AtomicBool = AtomicBool::new(false);
#[cfg(target_os = "android")]
static TLS_VERIFIER_INIT_LOCK: Mutex<()> = Mutex::new(());

#[cfg(target_os = "android")]
pub(crate) fn tls_verifier_initialized() -> bool {
    TLS_VERIFIER_INITIALIZED.load(Ordering::Acquire)
}

#[cfg(not(target_os = "android"))]
pub(crate) fn tls_verifier_initialized() -> bool {
    true
}

#[cfg(target_os = "android")]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_android_init_tls_verifier(
    jni_env: *mut c_void,
    context: *mut c_void,
) -> *mut c_char {
    if tls_verifier_initialized() {
        return std::ptr::null_mut();
    }

    if jni_env.is_null() || context.is_null() {
        return error_ptr("jni_env and context must not be null");
    }

    let _guard = TLS_VERIFIER_INIT_LOCK
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if tls_verifier_initialized() {
        return std::ptr::null_mut();
    }

    match init_tls_verifier(jni_env, context) {
        Ok(()) => {
            TLS_VERIFIER_INITIALIZED.store(true, Ordering::Release);
            std::ptr::null_mut()
        }
        Err(error) => error_ptr(&error),
    }
}

#[cfg(not(target_os = "android"))]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_android_init_tls_verifier(
    _jni_env: *mut c_void,
    _context: *mut c_void,
) -> *mut c_char {
    error_ptr("Android TLS verifier initialization is not supported")
}

#[cfg(target_os = "android")]
fn init_tls_verifier(jni_env: *mut c_void, context: *mut c_void) -> Result<(), String> {
    let mut env = unsafe { jni::JNIEnv::from_raw(jni_env as *mut jni::sys::JNIEnv) }
        .map_err(|error| error.to_string())?;
    let context = unsafe { jni::objects::JObject::from_raw(context as jni::sys::jobject) };
    let application_context = env
        .call_method(
            &context,
            "getApplicationContext",
            "()Landroid/content/Context;",
            &[],
        )
        .and_then(|value| value.l())
        .map_err(|error| error.to_string())?;
    let context = if application_context.is_null() {
        context
    } else {
        application_context
    };

    rustls_platform_verifier::android::init_with_env(&mut env, context)
        .map_err(|error| error.to_string())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_android_error_free(error: *mut c_char) {
    if error.is_null() {
        return;
    }

    unsafe {
        drop(CString::from_raw(error));
    }
}

fn error_ptr(message: &str) -> *mut c_char {
    CString::new(message)
        .unwrap_or_else(|_| {
            CString::new("Android TLS verifier initialization failed")
                .expect("static string has no interior nul")
        })
        .into_raw()
}
