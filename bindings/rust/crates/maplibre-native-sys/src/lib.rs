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
