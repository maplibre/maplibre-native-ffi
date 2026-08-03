//! The Node-API transport for the TypeScript binding.
//!
//! This addon is the whole native boundary: it reports the ABI handshake, turns
//! JavaScript-owned `ArrayBuffer` slabs into addresses the C API can use, calls
//! entrypoints through the shared normalized dispatch, and copies bytes out of
//! library-owned memory. Everything above it — struct encoding, handle state,
//! error mapping, the public API — is TypeScript shared with the WebAssembly
//! transport.
//!
//! Addresses cross this boundary as `bigint`. A JavaScript `number` cannot carry
//! one: Android tags heap pointers in the top byte, so a pointer converted
//! through a double loses bits the allocator requires.

#![deny(clippy::all)]

use std::ffi::CStr;
use std::os::raw::c_char;

use napi::bindgen_prelude::*;
use napi_derive::napi;

unsafe extern "C" {
    fn mln_abi_fingerprint() -> *const c_char;
    fn mln_abi_header_digest() -> *const c_char;
    fn mln_abi_entrypoint_count() -> u32;
    fn mln_abi_entrypoint_name(entrypoint: u32) -> *const c_char;
    fn mln_abi_call(
        entrypoint: u32,
        slots: *mut std::ffi::c_void,
        diagnostic: *mut c_char,
        diagnostic_capacity: u32,
        diagnostic_length: *mut u32,
    ) -> i32;
    fn mln_abi_symbol(entrypoint: u32) -> *mut std::ffi::c_void;
    fn mln_abi_transfer_issue(handle: u64) -> u64;
    fn mln_abi_transfer_claim(token: u64) -> u64;
    fn mln_abi_transfer_discard(token: u64) -> u64;
}

fn c_string(pointer: *const c_char) -> Option<String> {
    if pointer.is_null() {
        return None;
    }
    Some(
        unsafe { CStr::from_ptr(pointer) }
            .to_string_lossy()
            .into_owned(),
    )
}

/// Reads an address out of a JavaScript `BigInt`.
///
/// A value that does not fit an unsigned 64-bit integer is a caller error rather
/// than something to truncate, because the result would name another address.
fn address(value: BigInt, what: &str) -> Result<u64> {
    let (signed, words, lossless) = (value.sign_bit, value.words, true);
    if signed || words.len() > 1 && words[1..].iter().any(|word| *word != 0) || !lossless {
        return Err(Error::new(
            Status::InvalidArg,
            format!("{what} is not an unsigned 64-bit address"),
        ));
    }
    Ok(words.first().copied().unwrap_or(0))
}

/// Reports the ABI schema fingerprint this payload's dispatch was built from.
#[napi]
pub fn abi_fingerprint() -> String {
    c_string(unsafe { mln_abi_fingerprint() }).unwrap_or_default()
}

/// Reports the digest of the public headers this payload was built against.
#[napi]
pub fn abi_header_digest() -> String {
    c_string(unsafe { mln_abi_header_digest() }).unwrap_or_default()
}

/// Reports how many entrypoints this payload dispatches.
#[napi]
pub fn entrypoint_count() -> u32 {
    unsafe { mln_abi_entrypoint_count() }
}

/// Names an entrypoint, so a failure can say which call it came from.
#[napi]
pub fn entrypoint_name(entrypoint: u32) -> Option<String> {
    c_string(unsafe { mln_abi_entrypoint_name(entrypoint) })
}

/// Reports the address of a slab's backing store.
///
/// The caller keeps the `ArrayBuffer` reachable for as long as the address is in
/// use. Backing stores do not move, so the address stays valid for the buffer's
/// life.
#[napi]
pub fn register_slab(buffer: ArrayBuffer) -> BigInt {
    BigInt::from(buffer.as_ref().as_ptr() as usize as u64)
}

/// Calls one entrypoint through the shared normalized dispatch.
///
/// `slots` and `diagnostic` are addresses inside slabs the caller owns. The
/// diagnostic is copied inside the failing call, so nothing the host does
/// afterwards can replace it.
#[napi]
pub fn call(
    entrypoint: u32,
    slots: BigInt,
    diagnostic: BigInt,
    diagnostic_capacity: u32,
    diagnostic_length: BigInt,
) -> Result<i32> {
    let slots = address(slots, "slots")? as usize as *mut std::ffi::c_void;
    let diagnostic = address(diagnostic, "diagnostic")? as usize as *mut c_char;
    let diagnostic_length = address(diagnostic_length, "diagnosticLength")? as usize as *mut u32;
    Ok(unsafe {
        mln_abi_call(
            entrypoint,
            slots,
            diagnostic,
            diagnostic_capacity,
            diagnostic_length,
        )
    })
}

/// Reports the address of an entrypoint, for a struct field the C API reads as
/// a callback.
#[napi]
pub fn symbol(entrypoint: u32) -> BigInt {
    BigInt::from(unsafe { mln_abi_symbol(entrypoint) } as usize as u64)
}

/// Copies bytes out of library-owned memory.
///
/// Library allocations sit outside every slab, so the shared layer cannot read
/// them through a view. This is the copy step the binding specification requires
/// before a borrowed native pointer's window ends.
#[napi]
pub fn read_foreign(pointer: BigInt, length: u32) -> Result<Uint8Array> {
    let pointer = address(pointer, "pointer")?;
    if pointer == 0 {
        return Err(Error::new(
            Status::InvalidArg,
            "cannot read from a null pointer".to_owned(),
        ));
    }
    let bytes =
        unsafe { std::slice::from_raw_parts(pointer as usize as *const u8, length as usize) };
    Ok(Uint8Array::new(bytes.to_vec()))
}

/// Copies a null-terminated library-owned string.
#[napi]
pub fn read_foreign_c_string(pointer: BigInt) -> Result<Option<String>> {
    let pointer = address(pointer, "pointer")?;
    Ok(c_string(pointer as usize as *const c_char))
}

/// Issues a one-shot token naming a handle, for moving it to another context.
#[napi]
pub fn transfer_issue(handle: BigInt) -> Result<BigInt> {
    Ok(BigInt::from(unsafe {
        mln_abi_transfer_issue(address(handle, "handle")?)
    }))
}

/// Claims a transfer token, reporting the handle it named.
#[napi]
pub fn transfer_claim(token: BigInt) -> Result<BigInt> {
    Ok(BigInt::from(unsafe {
        mln_abi_transfer_claim(address(token, "token")?)
    }))
}

/// Discards an unclaimed transfer token.
#[napi]
pub fn transfer_discard(token: BigInt) -> Result<BigInt> {
    Ok(BigInt::from(unsafe {
        mln_abi_transfer_discard(address(token, "token")?)
    }))
}
