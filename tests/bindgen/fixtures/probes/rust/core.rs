//! The core runtime as the probe builds it.
#![deny(unsafe_op_in_unsafe_fn)]
pub mod abi;
pub mod callback;
pub mod error;
pub mod handle;
pub mod memory;
pub mod ptr;
pub mod report;
pub mod string;
pub use abi::{EXPECTED_C_ABI_VERSION, require_abi_version, validate_abi_version};
pub mod status;
pub use error::{Error, Result, check};
pub use status::Status;
