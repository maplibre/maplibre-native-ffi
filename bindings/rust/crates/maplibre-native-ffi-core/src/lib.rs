//! Shared error, ownership, callback, and memory-safety runtime for generated bindings.
#![deny(unsafe_op_in_unsafe_fn)]
pub mod abi;
pub mod callback;
pub mod error;
#[allow(clippy::all, unused_parens, unused_variables, unused_unsafe)]
pub mod generated;
pub mod handle;
pub mod input;
pub mod ptr;
pub mod resource;
pub mod string;
pub mod values;
#[cfg(feature = "abi-version-override")]
pub use abi::set_abi_version_override;
pub use abi::{EXPECTED_C_ABI_VERSION, validate_abi_version, validate_abi_version_value};
pub use error::{Error, ErrorKind, Result, check};
pub use generated::*;
