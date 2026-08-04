//! Generated raw declarations for the MapLibre Native public C ABI.
//!
//! This crate mirrors the C boundary: constants, layouts, opaque handle types,
//! and unsafe extern functions generated from `include/maplibre_native_c.h`.
//! Safety policy and ergonomic adaptation live in crates above this layer.

// The C core calls into the platform library for HTTP and image decoding. In a
// browser build that library reaches the module as an rlib rather than inside
// the installed archive, so that the module carries one copy of `std` instead of
// two. rustc drops an rlib no Rust code names, and the calls come from the C
// side, so this is what keeps it on the link line.
#[cfg(target_os = "emscripten")]
extern crate mln_ffi_platform as _;

mod bindings {
    #![allow(clippy::all)]
    #![allow(non_camel_case_types)]
    #![allow(non_snake_case)]
    #![allow(non_upper_case_globals)]
    #![allow(unsafe_op_in_unsafe_fn)]

    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

pub use bindings::*;

#[cfg(test)]
mod tests {
    /// Calls into the native library so that the test binary links and loads
    /// it. Nothing else in this crate references a symbol, so without this the
    /// linker drops the dependency and an unusable install prefix — a
    /// half-extracted download, a mismatched architecture — still builds
    /// clean. Every artifact compiles exactly one renderer, so the mask is
    /// never empty.
    #[test]
    fn loads_the_native_library() {
        // SAFETY: mln_supported_render_backend_mask takes no arguments and
        // returns a process-global constant.
        assert_ne!(unsafe { super::mln_supported_render_backend_mask() }, 0);
    }
}
