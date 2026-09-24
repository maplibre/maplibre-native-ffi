// Platform types required by gl_generator's EGL bindings.
#![allow(non_camel_case_types, dead_code, unsafe_op_in_unsafe_fn)]
#![allow(clippy::all)]

pub type khronos_utime_nanoseconds_t = u64;
pub type khronos_uint64_t = u64;
pub type khronos_ssize_t = std::ffi::c_long;
pub type EGLint = i32;
pub type EGLNativeDisplayType = *const std::ffi::c_void;
pub type EGLNativePixmapType = *const std::ffi::c_void;
pub type EGLNativeWindowType = *const std::ffi::c_void;
pub type NativeDisplayType = EGLNativeDisplayType;
pub type NativePixmapType = EGLNativePixmapType;
pub type NativeWindowType = EGLNativeWindowType;

include!(concat!(env!("OUT_DIR"), "/egl.rs"));
