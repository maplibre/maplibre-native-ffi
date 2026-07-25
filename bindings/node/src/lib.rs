//! Node.js N-API add-on for the MapLibre Native FFI binding.
//!
//! The public JavaScript and TypeScript package owns Node-specific API policy.
//! Shared C ABI adaptation comes from `maplibre-native-core`; direct `sys` calls
//! in this scaffold are limited to the initial process-global proof slice.

#![deny(unsafe_op_in_unsafe_fn)]

mod error;
mod map;
mod maplibre;
mod projection;
mod render;
mod runtime;
mod values;

pub use map::*;
pub use maplibre::*;
pub use projection::*;
pub use render::*;
pub use runtime::*;
pub use values::*;
