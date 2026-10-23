//! Locale services on Windows, from the ICU that Windows 10 version 1903 and
//! later ship as icu.dll. The library imports from icu.dll alone because the
//! older icuuc.dll and icuin.dll need COM initialized on the calling thread.

use std::cmp::Ordering;
use std::ffi::{CStr, CString, c_char, c_void};
use std::ptr::{self, NonNull};
use std::slice;

use super::currency_code;

type UErrorCode = i32;

const U_ZERO_ERROR: UErrorCode = 0;
const U_BUFFER_OVERFLOW_ERROR: UErrorCode = 15;

const UCOL_CASE_LEVEL: i32 = 3;
const UCOL_NORMALIZATION_MODE: i32 = 4;
const UCOL_STRENGTH: i32 = 5;
const UCOL_PRIMARY: i32 = 0;
const UCOL_SECONDARY: i32 = 1;
const UCOL_TERTIARY: i32 = 2;
const UCOL_OFF: i32 = 16;
const UCOL_ON: i32 = 17;

const UNUM_DECIMAL: i32 = 1;
const UNUM_CURRENCY: i32 = 2;
const UNUM_MAX_FRACTION_DIGITS: i32 = 6;
const UNUM_MIN_FRACTION_DIGITS: i32 = 7;

/// The longest locale name Windows returns, with its terminating NUL.
const LOCALE_NAME_MAX_LENGTH: usize = 85;

#[link(name = "icu", kind = "raw-dylib")]
unsafe extern "C" {
    fn u_errorName(code: UErrorCode) -> *const c_char;
    fn uenum_close(enumeration: *mut c_void);
    fn uenum_next(
        enumeration: *mut c_void,
        length: *mut i32,
        status: *mut UErrorCode,
    ) -> *const c_char;
    fn uloc_addLikelySubtags(
        id: *const c_char,
        maximized: *mut c_char,
        capacity: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_canonicalize(
        id: *const c_char,
        name: *mut c_char,
        capacity: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_forLanguageTag(
        tag: *const c_char,
        id: *mut c_char,
        capacity: i32,
        parsed: *mut i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_getBaseName(
        id: *const c_char,
        name: *mut c_char,
        capacity: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_getKeywordValue(
        id: *const c_char,
        keyword: *const c_char,
        value: *mut c_char,
        capacity: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_getLanguage(
        id: *const c_char,
        language: *mut c_char,
        capacity: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_getScript(
        id: *const c_char,
        script: *mut c_char,
        capacity: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_setKeywordValue(
        keyword: *const c_char,
        value: *const c_char,
        id: *mut c_char,
        capacity: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn uloc_toLanguageTag(
        id: *const c_char,
        tag: *mut c_char,
        capacity: i32,
        strict: i8,
        status: *mut UErrorCode,
    ) -> i32;
    fn ucol_close(collator: *mut c_void);
    fn ucol_getKeywordValuesForLocale(
        key: *const c_char,
        locale: *const c_char,
        commonly_used: i8,
        status: *mut UErrorCode,
    ) -> *mut c_void;
    fn ucol_open(locale: *const c_char, status: *mut UErrorCode) -> *mut c_void;
    fn ucol_setAttribute(
        collator: *mut c_void,
        attribute: i32,
        value: i32,
        status: *mut UErrorCode,
    );
    fn ucol_strcollUTF8(
        collator: *const c_void,
        lhs: *const c_char,
        lhs_length: i32,
        rhs: *const c_char,
        rhs_length: i32,
        status: *mut UErrorCode,
    ) -> i32;
    fn unum_close(formatter: *mut c_void);
    fn unum_formatDouble(
        formatter: *const c_void,
        number: f64,
        result: *mut u16,
        capacity: i32,
        position: *mut c_void,
        status: *mut UErrorCode,
    ) -> i32;
    fn unum_open(
        style: i32,
        pattern: *const u16,
        pattern_length: i32,
        locale: *const c_char,
        parse_error: *mut c_void,
        status: *mut UErrorCode,
    ) -> *mut c_void;
    fn unum_setAttribute(formatter: *mut c_void, attribute: i32, value: i32);
}

#[link(name = "kernel32")]
unsafe extern "system" {
    fn GetUserDefaultLocaleName(name: *mut u16, capacity: i32) -> i32;
}

fn check(status: UErrorCode) -> Result<(), String> {
    if status <= U_ZERO_ERROR {
        return Ok(());
    }
    // SAFETY: ICU names every error code with a static string.
    Err(unsafe { CStr::from_ptr(u_errorName(status)) }
        .to_string_lossy()
        .into_owned())
}

/// An ICU length for a buffer or a string, which is an `int32_t`.
fn icu_length(length: usize) -> i32 {
    i32::try_from(length).unwrap_or(i32::MAX)
}

/// Calls an ICU function that writes a string to a buffer of a capacity and
/// returns its length, growing the buffer when it is too small.
fn read<T: Copy + Default>(
    mut write: impl FnMut(*mut T, i32, &mut UErrorCode) -> i32,
) -> Result<Vec<T>, String> {
    let mut buffer = vec![T::default(); 64];
    loop {
        let mut status = U_ZERO_ERROR;
        let length = write(buffer.as_mut_ptr(), icu_length(buffer.len()), &mut status);
        let length = usize::try_from(length).unwrap_or_default();
        if status == U_BUFFER_OVERFLOW_ERROR && length >= buffer.len() {
            buffer.resize(length + 1, T::default());
            continue;
        }
        check(status)?;
        buffer.truncate(length);
        return Ok(buffer);
    }
}

/// Reads a locale ID, language tag, or subtag, which ICU writes in ASCII.
fn read_ascii(
    mut write: impl FnMut(*mut c_char, i32, &mut UErrorCode) -> i32,
) -> Result<String, String> {
    let text = read(|buffer: *mut u8, capacity, status| write(buffer.cast(), capacity, status))?;
    String::from_utf8(text).map_err(|error| error.to_string())
}

fn c_string(text: &str) -> Result<CString, String> {
    CString::new(text).map_err(|error| error.to_string())
}

/// The canonical ICU locale ID for a well-formed language tag, such as
/// `de_DE@collation=phonebook` for `de-DE-u-co-phonebk`.
fn locale_id(tag: &str) -> Option<String> {
    let tag = c_string(tag).ok()?;
    let mut parsed = 0;
    let id = read_ascii(|id, capacity, status| {
        // SAFETY: The tag is a C string, and `id` has `capacity` writable bytes.
        unsafe { uloc_forLanguageTag(tag.as_ptr(), id, capacity, &mut parsed, status) }
    })
    .ok()?;
    if usize::try_from(parsed).ok()? != tag.as_bytes().len() {
        return None;
    }
    let id = c_string(&id).ok()?;
    read_ascii(|name, capacity, status| {
        // SAFETY: The ID is a C string, and `name` has `capacity` writable bytes.
        unsafe { uloc_canonicalize(id.as_ptr(), name, capacity, status) }
    })
    .ok()
}

/// Whether CLDR knows a locale's language. ICU would fall back to root data
/// for an unknown language, which it shares with supported languages such as
/// English.
fn supported(id: &str) -> bool {
    let Ok(id) = c_string(id) else {
        return false;
    };
    let language = read_ascii(|language, capacity, status| {
        // SAFETY: The ID is a C string, and `language` has `capacity` writable
        // bytes.
        unsafe { uloc_getLanguage(id.as_ptr(), language, capacity, status) }
    });
    let Some(language) = language.ok().filter(|language| !language.is_empty()) else {
        return false;
    };
    let Ok(language) = c_string(&language) else {
        return false;
    };
    let script = read_ascii(|maximized, capacity, status| {
        // SAFETY: The language is a C string, and `maximized` has `capacity`
        // writable bytes.
        unsafe { uloc_addLikelySubtags(language.as_ptr(), maximized, capacity, status) }
    })
    .and_then(|maximized| c_string(&maximized))
    .and_then(|maximized| {
        read_ascii(|script, capacity, status| {
            // SAFETY: The ID is a C string, and `script` has `capacity`
            // writable bytes.
            unsafe { uloc_getScript(maximized.as_ptr(), script, capacity, status) }
        })
    });
    script.is_ok_and(|script| !script.is_empty())
}

/// The ICU locale ID of the user's locale, or `en_US` when CLDR does not know
/// its language. An alternate sort order that CLDR also defines becomes the
/// locale's collation.
fn user_locale() -> String {
    let mut name = [0; LOCALE_NAME_MAX_LENGTH];
    // SAFETY: `name` has `LOCALE_NAME_MAX_LENGTH` writable UTF-16 units.
    let length = unsafe { GetUserDefaultLocaleName(name.as_mut_ptr(), icu_length(name.len())) };
    // The length counts the terminating NUL, and 0 means failure.
    let length = usize::try_from(length)
        .unwrap_or_default()
        .saturating_sub(1);
    let name = String::from_utf16_lossy(&name[..length]);
    // An alternate sort order follows an underscore, as in `de-DE_phoneb`.
    let (tag, sort) = name.split_once('_').unwrap_or((&name, ""));
    let collation = match sort {
        "phoneb" => "-u-co-phonebk",
        "pronun" => "-u-co-zhuyin",
        "radstr" => "-u-co-unihan",
        "stroke" => "-u-co-stroke",
        "tradnl" => "-u-co-trad",
        _ => "",
    };
    locale_id(&format!("{tag}{collation}"))
        .filter(|id| supported(id))
        .unwrap_or_else(|| "en_US".to_owned())
}

/// Resolves a requested language tag to an ICU locale ID. An empty or
/// unsupported tag takes the user's locale with the requested Unicode
/// extensions, which replace any of the user's.
fn resolve_locale(requested: &[u8]) -> String {
    let id = str::from_utf8(requested)
        .ok()
        .and_then(locale_id)
        .unwrap_or_default();
    if supported(&id) {
        return id;
    }
    let mut user = user_locale();
    let keywords = id.split_once('@').map_or("", |(_, keywords)| keywords);
    for (name, value) in keywords
        .split(';')
        .filter_map(|keyword| keyword.split_once('='))
    {
        if let Ok(name) = c_string(name) {
            user = with_keyword(&user, &name, value).unwrap_or(user);
        }
    }
    user
}

/// The value of a keyword in an ICU locale ID, such as `phonebook` for
/// `collation`.
fn keyword(id: &CStr, keyword: &CStr) -> Option<String> {
    read_ascii(|value, capacity, status| {
        // SAFETY: The ID and keyword are C strings, and `value` has `capacity`
        // writable bytes.
        unsafe { uloc_getKeywordValue(id.as_ptr(), keyword.as_ptr(), value, capacity, status) }
    })
    .ok()
    .filter(|value| !value.is_empty())
}

/// An ICU locale ID with a keyword set to a value, replacing any it had.
fn with_keyword(id: &str, keyword: &CStr, value: &str) -> Result<String, String> {
    let value = c_string(value)?;
    // Room for the ID, a separator, the keyword, `=`, the value, and a NUL.
    let mut buffer = id.as_bytes().to_vec();
    buffer.resize(
        id.len() + keyword.count_bytes() + value.count_bytes() + 3,
        0,
    );
    let mut status = U_ZERO_ERROR;
    // SAFETY: The keyword and value are C strings, and the buffer holds the ID
    // as a C string with room for the keyword and value.
    let length = unsafe {
        uloc_setKeywordValue(
            keyword.as_ptr(),
            value.as_ptr(),
            buffer.as_mut_ptr().cast(),
            icu_length(buffer.len()),
            &mut status,
        )
    };
    check(status)?;
    buffer.truncate(usize::try_from(length).unwrap_or_default());
    String::from_utf8(buffer).map_err(|error| error.to_string())
}

/// Whether ICU has the collation type `kind`, such as `phonebook`, for a
/// locale.
fn has_collation_type(id: &CStr, kind: &str) -> bool {
    let mut status = U_ZERO_ERROR;
    // SAFETY: The key and locale are C strings.
    let kinds = unsafe {
        ucol_getKeywordValuesForLocale(c"collation".as_ptr(), id.as_ptr(), 0, &mut status)
    };
    if kinds.is_null() {
        return false;
    }
    let mut found = false;
    while check(status).is_ok() {
        let mut length = 0;
        // SAFETY: The enumeration is open.
        let value = unsafe { uenum_next(kinds, &mut length, &mut status) };
        if value.is_null() {
            break;
        }
        // SAFETY: ICU returns `length` readable bytes, valid until the next call.
        let value = unsafe {
            slice::from_raw_parts(
                value.cast::<u8>(),
                usize::try_from(length).unwrap_or_default(),
            )
        };
        if value == kind.as_bytes() {
            found = true;
            break;
        }
    }
    // SAFETY: The enumeration is open and not used again.
    unsafe { uenum_close(kinds) };
    found
}

pub struct Collator(NonNull<c_void>);

impl Drop for Collator {
    fn drop(&mut self) {
        // SAFETY: The collator is open and not used again.
        unsafe { ucol_close(self.0.as_ptr()) };
    }
}

/// Creates a collator and returns it with the locale it reports: the resolved
/// language with the collation extensions it honors.
pub fn new_collator(
    requested: &[u8],
    case_sensitive: bool,
    diacritic_sensitive: bool,
) -> Result<(Collator, String), String> {
    let requested = c_string(&resolve_locale(requested))?;
    // The collator takes only the extensions it honors, which it also reports.
    // ICU rejects a locale whose `kf` or `kn` it does not understand, and
    // falls back to the standard collation for an unknown `co`.
    let mut keywords = Vec::new();
    for (name, values) in [
        (c"collation", None),
        (c"colcasefirst", Some(["upper", "lower", "no"].as_slice())),
        (c"colnumeric", Some(["yes", "no"].as_slice())),
    ] {
        let Some(value) = keyword(&requested, name) else {
            continue;
        };
        let honored = values.map_or_else(
            || has_collation_type(&requested, &value),
            |values| values.contains(&value.as_str()),
        );
        if honored {
            keywords.push(format!("{}={value}", name.to_string_lossy()));
        }
    }
    let mut id = read_ascii(|name, capacity, status| {
        // SAFETY: The ID is a C string, and `name` has `capacity` writable bytes.
        unsafe { uloc_getBaseName(requested.as_ptr(), name, capacity, status) }
    })?;
    if !keywords.is_empty() {
        id = format!("{id}@{}", keywords.join(";"));
    }
    let id = c_string(&id)?;

    let mut status = U_ZERO_ERROR;
    // SAFETY: The ID is a C string.
    let collator = unsafe { ucol_open(id.as_ptr(), &mut status) };
    check(status)?;
    let collator = Collator(NonNull::new(collator).ok_or("ICU opened no collator")?);
    let strength = match (diacritic_sensitive, case_sensitive) {
        (false, _) => UCOL_PRIMARY,
        (true, false) => UCOL_SECONDARY,
        (true, true) => UCOL_TERTIARY,
    };
    let case_level = if case_sensitive && !diacritic_sensitive {
        UCOL_ON
    } else {
        UCOL_OFF
    };
    for (attribute, value) in [
        (UCOL_STRENGTH, strength),
        (UCOL_CASE_LEVEL, case_level),
        (UCOL_NORMALIZATION_MODE, UCOL_ON),
    ] {
        // SAFETY: The collator is open.
        unsafe { ucol_setAttribute(collator.0.as_ptr(), attribute, value, &mut status) };
    }
    check(status)?;
    let tag = read_ascii(|tag, capacity, status| {
        // SAFETY: The ID is a C string, and `tag` has `capacity` writable bytes.
        unsafe { uloc_toLanguageTag(id.as_ptr(), tag, capacity, 0, status) }
    })?;
    Ok((collator, tag))
}

pub fn compare(collator: &Collator, lhs: &[u8], rhs: &[u8]) -> Ordering {
    let mut status = U_ZERO_ERROR;
    // SAFETY: The collator is open, and each string has its length in readable
    // bytes. ICU reads at most `i32::MAX` bytes of each.
    let result = unsafe {
        ucol_strcollUTF8(
            collator.0.as_ptr(),
            lhs.as_ptr().cast(),
            icu_length(lhs.len()),
            rhs.as_ptr().cast(),
            icu_length(rhs.len()),
            &mut status,
        )
    };
    result.cmp(&0)
}

struct NumberFormat(NonNull<c_void>);

impl Drop for NumberFormat {
    fn drop(&mut self) {
        // SAFETY: The formatter is open and not used again.
        unsafe { unum_close(self.0.as_ptr()) };
    }
}

pub fn format_number(
    number: f64,
    requested: &[u8],
    currency: &[u8],
    min: Option<u8>,
    max: Option<u8>,
) -> Result<String, String> {
    let mut id = resolve_locale(requested);
    let style = if currency.is_empty() {
        UNUM_DECIMAL
    } else {
        id = with_keyword(&id, c"currency", &currency_code(currency)?)?;
        UNUM_CURRENCY
    };
    let id = c_string(&id)?;
    let mut status = U_ZERO_ERROR;
    // SAFETY: The ID is a C string, and a null pattern takes the style's.
    let formatter = unsafe {
        unum_open(
            style,
            ptr::null(),
            0,
            id.as_ptr(),
            ptr::null_mut(),
            &mut status,
        )
    };
    check(status)?;
    let formatter = NumberFormat(NonNull::new(formatter).ok_or("ICU opened no formatter")?);
    // Each limit moves the other past it, so setting the maximum last lets an
    // explicit maximum win over a minimum.
    for (attribute, limit) in [
        (UNUM_MIN_FRACTION_DIGITS, min),
        (UNUM_MAX_FRACTION_DIGITS, max),
    ] {
        if let Some(limit) = limit {
            // SAFETY: The formatter is open.
            unsafe { unum_setAttribute(formatter.0.as_ptr(), attribute, i32::from(limit)) };
        }
    }
    let text = read(|result, capacity, status| {
        // SAFETY: The formatter is open, `result` has `capacity` writable UTF-16
        // units, and a null position asks for none.
        unsafe {
            unum_formatDouble(
                formatter.0.as_ptr(),
                number,
                result,
                capacity,
                ptr::null_mut(),
                status,
            )
        }
    })?;
    String::from_utf16(&text).map_err(|error| error.to_string())
}
