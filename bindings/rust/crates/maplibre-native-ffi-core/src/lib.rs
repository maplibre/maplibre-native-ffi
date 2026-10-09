//! Shared error, ownership, callback, and memory-safety runtime for the Rust
//! and Python bindings.
#![deny(unsafe_op_in_unsafe_fn)]
pub mod abi;
pub mod callback;
pub mod error;
pub mod handle;
pub mod ptr;
pub mod resource;
pub mod string;
#[cfg(feature = "abi-version-override")]
pub use abi::set_abi_version_override;
pub use abi::{EXPECTED_C_ABI_VERSION, validate_abi_version};
pub use error::{Error, ErrorKind, Result, check};
