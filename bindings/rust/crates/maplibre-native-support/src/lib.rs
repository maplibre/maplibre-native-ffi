#![deny(unsafe_op_in_unsafe_fn)]

pub mod abi;
pub mod error;
pub mod handle;
pub mod ptr;
pub mod string;

pub use abi::{EXPECTED_C_ABI_VERSION, validate_abi_version, validate_abi_version_value};
pub use error::{Error, ErrorKind, Result, check};
