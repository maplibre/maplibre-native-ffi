// Locale-sensitive style expressions on OpenHarmony, which evaluate them with
// the ICU4C that the system exposes through libicu.so. These sources must see
// the sysroot's ICU headers rather than the ICU that Native vendors for bidi:
// the vendored headers rename every symbol for ICU 61.
//
// libicu.so does not locate ICU's data itself. System libraries such as the
// ArkTS runtime and the image decoders this library links do, so a process
// that loads only libicu.so would fail every lookup.

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <mln/i18n/collator.hpp>
#include <mln/i18n/number_format.hpp>
#include <mln/util/exception.hpp>
#include <mln/util/utf.hpp>

#include <unicode/ucol.h>
#include <unicode/uloc.h>
#include <unicode/umachine.h>
#include <unicode/unum.h>
#include <unicode/utypes.h>

namespace {

// u_errorName would need API version 15, so the message names the code.
void check(UErrorCode status) {
  if (U_FAILURE(status) != 0) {
    throw mln::util::LocaleException(
      "ICU failed with error code " + std::to_string(status)
    );
  }
}

// Reads the text an ICU function writes into a buffer, growing the buffer
// once if the first attempt overflows.
template <typename Char, typename Write>
auto read(Write write) -> std::basic_string<Char> {
  std::basic_string<Char> text(64, Char{});
  UErrorCode status = U_ZERO_ERROR;
  auto length = write(text.data(), static_cast<int32_t>(text.size()), &status);
  if (status == U_BUFFER_OVERFLOW_ERROR) {
    text.resize(length);
    status = U_ZERO_ERROR;
    length = write(text.data(), length, &status);
  }
  check(status);
  text.resize(length);
  return text;
}

// One part of an ICU locale ID, such as its language or a keyword's value.
template <typename Get>
auto part(const std::string& locale_id, Get get) -> std::string {
  return read<char>(
    [&](char* buffer, int32_t capacity, UErrorCode* status) -> int32_t {
      return get(locale_id.c_str(), buffer, capacity, status);
    }
  );
}

auto keyword(const std::string& locale_id, const char* key) -> std::string {
  return read<char>(
    [&](char* buffer, int32_t capacity, UErrorCode* status) -> int32_t {
      return uloc_getKeywordValue(
        locale_id.c_str(), key, buffer, capacity, status
      );
    }
  );
}

auto with_keyword(
  std::string locale_id, const char* key, const std::string& value
) -> std::string {
  if (value.empty()) {
    return locale_id;
  }
  // Room for the separators and the terminator ICU writes.
  locale_id.resize(
    locale_id.size() + std::char_traits<char>::length(key) + value.size() + 3
  );
  UErrorCode status = U_ZERO_ERROR;
  const auto length = uloc_setKeywordValue(
    key, value.c_str(), locale_id.data(),
    static_cast<int32_t>(locale_id.size()), &status
  );
  check(status);
  locale_id.resize(length);
  return locale_id;
}

// The language, script, and region of an ICU locale ID.
auto base(const std::string& locale_id) -> std::string {
  std::string result = part(locale_id, uloc_getLanguage);
  for (const auto& subtag :
       {part(locale_id, uloc_getScript), part(locale_id, uloc_getCountry)}) {
    if (!subtag.empty()) {
      result += "_" + subtag;
    }
  }
  return result;
}

// Whether the system's CLDR data knows the language of an ICU locale ID. ICU
// would fall back to root data for an unknown language, which it shares with
// supported languages such as English.
auto supported(const std::string& locale_id) -> bool {
  const auto language = part(locale_id, uloc_getLanguage);
  return !language.empty() &&
         !part(part(language, uloc_addLikelySubtags), uloc_getScript).empty();
}

// Resolves a requested language tag to an ICU locale ID. An empty or
// unsupported tag takes the language of ICU's default locale, keeping the
// requested Unicode extensions. A default that ICU's data lacks, such as the
// POSIX locale, means en-US.
auto resolve(const std::string& tag) -> std::string {
  int32_t parsed = 0;
  const auto locale_id = read<char>(
    [&](char* buffer, int32_t capacity, UErrorCode* status) -> int32_t {
      return uloc_forLanguageTag(
        tag.c_str(), buffer, capacity, &parsed, status
      );
    }
  );
  if (supported(locale_id)) {
    return locale_id;
  }
  auto fallback = base(uloc_getDefault());
  if (!supported(fallback)) {
    fallback = "en_US";
  }
  const auto keywords = locale_id.find('@');
  return keywords == std::string::npos ? fallback
                                       : fallback + locale_id.substr(keywords);
}

auto language_tag(const std::string& locale_id) -> std::string {
  return part(
    locale_id,
    [](
      const char* locale, char* buffer, int32_t capacity, UErrorCode* status
    ) -> int32_t {
      return uloc_toLanguageTag(locale, buffer, capacity, false, status);
    }
  );
}

// The locale a collator honors: the resolved language with the collation type
// that the system's data holds for it, and any valid case-first and numeric
// keywords.
auto collation_locale(const std::string& tag) -> std::string {
  const auto requested = resolve(tag);
  auto locale_id = base(requested);
  const auto type = keyword(requested, "collation");
  if (!type.empty()) {
    const auto candidate = with_keyword(locale_id, "collation", type);
    const auto equivalent = read<char>(
      [&](char* buffer, int32_t capacity, UErrorCode* status) -> int32_t {
        UBool available = false;
        return ucol_getFunctionalEquivalent(
          buffer, capacity, "collation", candidate.c_str(), &available, status
        );
      }
    );
    if (keyword(equivalent, "collation") == type) {
      locale_id = candidate;
    }
  }
  // ICU fails to open a collator for a value it does not know.
  const auto case_first = keyword(requested, "colcasefirst");
  if (case_first == "upper" || case_first == "lower" || case_first == "no") {
    locale_id = with_keyword(locale_id, "colcasefirst", case_first);
  }
  const auto numeric = keyword(requested, "colnumeric");
  if (numeric == "yes" || numeric == "no") {
    locale_id = with_keyword(locale_id, "colnumeric", numeric);
  }
  return locale_id;
}

}  // namespace

namespace mln::platform {

class Collator::Impl {
 public:
  Impl(
    bool caseSensitive, bool diacriticSensitive,
    const std::optional<std::string>& locale
  )
      : case_sensitive(caseSensitive), diacritic_sensitive(diacriticSensitive) {
    const auto id = collation_locale(locale.value_or(""));
    resolved_locale = language_tag(id);
    UErrorCode status = U_ZERO_ERROR;
    collator.reset(ucol_open(id.c_str(), &status));
    check(status);
    UColAttributeValue strength = UCOL_PRIMARY;
    if (diacriticSensitive) {
      strength = caseSensitive ? UCOL_TERTIARY : UCOL_SECONDARY;
    }
    ucol_setStrength(collator.get(), strength);
    ucol_setAttribute(
      collator.get(), UCOL_CASE_LEVEL,
      caseSensitive && !diacriticSensitive ? UCOL_ON : UCOL_OFF, &status
    );
    ucol_setAttribute(
      collator.get(), UCOL_NORMALIZATION_MODE, UCOL_ON, &status
    );
    check(status);
  }

  // Ill-formed UTF-8 compares as U+FFFD.
  [[nodiscard]] auto compare(
    const std::string& lhs, const std::string& rhs
  ) const -> int {
    UErrorCode status = U_ZERO_ERROR;
    const auto result = ucol_strcollUTF8(
      collator.get(), lhs.data(), static_cast<int32_t>(lhs.size()), rhs.data(),
      static_cast<int32_t>(rhs.size()), &status
    );
    check(status);
    return result;
  }

  auto operator==(const Impl& other) const -> bool {
    return case_sensitive == other.case_sensitive &&
           diacritic_sensitive == other.diacritic_sensitive &&
           resolved_locale == other.resolved_locale;
  }

  [[nodiscard]] auto resolvedLocale() const -> std::string {
    return resolved_locale;
  }

 private:
  std::unique_ptr<UCollator, decltype(&ucol_close)> collator{
    nullptr, ucol_close
  };
  bool case_sensitive;
  bool diacritic_sensitive;
  std::string resolved_locale;
};

Collator::Collator(
  bool caseSensitive, bool diacriticSensitive,
  const std::optional<std::string>& locale
)
    : impl(std::make_shared<Impl>(caseSensitive, diacriticSensitive, locale)) {}

auto Collator::operator==(const Collator& other) const -> bool {
  return *impl == *other.impl;
}

auto Collator::compare(const std::string& lhs, const std::string& rhs) const
  -> int {
  return impl->compare(lhs, rhs);
}

auto Collator::resolvedLocale() const -> std::string {
  return impl->resolvedLocale();
}

// ICU rounds half to even. Omitted limits keep the style's defaults: 0 and 3,
// or the currency's digits. A minimum above the maximum raises it, and a later
// maximum lowers the minimum, so an explicit maximum wins.
auto formatNumber(
  double number, const std::string& localeId, const std::string& currency,
  std::optional<uint8_t> minFractionDigits,
  std::optional<uint8_t> maxFractionDigits
) -> std::string {
  // ICU takes any three invariant characters as a code, and the first three of
  // a longer one.
  const auto letter = [](char character) -> bool {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
  };
  if (
    !currency.empty() &&
    (currency.size() != 3 || !std::ranges::all_of(currency, letter))
  ) {
    throw util::LocaleException("Currency is not an ISO 4217 code");
  }
  const auto id = resolve(localeId);
  UErrorCode status = U_ZERO_ERROR;
  const std::unique_ptr<UNumberFormat, decltype(&unum_close)> formatter{
    unum_open(
      currency.empty() ? UNUM_DECIMAL : UNUM_CURRENCY, nullptr, 0, id.c_str(),
      nullptr, &status
    ),
    unum_close
  };
  check(status);
  if (!currency.empty()) {
    const auto code = util::convertUTF8ToUTF16(currency);
    unum_setTextAttribute(
      formatter.get(), UNUM_CURRENCY_CODE, code.data(),
      static_cast<int32_t>(code.size()), &status
    );
    check(status);
  }
  if (minFractionDigits) {
    unum_setAttribute(
      formatter.get(), UNUM_MIN_FRACTION_DIGITS, *minFractionDigits
    );
  }
  if (maxFractionDigits) {
    unum_setAttribute(
      formatter.get(), UNUM_MAX_FRACTION_DIGITS, *maxFractionDigits
    );
  }
  return util::convertUTF16ToUTF8(
    read<char16_t>(
      [&](char16_t* buffer, int32_t capacity, UErrorCode* status) -> int32_t {
        return unum_formatDouble(
          formatter.get(), number, buffer, capacity, nullptr, status
        );
      }
    )
  );
}

}  // namespace mln::platform
