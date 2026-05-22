//! JNI bridge entry points for the Java JNI binding.
//!
//! This crate owns JNI registration and delegates shared ABI adaptation to the
//! Rust binding crates.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};

use jni::objects::JClass;
use jni::sys::{JNI_VERSION_1_8, jint, jlong};
use jni::{JNIEnv, JavaVM, NativeMethod};

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
    let methods = [NativeMethod {
        name: "cVersion".into(),
        sig: "()J".into(),
        fn_ptr: c_version as *mut c_void,
    }];
    env.register_native_methods(class, &methods)
}

extern "system" fn c_version(_env: JNIEnv<'_>, _class: JClass<'_>) -> jlong {
    catch_unwind(|| unsafe { maplibre_native_sys::mln_c_version() as jlong }).unwrap_or(0)
}
