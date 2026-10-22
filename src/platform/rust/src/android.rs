use std::ffi::CString;
use std::os::raw::{c_char, c_void};

#[cfg(target_os = "android")]
use std::io::Write;
#[cfg(target_os = "android")]
use std::os::unix::fs::{OpenOptionsExt, PermissionsExt};
#[cfg(target_os = "android")]
use std::path::{Path, PathBuf};
#[cfg(target_os = "android")]
use std::sync::Mutex;
#[cfg(target_os = "android")]
use std::sync::atomic::{AtomicBool, Ordering};

#[cfg(target_os = "android")]
use jni::JNIEnv;
#[cfg(target_os = "android")]
use jni::objects::{JObject, JString};

/// The verifier's Kotlin helper and the parts of the Kotlin standard library that
/// it uses, which cmake/mln_ffi_android_tls_helper.cmake compiles into one dex.
#[cfg(target_os = "android")]
static TLS_HELPER_DEX: &[u8] = include_bytes!(env!("MLN_FFI_ANDROID_TLS_HELPER_DEX"));

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
pub unsafe extern "C" fn mlnffi_rust_android_init_tls_verifier(
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
pub unsafe extern "C" fn mlnffi_rust_android_init_tls_verifier(
    _jni_env: *mut c_void,
    _context: *mut c_void,
) -> *mut c_char {
    error_ptr("Android TLS verifier initialization is not supported")
}

#[cfg(target_os = "android")]
fn init_tls_verifier(jni_env: *mut c_void, context: *mut c_void) -> Result<(), String> {
    let mut env = unsafe { JNIEnv::from_raw(jni_env as *mut jni::sys::JNIEnv) }
        .map_err(|error| error.to_string())?;
    let context = unsafe { JObject::from_raw(context as jni::sys::jobject) };
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

    let loader = tls_helper_loader(&mut env, &context)?;
    let java_vm = env.get_java_vm().map_err(|error| error.to_string())?;
    let context = env
        .new_global_ref(&context)
        .map_err(|error| error.to_string())?;
    let loader = env
        .new_global_ref(&loader)
        .map_err(|error| error.to_string())?;
    rustls_platform_verifier::android::init_with_refs(java_vm, context, loader);
    Ok(())
}

/// Loads the TLS helper in a class loader of its own, so the host packages no
/// Java code for it. The loader delegates to the application's for the platform
/// classes, and the helper's own classes are named so as not to collide.
#[cfg(target_os = "android")]
fn tls_helper_loader<'local>(
    env: &mut JNIEnv<'local>,
    context: &JObject,
) -> Result<JObject<'local>, String> {
    let jni_error = |error: jni::errors::Error| error.to_string();
    let cache_dir = env
        .call_method(context, "getCodeCacheDir", "()Ljava/io/File;", &[])
        .and_then(|value| value.l())
        .map_err(jni_error)?;
    let cache_dir = env
        .call_method(&cache_dir, "getAbsolutePath", "()Ljava/lang/String;", &[])
        .and_then(|value| value.l())
        .map_err(jni_error)?;
    let cache_dir: String = env
        .get_string(&JString::from(cache_dir))
        .map_err(jni_error)?
        .into();
    let dex = write_tls_helper_dex(Path::new(&cache_dir))
        .map_err(|error| format!("cannot write the TLS helper dex: {error}"))?;

    let parent = env
        .call_method(context, "getClassLoader", "()Ljava/lang/ClassLoader;", &[])
        .and_then(|value| value.l())
        .map_err(jni_error)?;
    let dex = env.new_string(dex.to_string_lossy()).map_err(jni_error)?;
    // API levels before 26 require a directory for optimized code, and later
    // ones ignore it.
    let optimized_dir = env.new_string(&cache_dir).map_err(jni_error)?;
    env.new_object(
        "dalvik/system/DexClassLoader",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V",
        &[
            (&dex).into(),
            (&optimized_dir).into(),
            (&JObject::null()).into(),
            (&parent).into(),
        ],
    )
    .map_err(jni_error)
}

/// Writes the TLS helper dex into the application's code cache, which the
/// platform clears when the application or the system updates.
#[cfg(target_os = "android")]
fn write_tls_helper_dex(cache_dir: &Path) -> std::io::Result<PathBuf> {
    // A dex header holds the SHA-1 of the rest of the file at bytes 12 to 32, so
    // naming the file by it never loads a dex that another version wrote.
    let signature: String = TLS_HELPER_DEX[12..32]
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect();
    let path = cache_dir.join(format!("maplibre-native-ffi-tls-{signature}.dex"));
    // Another process that runs as the application may have written the path,
    // so a cached file counts only when it holds this dex and is read-only.
    // Otherwise the rename below replaces it.
    if let (Ok(cached), Ok(metadata)) = (std::fs::read(&path), std::fs::metadata(&path))
        && cached == TLS_HELPER_DEX
        && metadata.permissions().mode() & 0o222 == 0
    {
        return Ok(path);
    }

    // Each process writes a file of its own and renames it into place, so a
    // concurrent process never loads a partial dex. Android 14 refuses to load a
    // dex that is still writable, and the file is read-only from its creation, so
    // only the handle that created it can write the code.
    let temporary = cache_dir.join(format!(
        "maplibre-native-ffi-tls-{signature}.{}.tmp",
        std::process::id()
    ));
    // A process that exited mid-write may have left a file under this PID.
    let _ = std::fs::remove_file(&temporary);
    let result = std::fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .mode(0o444)
        .open(&temporary)
        .and_then(|mut file| file.write_all(TLS_HELPER_DEX))
        .and_then(|()| std::fs::rename(&temporary, &path));
    if result.is_err() {
        let _ = std::fs::remove_file(&temporary);
    }
    result.map(|()| path)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_android_error_free(error: *mut c_char) {
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
