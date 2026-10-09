use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::{ptr, slice, str};

use fixed_decimal::{Decimal, FloatPrecision, Sign};
use icu_collator::options::{CaseLevel, CollatorOptions, Strength};
use icu_collator::provider::{Baked as CollationData, CollationMetadataV1};
use icu_collator::{Collator, CollatorBorrowed, CollatorPreferences};
use icu_decimal::{AbstractFormatter, DecimalFormatter, DecimalFormatterPreferences};
use icu_experimental::dimension::currency::CurrencyType;
use icu_experimental::dimension::provider::currency::{
    essentials::CurrencyEssentialsV1, fractions::CurrencyFractionsV1, symbols::CurrencySymbolsV1,
};
use icu_locale::{LocaleCanonicalizer, LocaleExpander};
use icu_locale_core::Locale;
use icu_locale_core::extensions::unicode::key;
use icu_provider::prelude::*;
use tinystr::TinyAsciiStr;
use writeable::Writeable;

#[repr(C)]
pub struct RustText {
    data: *mut u8,
    length: usize,
    failed: bool,
}

struct RustCollator {
    collator: CollatorBorrowed<'static>,
}

fn text_result(result: Result<String, String>) -> RustText {
    let (text, failed) = match result {
        Ok(text) => (text, false),
        Err(error) => (error, true),
    };
    let data = text.into_bytes().into_boxed_slice();
    let length = data.len();
    RustText {
        data: Box::into_raw(data).cast(),
        length,
        failed,
    }
}

fn protect(function: impl FnOnce() -> Result<String, String>) -> RustText {
    text_result(
        catch_unwind(AssertUnwindSafe(function))
            .unwrap_or_else(|_| Err("Linux locale operation failed".into())),
    )
}

unsafe fn input<'a>(data: *const u8, length: usize) -> Result<&'a str, String> {
    // SAFETY: The internal C++ adapter supplies a readable, length-delimited string.
    str::from_utf8(unsafe { slice::from_raw_parts(data, length) })
        .map_err(|_| "Locale input is not UTF-8".into())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_text_free(text: RustText) {
    // SAFETY: The C++ adapter returns each boxed string exactly once with its original length.
    unsafe {
        drop(Box::from_raw(ptr::slice_from_raw_parts_mut(
            text.data,
            text.length,
        )))
    };
}

// Linux locale archives are unnecessary: the selected tag names bundled CLDR data.
fn default_locale(category: &str) -> Locale {
    for name in ["LC_ALL", category, "LANG"] {
        if let Ok(value) = std::env::var(name) {
            if value.is_empty() {
                continue;
            }
            let tag = value.split(['.', '@']).next().unwrap_or_default();
            if matches!(tag, "C" | "POSIX") {
                break;
            }
            if let Ok(locale) = tag.replace('_', "-").parse() {
                return locale;
            }
        }
    }
    "en-US".parse().expect("the default locale is valid")
}

// Known languages can share root formatting or collation data. CLDR likely
// subtags distinguish those languages from an unrecognized requested language.
fn supports_locale(locale: &Locale) -> bool {
    if locale.id.language.is_unknown() {
        return false;
    }
    let mut language = locale.id.clone();
    language.script = None;
    language.region = None;
    LocaleExpander::new_common().maximize(&mut language);
    language.script.is_some()
}

fn resolve_locale(requested: &str, category: &str) -> Result<Locale, String> {
    let mut fallback = default_locale(category);
    let canonicalizer = LocaleCanonicalizer::new_common();
    canonicalizer.canonicalize(&mut fallback);
    let mut locale = if requested.is_empty() {
        fallback.clone()
    } else {
        requested
            .parse::<Locale>()
            .map_err(|_| "Invalid locale language tag".to_string())?
    };
    canonicalizer.canonicalize(&mut locale);
    if supports_locale(&locale) {
        return Ok(locale);
    }
    if supports_locale(&fallback) {
        return Ok(fallback);
    }
    Ok("en-US".parse().expect("the default locale is valid"))
}

fn create_collator(
    requested: &str,
    case: bool,
    accent: bool,
) -> Result<(RustCollator, String), String> {
    let mut locale = resolve_locale(requested, "LC_COLLATE")?;
    let prefs = CollatorPreferences::from(locale.clone());
    let data_locale = CollationMetadataV1::make_locale(prefs.locale_preferences);
    let attributes = prefs
        .collation_type
        .as_ref()
        .map(|kind| DataMarkerAttributes::from_str_or_panic(kind.as_str()))
        .unwrap_or_default();
    let mut response = DataProvider::<CollationMetadataV1>::load(
        &CollationData,
        DataRequest {
            id: DataIdentifierBorrowed::for_marker_attributes_and_locale(attributes, &data_locale),
            ..Default::default()
        },
    );
    if response.is_err() || prefs.collation_type.is_none() {
        locale.extensions.unicode.keywords.remove(key!("co"));
        response = DataProvider::<CollationMetadataV1>::load(
            &CollationData,
            DataRequest {
                id: DataIdentifierBorrowed::for_locale(&data_locale),
                ..Default::default()
            },
        );
    }
    let response = response.map_err(|error| error.to_string())?;
    let selected = response.metadata.locale.unwrap_or(data_locale);
    // Root data is shared by supported languages such as English and German.
    if !selected.is_unknown() {
        locale.id = selected
            .to_string()
            .parse()
            .map_err(|_| "Invalid collation data locale".to_string())?;
    }
    if prefs.case_first.is_none() {
        locale.extensions.unicode.keywords.remove(key!("kf"));
    }
    if prefs.numeric_ordering.is_none() {
        locale.extensions.unicode.keywords.remove(key!("kn"));
    }
    let mut reported = Locale::from(locale.id.clone());
    for keyword in [key!("co"), key!("kf"), key!("kn")] {
        if let Some(value) = locale.extensions.unicode.keywords.get(&keyword) {
            reported
                .extensions
                .unicode
                .keywords
                .set(keyword, value.clone());
        }
    }
    let mut options = CollatorOptions::default();
    options.strength = Some(if accent {
        if case {
            Strength::Tertiary
        } else {
            Strength::Secondary
        }
    } else {
        Strength::Primary
    });
    options.case_level = Some(if case && !accent {
        CaseLevel::On
    } else {
        CaseLevel::Off
    });
    let collator =
        Collator::try_new(reported.clone().into(), options).map_err(|error| error.to_string())?;
    Ok((RustCollator { collator }, reported.to_string()))
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_collator_new(
    locale: *const u8,
    length: usize,
    case: bool,
    accent: bool,
    output: *mut *mut c_void,
) -> RustText {
    protect(|| {
        // SAFETY: The adapter passes a readable string and writable output pointer.
        let (collator, resolved) =
            create_collator(unsafe { input(locale, length) }?, case, accent)?;
        unsafe { *output = Box::into_raw(Box::new(collator)).cast() };
        Ok(resolved)
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_collator_free(collator: *mut c_void) {
    // SAFETY: The adapter returns the owned collator exactly once.
    unsafe { drop(Box::from_raw(collator.cast::<RustCollator>())) };
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_collator_compare(
    collator: *const c_void,
    lhs: *const u8,
    lhs_length: usize,
    rhs: *const u8,
    rhs_length: usize,
) -> i32 {
    // SAFETY: The adapter keeps the collator alive and supplies readable string spans.
    let (collator, lhs, rhs) = unsafe {
        (
            &*collator.cast::<RustCollator>(),
            slice::from_raw_parts(lhs, lhs_length),
            slice::from_raw_parts(rhs, rhs_length),
        )
    };
    match collator.collator.compare_utf8(lhs, rhs) {
        std::cmp::Ordering::Less => -1,
        std::cmp::Ordering::Equal => 0,
        std::cmp::Ordering::Greater => 1,
    }
}

fn fraction_limits(
    minimum: i16,
    maximum: i16,
    currency_digits: Option<u8>,
) -> Result<(i16, i16), String> {
    let default_min = i16::from(currency_digits.unwrap_or(0));
    let default_max = i16::from(currency_digits.unwrap_or(3));
    let min = if minimum >= 0 {
        minimum
    } else {
        default_min.min(if maximum >= 0 { maximum } else { default_min })
    };
    let max = if maximum >= 0 {
        maximum
    } else {
        default_max.max(min)
    };
    if min > max {
        return Err("Minimum fraction digits exceeds maximum fraction digits".into());
    }
    Ok((min, max))
}

fn round_number(number: f64, min: i16, max: i16) -> Result<Decimal, String> {
    let mut value = Decimal::try_from_f64(number, FloatPrecision::RoundTrip)
        .map_err(|error| error.to_string())?;
    value.round(-max);
    value.trim_end();
    value.pad_end(-min);
    Ok(value)
}

fn format_number(
    number: f64,
    requested: &str,
    currency: &str,
    min: i16,
    max: i16,
) -> Result<String, String> {
    let locale = resolve_locale(requested, "LC_NUMERIC")?;
    let prefs = DecimalFormatterPreferences::from(locale);
    let formatter =
        DecimalFormatter::try_new(prefs, Default::default()).map_err(|error| error.to_string())?;
    if currency.is_empty() {
        let (min, max) = fraction_limits(min, max, None)?;
        return Ok(formatter
            .format(&round_number(number, min, max)?)
            .write_to_string()
            .into_owned());
    }
    if currency.len() != 3 || !currency.bytes().all(|byte| byte.is_ascii_alphabetic()) {
        return Err("Invalid ISO currency code".into());
    }
    let currency: CurrencyType = currency
        .parse()
        .map_err(|_| "Invalid ISO currency code".to_string())?;
    let fractions: DataResponse<CurrencyFractionsV1> = icu_experimental::provider::Baked
        .load(Default::default())
        .map_err(|error| error.to_string())?;
    let (min, max) = fraction_limits(
        min,
        max,
        Some(fractions.payload.get().resolve(currency).digits),
    )?;
    let value = round_number(number, min, max)?;
    // The experimental formatter fixes currency precision. Use its CLDR patterns
    // with the decimal formatter to preserve explicit expression fraction limits.
    let data_locale = CurrencyEssentialsV1::make_locale(prefs.locale_preferences);
    let default_id = DataIdentifierBorrowed::for_locale(&data_locale);
    let essential: DataResponse<CurrencyEssentialsV1> = prefs
        .nu_id(&data_locale)
        .and_then(|id| {
            icu_experimental::provider::Baked
                .load(DataRequest {
                    id,
                    ..Default::default()
                })
                .ok()
        })
        .map(Ok)
        .unwrap_or_else(|| {
            icu_experimental::provider::Baked.load(DataRequest {
                id: default_id,
                ..Default::default()
            })
        })
        .map_err(|error| error.to_string())?;
    let mut attribute_buffer = TinyAsciiStr::EMPTY;
    let symbol: Option<DataResponse<CurrencySymbolsV1>> = icu_experimental::provider::Baked
        .load(DataRequest {
            id: DataIdentifierBorrowed::for_marker_attributes_and_locale(
                CurrencySymbolsV1::make_attributes(
                    currency,
                    CurrencySymbolsV1::SHORT,
                    &mut attribute_buffer,
                ),
                &data_locale,
            ),
            ..Default::default()
        })
        .allow_identifier_not_found()
        .map_err(|error| error.to_string())?;
    let iso = currency.iso_code();
    let (symbol, starts, ends) = match &symbol {
        Some(symbol) => {
            let symbol = symbol.payload.get();
            (
                symbol.as_str(),
                symbol.starts_with_letter(),
                symbol.ends_with_letter(),
            )
        }
        None => (iso.as_str(), true, true),
    };
    let essentials = essential.payload.get();
    let negative = (value.sign == Sign::Negative)
        .then(|| essentials.get_negative(starts, ends))
        .flatten();
    let pattern = negative.unwrap_or_else(|| essentials.get_positive(starts, ends));
    let sign = if negative.is_some() {
        Sign::None
    } else {
        value.sign
    };
    let formatted = AbstractFormatter::format_unsigned(&formatter, value.absolute);
    Ok(
        AbstractFormatter::format_sign(&formatter, pattern.interpolate((formatted, symbol)), sign)
            .write_to_string()
            .into_owned(),
    )
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_format_number(
    number: f64,
    locale: *const u8,
    locale_length: usize,
    currency: *const u8,
    currency_length: usize,
    min: i16,
    max: i16,
) -> RustText {
    protect(|| {
        // SAFETY: The adapter supplies readable, length-delimited strings.
        let (locale, currency) = unsafe {
            (
                input(locale, locale_length)?,
                input(currency, currency_length)?,
            )
        };
        format_number(number, locale, currency, min, max)
    })
}
