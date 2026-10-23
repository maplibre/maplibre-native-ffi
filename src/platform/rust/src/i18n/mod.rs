//! Locale-sensitive style expressions on the platforms whose C++ side takes its
//! locale services from here: Linux, from ICU4X, and Windows, from the ICU that
//! ships with the system.

use std::cmp::Ordering;
use std::ffi::c_void;
use std::{ptr, slice};

#[cfg(target_os = "linux")]
mod icu4x;
#[cfg(windows)]
mod windows;

#[cfg(target_os = "linux")]
use icu4x as backend;
#[cfg(windows)]
use windows as backend;

use backend::Collator;

/// Text that Rust allocates and returns to C++, which passes it back to
/// `mlnffi_rust_text_free`. When `error` is set, the text is the message.
#[repr(C)]
pub struct MlnRustText {
    data: *mut u8,
    length: usize,
    error: bool,
}

impl From<Result<String, String>> for MlnRustText {
    fn from(result: Result<String, String>) -> Self {
        let error = result.is_err();
        let text = result.unwrap_or_else(|message| message).into_boxed_str();
        let length = text.len();
        Self {
            data: Box::into_raw(text).cast(),
            length,
            error,
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_text_free(text: MlnRustText) {
    // SAFETY: The text came from `MlnRustText::from` with its original length.
    drop(unsafe { Box::from_raw(ptr::slice_from_raw_parts_mut(text.data, text.length)) });
}

/// # Safety
///
/// `data` must point to `length` readable bytes that outlive the result.
unsafe fn bytes<'a>(data: *const u8, length: usize) -> &'a [u8] {
    // SAFETY: The caller upholds this function's contract.
    unsafe { slice::from_raw_parts(data, length) }
}

/// The uppercase form of a currency code that has the shape of an ISO 4217
/// code: three ASCII letters.
fn currency_code(currency: &[u8]) -> Result<String, String> {
    str::from_utf8(currency)
        .ok()
        .filter(|code| code.len() == 3 && code.bytes().all(|byte| byte.is_ascii_alphabetic()))
        .map(str::to_ascii_uppercase)
        .ok_or_else(|| "Currency is not an ISO 4217 code".to_owned())
}

/// Creates a collator for a language tag, or the platform's collation locale
/// for an empty tag, and stores it in `collator`. Returns the locale the
/// collator reports.
///
/// # Safety
///
/// `locale` must point to `length` readable bytes, and `collator` must be
/// writable. On success, pass the stored collator to
/// `mlnffi_rust_collator_free`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_collator_new(
    locale: *const u8,
    length: usize,
    case_sensitive: bool,
    diacritic_sensitive: bool,
    collator: *mut *mut c_void,
) -> MlnRustText {
    // SAFETY: The caller upholds this function's contract.
    let requested = unsafe { bytes(locale, length) };
    backend::new_collator(requested, case_sensitive, diacritic_sensitive)
        .map(|(created, resolved)| {
            // SAFETY: The caller upholds this function's contract.
            unsafe { *collator = Box::into_raw(Box::new(created)).cast() };
            resolved
        })
        .into()
}

/// # Safety
///
/// `collator` must come from `mlnffi_rust_collator_new` and not be used again.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_collator_free(collator: *mut c_void) {
    // SAFETY: The caller upholds this function's contract.
    drop(unsafe { Box::from_raw(collator.cast::<Collator>()) });
}

/// Compares two UTF-8 strings, returning a negative, zero, or positive result.
/// Ill-formed UTF-8 compares as U+FFFD.
///
/// # Safety
///
/// `collator` must come from `mlnffi_rust_collator_new`, and each string must
/// point to its length in readable bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_collator_compare(
    collator: *const c_void,
    lhs: *const u8,
    lhs_length: usize,
    rhs: *const u8,
    rhs_length: usize,
) -> i32 {
    // SAFETY: The caller upholds this function's contract.
    let (collator, lhs, rhs) = unsafe {
        (
            &*collator.cast::<Collator>(),
            bytes(lhs, lhs_length),
            bytes(rhs, rhs_length),
        )
    };
    match backend::compare(collator, lhs, rhs) {
        Ordering::Less => -1,
        Ordering::Equal => 0,
        Ordering::Greater => 1,
    }
}

/// Formats a number for a language tag, or the platform's numeric locale for
/// an empty tag, as a decimal or, with a currency code, as currency. A negative
/// fraction limit is omitted.
///
/// # Safety
///
/// `locale` and `currency` must each point to their length in readable bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_format_number(
    number: f64,
    locale: *const u8,
    locale_length: usize,
    currency: *const u8,
    currency_length: usize,
    min_fraction_digits: i16,
    max_fraction_digits: i16,
) -> MlnRustText {
    // SAFETY: The caller upholds this function's contract.
    let (locale, currency) = unsafe {
        (
            bytes(locale, locale_length),
            bytes(currency, currency_length),
        )
    };
    backend::format_number(
        number,
        locale,
        currency,
        u8::try_from(min_fraction_digits).ok(),
        u8::try_from(max_fraction_digits).ok(),
    )
    .into()
}
