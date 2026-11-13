//! Raw declarations for the MapLibre Native C ABI.
//!
//! This crate mirrors the C boundary: constants, layouts, opaque handle types,
//! and unsafe extern functions. It also declares the delivery queue protocol,
//! and the exports of `binding_support.h`, which carry no compatibility
//! promise. Prefer the safe `maplibre-native-ffi` crate.
//!
//! # Linking
//!
//! The build script links the native library statically. It takes the library
//! from the install prefix that `MAPLIBRE_NATIVE_FFI_LOCAL_BUILD` names as an
//! absolute path, which holds `share/maplibre-native-c/artifact.json`. Without
//! one, a published crate downloads the release that it was published against,
//! from the base URL in `MAPLIBRE_NATIVE_FFI_RELEASE_URL` when that is set.
//!
//! The `metal`, `opengl`, `vulkan`, and `webgpu` features select the render
//! backend, and at most one can be enabled. Without one, a downloaded release
//! takes the default backend its artifact descriptor records: Metal on Apple
//! platforms, OpenGL on Android and OpenHarmony, WebGPU in the browser, and
//! Vulkan elsewhere. A local install prefix serves its own backend unless a
//! feature selects another, and must have been built for the Cargo target; a
//! mismatch is an error. Dependents read the backend from
//! `DEP_MAPLIBRE_NATIVE_C_RENDER_BACKEND` and the install prefix from
//! `DEP_MAPLIBRE_NATIVE_C_PREFIX`.
//!
//! On `x86_64-pc-windows-gnu`, link with MSYS2's MINGW64 Clang and LLD
//! (`-C link-arg=-fuse-ld=lld`). Its MSVCRT and libstdc++ must match the native
//! artifact. Keep MINGW64's `bin` directory on `PATH` for the runtime DLLs.
//!
//! In the browser, the library uses shared memory, so the final module needs a
//! standard library built with atomics: `-C target-feature=+atomics,+bulk-memory`
//! and `-Zbuild-std=std,panic_abort`. It also needs the link options listed in
//! `share/maplibre-native-c/emscripten-link-flags.txt` under the install prefix,
//! passed through `-C link-arg`.

#![allow(non_camel_case_types)]

mod generated;

pub use generated::*;

/// A C handle type: a transparent newtype over the 64-bit id the C API issues.
pub trait NativeHandle: Copy + 'static {
    fn to_raw(self) -> u64;
    fn from_raw(raw: u64) -> Self;
}

/// The receiver of a call that has none, which admits no handle.
impl NativeHandle for () {
    fn to_raw(self) -> u64 {
        0
    }

    fn from_raw(_: u64) -> Self {}
}

macro_rules! native_handles {
    ($($handle:ident),* $(,)?) => {$(
        impl $crate::NativeHandle for $handle {
            fn to_raw(self) -> u64 {
                self.0
            }

            fn from_raw(raw: u64) -> Self {
                Self(raw)
            }
        }
    )*};
}
use native_handles;

#[cfg(test)]
mod tests {
    /// Calls into the native library so the test binary links and loads it.
    /// Nothing else in this crate references a symbol, so without this the
    /// linker drops the dependency and an unusable install prefix still builds
    /// clean.
    #[test]
    fn loads_the_native_library() {
        // SAFETY: mln_supported_render_backend_mask takes no arguments and
        // returns a process-global constant.
        assert_ne!(unsafe { super::mln_supported_render_backend_mask() }, 0);
    }

    /// The delivery protocol links, which nothing above this crate calls yet.
    #[test]
    fn links_delivery_queue() {
        let mut queue = super::mln_delivery_queue(0);
        // SAFETY: null options ask for the defaults, `queue` holds the null
        // handle, and a null diagnostic asks for none.
        let status = unsafe {
            super::mln_delivery_queue_create(std::ptr::null(), &mut queue, std::ptr::null_mut())
        };
        assert_eq!(status, super::MLN_STATUS_OK);
        // SAFETY: the queue is live and released once.
        unsafe { super::mln_delivery_queue_release(queue) };
    }
}
