//! Integration tests of what this binding adds on top of the C API: handle
//! lifecycles, completions, callbacks, generated value shapes, and rendering.
//!
//! Native semantics are tested once, in tests/native. A test here calls native
//! only to reach a binding code path, and it asserts what the binding does.
//! tests/conformance/rust.toml maps each conformance case to its test.
//!
//! The browser build runs its own few tests, in `browser`, because its GPU
//! contexts come from JavaScript rather than tests/graphics.

// The browser build runs a few of the tests, and so uses part of the support.
#[cfg_attr(target_os = "emscripten", allow(dead_code))]
mod support;

#[cfg(not(target_os = "emscripten"))]
mod callbacks;
#[cfg(not(target_os = "emscripten"))]
mod lifecycle;
#[cfg(not(target_os = "emscripten"))]
mod render;
#[cfg(not(target_os = "emscripten"))]
mod values;

#[cfg(target_os = "emscripten")]
mod browser;
