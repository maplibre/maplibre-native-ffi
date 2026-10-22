//! Locale-sensitive style expressions on Linux. The C library has no portable
//! locale services there, so the expressions use the CLDR data that ICU4X
//! compiles into the library.

use std::cmp::Ordering;
use std::ffi::c_void;
use std::{ptr, slice};

use fixed_decimal::{Decimal, FloatPrecision, Sign};
use icu_collator::options::{CaseLevel, CollatorOptions, Strength};
use icu_collator::provider::CollationMetadataV1;
use icu_collator::{Collator, CollatorBorrowed, CollatorPreferences};
use icu_decimal::{AbstractFormatter, DecimalFormatter, DecimalFormatterPreferences};
use icu_experimental::dimension::currency::CurrencyType;
use icu_experimental::dimension::provider::currency::essentials::CurrencyEssentialsV1;
use icu_experimental::dimension::provider::currency::fractions::CurrencyFractionsV1;
use icu_experimental::dimension::provider::currency::symbols::CurrencySymbolsV1;
use icu_locale::{LocaleCanonicalizer, LocaleExpander};
use icu_locale_core::extensions::unicode::key;
use icu_locale_core::subtags::script;
use icu_locale_core::{Locale, locale};
use icu_provider::prelude::*;
use writeable::Writeable;

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

/// The locale the environment names for a POSIX locale category, as the first
/// nonempty of `LC_ALL`, the category, and `LANG`, such as `sv_SE.UTF-8` or
/// `sr_RS.UTF-8@latin`. `C`, `POSIX`, and an unset or unsupported locale mean
/// `en-US`.
fn environment_locale(category: &str, variable: impl Fn(&str) -> Option<String>) -> Locale {
    let name = ["LC_ALL", category, "LANG"]
        .into_iter()
        .find_map(|name| variable(name).filter(|value| !value.is_empty()))
        .unwrap_or_default();
    let (name, modifier) = name.split_once('@').unwrap_or((&name, ""));
    let tag = name.split('.').next().unwrap_or_default();
    let Some(mut locale) = supported_locale(&tag.replace('_', "-")) else {
        return locale!("en-US");
    };
    // glibc names a language's non-default script with a modifier.
    locale.id.script = match modifier {
        "cyrillic" => Some(script!("Cyrl")),
        "devanagari" => Some(script!("Deva")),
        "latin" => Some(script!("Latn")),
        _ => locale.id.script,
    };
    locale
}

/// Parses and canonicalizes a language tag whose language CLDR knows. Data
/// lookups would fall back to root data for an unknown language, which they
/// share with supported languages such as English.
fn supported_locale(tag: &str) -> Option<Locale> {
    let mut locale = Locale::try_from_str(tag).ok()?;
    LocaleCanonicalizer::new_common().canonicalize(&mut locale);
    let mut language = locale.id.clone();
    language.script = None;
    language.region = None;
    LocaleExpander::new_common().maximize(&mut language);
    language.script.is_some().then_some(locale)
}

/// Resolves a requested language tag for a POSIX locale category. An empty or
/// unsupported tag takes the environment's language, keeping the requested
/// Unicode extensions.
fn resolve_locale(requested: &[u8], category: &str) -> Locale {
    let requested = str::from_utf8(requested).unwrap_or_default();
    supported_locale(requested).unwrap_or_else(|| {
        let mut locale = Locale::try_from_str(requested).unwrap_or(Locale::UNKNOWN);
        locale.id = environment_locale(category, |name| std::env::var(name).ok()).id;
        locale
    })
}

/// A data request that leaves a miss to the caller instead of logging it.
fn request(id: DataIdentifierBorrowed<'_>) -> DataRequest<'_> {
    let mut metadata = DataRequestMetadata::default();
    metadata.silent = true;
    DataRequest { id, metadata }
}

/// Whether the compiled data holds the collation type that `prefs` requests.
fn has_collation_type(prefs: &CollatorPreferences) -> bool {
    let Some(attributes) = prefs
        .collation_type
        .as_ref()
        .and_then(|kind| DataMarkerAttributes::try_from_str(kind.as_str()).ok())
    else {
        return false;
    };
    let locale = CollationMetadataV1::make_locale(prefs.locale_preferences);
    DataProvider::<CollationMetadataV1>::load(
        &icu_collator::provider::Baked,
        request(DataIdentifierBorrowed::for_marker_attributes_and_locale(
            attributes, &locale,
        )),
    )
    .is_ok()
}

/// Creates a collator and returns it with the locale it reports: the resolved
/// language with the collation extensions it honors.
fn new_collator(
    requested: &[u8],
    case_sensitive: bool,
    diacritic_sensitive: bool,
) -> Result<(CollatorBorrowed<'static>, String), String> {
    let locale = resolve_locale(requested, "LC_COLLATE");
    let prefs = CollatorPreferences::from(&locale);
    let mut resolved = Locale::from(locale.id.clone());
    for (keyword, honored) in [
        (key!("co"), has_collation_type(&prefs)),
        (key!("kf"), prefs.case_first.is_some()),
        (key!("kn"), prefs.numeric_ordering.is_some()),
    ] {
        if let Some(value) = locale.extensions.unicode.keywords.get(&keyword)
            && honored
        {
            resolved
                .extensions
                .unicode
                .keywords
                .set(keyword, value.clone());
        }
    }

    let mut options = CollatorOptions::default();
    options.strength = Some(match (diacritic_sensitive, case_sensitive) {
        (false, _) => Strength::Primary,
        (true, false) => Strength::Secondary,
        (true, true) => Strength::Tertiary,
    });
    options.case_level = Some(if case_sensitive && !diacritic_sensitive {
        CaseLevel::On
    } else {
        CaseLevel::Off
    });
    let collator = Collator::try_new(prefs, options).map_err(|error| error.to_string())?;
    Ok((collator, resolved.to_string()))
}

/// Creates a collator for a language tag, or the environment's collation
/// locale for an empty tag, and stores it in `collator`. Returns the locale the
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
    new_collator(requested, case_sensitive, diacritic_sensitive)
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
    drop(unsafe { Box::from_raw(collator.cast::<CollatorBorrowed<'static>>()) });
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
            &*collator.cast::<CollatorBorrowed<'static>>(),
            bytes(lhs, lhs_length),
            bytes(rhs, rhs_length),
        )
    };
    match collator.compare_utf8(lhs, rhs) {
        Ordering::Less => -1,
        Ordering::Equal => 0,
        Ordering::Greater => 1,
    }
}

/// Rounds `number` half to even to at most `max` fraction digits and pads it
/// to at least `min`, as Android and Apple formatters do. Omitted limits
/// default to 0 and 3, or to the currency's digits for currency. An explicit
/// maximum wins over a minimum.
fn round(
    number: f64,
    min: Option<u8>,
    max: Option<u8>,
    currency_digits: Option<u8>,
) -> Result<Decimal, String> {
    let (default_min, default_max) = currency_digits.map_or((0, 3), |digits| (digits, digits));
    let max = max.unwrap_or_else(|| default_max.max(min.unwrap_or(0)));
    let min = min.unwrap_or(default_min).min(max);
    let mut decimal = Decimal::try_from_f64(number, FloatPrecision::RoundTrip)
        .map_err(|_| format!("Cannot format {number}"))?;
    decimal.round(-i16::from(max));
    decimal.trim_end();
    decimal.pad_end(-i16::from(min));
    Ok(decimal)
}

fn format_number(
    number: f64,
    requested: &[u8],
    currency: &[u8],
    min: Option<u8>,
    max: Option<u8>,
) -> Result<String, String> {
    let prefs = DecimalFormatterPreferences::from(&resolve_locale(requested, "LC_NUMERIC"));
    let formatter =
        DecimalFormatter::try_new(prefs, Default::default()).map_err(|error| error.to_string())?;
    if currency.is_empty() {
        let decimal = round(number, min, max, None)?;
        return Ok(formatter.format(&decimal).write_to_string().into_owned());
    }

    let currency = str::from_utf8(currency)
        .ok()
        .filter(|code| code.len() == 3 && code.bytes().all(|byte| byte.is_ascii_alphabetic()))
        .and_then(|code| CurrencyType::try_from_str(&code.to_ascii_uppercase()).ok())
        .ok_or("Currency is not an ISO 4217 code")?;
    let data = icu_experimental::provider::Baked;
    let fractions: DataResponse<CurrencyFractionsV1> = data
        .load(Default::default())
        .map_err(|error| error.to_string())?;
    let decimal = round(
        number,
        min,
        max,
        Some(fractions.payload.get().resolve(currency).digits),
    )?;

    // ICU4X's currency formatter always rounds to the currency's digits, so
    // apply its CLDR pattern to the rounded number instead.
    let locale = CurrencyEssentialsV1::make_locale(prefs.locale_preferences);
    let essentials: DataResponse<CurrencyEssentialsV1> = prefs
        .nu_id(&locale)
        .into_iter()
        .chain([DataIdentifierBorrowed::for_locale(&locale)])
        .find_map(|id| data.load(request(id)).ok())
        .ok_or("No currency patterns for the locale")?;
    let symbol: Option<DataResponse<CurrencySymbolsV1>> = data
        .load(request(
            DataIdentifierBorrowed::for_marker_attributes_and_locale(
                CurrencySymbolsV1::make_attributes(
                    currency,
                    CurrencySymbolsV1::SHORT,
                    &mut Default::default(),
                ),
                &locale,
            ),
        ))
        .allow_identifier_not_found()
        .map_err(|error| error.to_string())?;
    let code = currency.iso_code();
    let symbol = symbol.as_ref().map(|symbol| symbol.payload.get());
    let (symbol, starts_with_letter, ends_with_letter) =
        symbol.map_or((code.as_str(), true, true), |symbol| {
            (
                symbol.as_str(),
                symbol.starts_with_letter(),
                symbol.ends_with_letter(),
            )
        });
    let essentials = essentials.payload.get();
    let negative = (decimal.sign == Sign::Negative)
        .then(|| essentials.get_negative(starts_with_letter, ends_with_letter))
        .flatten();
    // An explicit negative pattern carries the sign itself.
    let (pattern, sign) = match negative {
        Some(pattern) => (pattern, Sign::None),
        None => (
            essentials.get_positive(starts_with_letter, ends_with_letter),
            decimal.sign,
        ),
    };
    let amount = formatter.format_unsigned(decimal.absolute);
    Ok(formatter
        .format_sign(pattern.interpolate((amount, symbol)), sign)
        .write_to_string()
        .into_owned())
}

/// Formats a number for a language tag, or the environment's numeric locale
/// for an empty tag, as a decimal or, with a currency code, as currency. A
/// negative fraction limit is omitted.
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
    format_number(
        number,
        locale,
        currency,
        u8::try_from(min_fraction_digits).ok(),
        u8::try_from(max_fraction_digits).ok(),
    )
    .into()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn environment_locale_follows_posix_precedence() {
        let locale = |variables: &[(&str, &str)]| {
            environment_locale("LC_COLLATE", |name| {
                variables
                    .iter()
                    .find(|(variable, _)| *variable == name)
                    .map(|(_, value)| (*value).to_owned())
            })
            .to_string()
        };
        assert_eq!(locale(&[]), "en-US");
        assert_eq!(locale(&[("LANG", "de_DE.UTF-8")]), "de-DE");
        assert_eq!(
            locale(&[("LC_COLLATE", "sv_SE"), ("LANG", "de_DE")]),
            "sv-SE"
        );
        assert_eq!(
            locale(&[("LC_ALL", ""), ("LC_COLLATE", "sv_SE@euro")]),
            "sv-SE"
        );
        assert_eq!(locale(&[("LC_ALL", "C.UTF-8"), ("LANG", "de_DE")]), "en-US");
        assert_eq!(locale(&[("LC_ALL", "POSIX")]), "en-US");
        assert_eq!(locale(&[("LC_ALL", "xx_ZZ"), ("LANG", "de_DE")]), "en-US");
        assert_eq!(locale(&[("LANG", "iw_IL")]), "he-IL");
        assert_eq!(locale(&[("LANG", "sr_RS.UTF-8@latin")]), "sr-Latn-RS");
    }
}
