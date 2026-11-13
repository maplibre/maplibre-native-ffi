//! Raw declarations for the MapLibre Native C ABI, which the Python extension
//! calls.
//!
//! This crate mirrors the C boundary: constants, layouts, opaque handle types,
//! and unsafe extern functions. It also declares the delivery queue protocol,
//! and the exports of `binding_support.h`.
//!
//! The build script links the native library statically, from the install
//! prefix that `MAPLIBRE_NATIVE_FFI_LOCAL_BUILD` names as an absolute path. The
//! prefix holds `share/maplibre-native-c/artifact.json` and must have been built
//! for the Cargo target.
//!
//! On `x86_64-pc-windows-gnu`, link with MSYS2's MINGW64 Clang and LLD
//! (`-C link-arg=-fuse-ld=lld`). Its MSVCRT and libstdc++ must match the native
//! artifact. Keep MINGW64's `bin` directory on `PATH` for the runtime DLLs.

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
