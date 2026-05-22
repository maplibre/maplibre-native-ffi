//! JNI bridge entry points for the Java JNI binding.
//!
//! This crate owns JNI registration and delegates shared ABI adaptation to the
//! Rust binding crates.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};

use jni::objects::{JClass, JIntArray, JObject};
use jni::sys::{JNI_VERSION_1_8, jint, jlong, jstring};
use jni::{JNIEnv, JavaVM, NativeMethod};
use maplibre_native_core::error::capture_thread_diagnostic;
use maplibre_native_sys as sys;

const BRIDGE_CLASS: &str = "org/maplibre/nativejni/internal/bridge/NativeBridge";

#[unsafe(no_mangle)]
pub extern "system" fn JNI_OnLoad(vm: JavaVM, _reserved: *mut c_void) -> jint {
    match catch_unwind(AssertUnwindSafe(|| register_natives(&vm))) {
        Ok(Ok(())) => JNI_VERSION_1_8,
        _ => 0,
    }
}

fn register_natives(vm: &JavaVM) -> jni::errors::Result<()> {
    let mut env = vm.get_env()?;
    let class = env.find_class(BRIDGE_CLASS)?;
    let methods = [
        NativeMethod {
            name: "cVersion".into(),
            sig: "()J".into(),
            fn_ptr: c_version as *mut c_void,
        },
        NativeMethod {
            name: "supportedRenderBackendMask".into(),
            sig: "()I".into(),
            fn_ptr: supported_render_backend_mask as *mut c_void,
        },
        NativeMethod {
            name: "networkStatusGet".into(),
            sig: "([I)I".into(),
            fn_ptr: network_status_get as *mut c_void,
        },
        NativeMethod {
            name: "networkStatusSet".into(),
            sig: "(I)I".into(),
            fn_ptr: network_status_set as *mut c_void,
        },
        NativeMethod {
            name: "threadLastErrorMessage".into(),
            sig: "()Ljava/lang/String;".into(),
            fn_ptr: thread_last_error_message as *mut c_void,
        },
    ];
    env.register_native_methods(class, &methods)
}

extern "system" fn c_version(_env: JNIEnv<'_>, _class: JClass<'_>) -> jlong {
    catch_unwind(|| unsafe { sys::mln_c_version() as jlong }).unwrap_or(0)
}

extern "system" fn supported_render_backend_mask(_env: JNIEnv<'_>, _class: JClass<'_>) -> jint {
    catch_unwind(|| unsafe { sys::mln_supported_render_backend_mask() as jint }).unwrap_or(0)
}

extern "system" fn network_status_get(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    out_status: JIntArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_status.is_null() || env.get_array_length(&out_status).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        let mut status: sys::mln_network_status = 0;
        let result = unsafe { sys::mln_network_status_get(&mut status) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_int_array_region(&out_status, 0, &[status as jint])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn network_status_set(_env: JNIEnv<'_>, _class: JClass<'_>, status: jint) -> jint {
    catch_unwind(|| unsafe { sys::mln_network_status_set(status as sys::mln_network_status) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn thread_last_error_message(env: JNIEnv<'_>, _class: JClass<'_>) -> jstring {
    catch_unwind(AssertUnwindSafe(|| {
        let diagnostic = capture_thread_diagnostic();
        match env.new_string(diagnostic) {
            Ok(message) => message.into_raw(),
            Err(_) => JObject::null().into_raw(),
        }
    }))
    .unwrap_or_else(|_| JObject::null().into_raw())
}
