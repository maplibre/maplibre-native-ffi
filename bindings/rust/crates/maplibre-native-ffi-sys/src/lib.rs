//! Generated raw declarations for the MapLibre Native public C ABI.
//!
//! This crate mirrors the C boundary: constants, layouts, opaque handle types,
//! and unsafe extern functions generated from `include/maplibre_native_c.h`.
//! Safety policy and ergonomic adaptation live in crates above this layer.

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
}
