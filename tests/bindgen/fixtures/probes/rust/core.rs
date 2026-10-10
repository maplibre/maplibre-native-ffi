//! The core runtime as the probe builds it.
#![deny(unsafe_op_in_unsafe_fn)]
pub mod abi;
pub mod callback;
pub mod decision;
pub mod error;
pub mod handle;
pub mod ptr;
pub mod string;
pub use abi::{EXPECTED_C_ABI_VERSION, validate_abi_version};
pub use error::{Error, ErrorKind, Result, check};
