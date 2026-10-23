// Locale-sensitive style expressions in the browser, which evaluate them with
// the JavaScript engine's Intl API, as MapLibre GL JS does.

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>

#include <mln/i18n/collator.hpp>
#include <mln/i18n/number_format.hpp>
#include <mln/util/exception.hpp>

#include <emscripten/em_js.h>

EM_JS_DEPS(mlnffi_intl, "$UTF8ToString,$stringToNewUTF8");

// Each function below returns text that the caller frees, and sets `*error`
// when the text is the message of an exception that Intl threw. An empty
// locale means the engine's default locale.

// Returns the locale that a collator for `locale` resolves to.
EM_JS(
  char*, mlnffi_intl_collator_resolve,
  (const char* locale, size_t length, const char* sensitivity, bool* error), {
    try {
      const requested = length ? UTF8ToString(locale, length, true) : undefined;
      return stringToNewUTF8(
        new Intl.Collator(requested, {sensitivity : UTF8ToString(sensitivity)})
          .resolvedOptions()
          .locale
      );
    } catch (exception) {
      HEAP8[error] = 1;
      return stringToNewUTF8(String(exception.message ?? exception));
    }
  }
);

// Compares two UTF-8 strings under a collator for a resolved locale. Each
// thread is a Web Worker with JavaScript objects of its own, so each keeps the
// collators it has made.
EM_JS(
  int, mlnffi_intl_collator_compare,
  (const char* locale, const char* sensitivity, const char* lhs,
   size_t lhsLength, const char* rhs, size_t rhsLength),
  {
    const options = {
      locale : UTF8ToString(locale),
      sensitivity : UTF8ToString(sensitivity)
    };
    const key = options.locale + ' ' + options.sensitivity;
    const collators = mlnffi_intl_collator_compare.collators || new Map();
    mlnffi_intl_collator_compare.collators = collators;
    let collator = collators.get(key);
    if (!collator) {
      collator = new Intl.Collator(options.locale, options);
      collators.set(key, collator);
    }
    return collator.compare(
      UTF8ToString(lhs, lhsLength, true), UTF8ToString(rhs, rhsLength, true)
    );
  }
);

// Formats a number as a decimal or, with a currency code, as currency. A
// negative fraction limit is omitted.
EM_JS(
  char*, mlnffi_intl_format_number,
  (double number, const char* locale, size_t localeLength, const char* currency,
   size_t currencyLength, int minFractionDigits, int maxFractionDigits,
   bool* error),
  {
    try {
      // Other platforms round half to even, and let an explicit maximum win
      // over a minimum where Intl would throw.
      const options = {roundingMode : 'halfEven'};
      if (currencyLength) {
        options.style = 'currency';
        options.currency = UTF8ToString(currency, currencyLength, true);
      }
      if (minFractionDigits >= 0) {
        options.minimumFractionDigits =
          maxFractionDigits >= 0
            ? Math.min(minFractionDigits, maxFractionDigits)
            : minFractionDigits;
      }
      if (maxFractionDigits >= 0) {
        options.maximumFractionDigits = maxFractionDigits;
      }
      const requested =
        localeLength ? UTF8ToString(locale, localeLength, true) : undefined;
      return stringToNewUTF8(
        new Intl.NumberFormat(requested, options).format(number)
      );
    } catch (exception) {
      HEAP8[error] = 1;
      return stringToNewUTF8(String(exception.message ?? exception));
    }
  }
);

namespace {

// Copies and frees text from JavaScript, throwing it as a locale failure when
// it is an error message.
auto take(char* text, bool error) -> std::string {
  const std::unique_ptr<char, decltype(&std::free)> owned(text, std::free);
  if (text == nullptr) {
    throw std::bad_alloc();
  }
  std::string value(text);
  if (error) {
    throw mln::util::LocaleException(value);
  }
  return value;
}

// Intl.Collator's name for a pair of sensitivities.
auto sensitivityName(bool caseSensitive, bool diacriticSensitive) -> const
  char* {
  if (caseSensitive) {
    return diacriticSensitive ? "variant" : "case";
  }
  return diacriticSensitive ? "accent" : "base";
}

}  // namespace

namespace mln::platform {

// MapLibre can create a collator on one thread and compare with it on another,
// so it holds the resolved locale rather than a JavaScript object.
class Collator::Impl {
 public:
  Impl(
    bool caseSensitive, bool diacriticSensitive,
    const std::optional<std::string>& locale
  )
      : sensitivity(sensitivityName(caseSensitive, diacriticSensitive)) {
    const auto requested = locale.value_or("");
    bool error = false;
    char* text = mlnffi_intl_collator_resolve(
      requested.data(), requested.size(), sensitivity, &error
    );
    resolved_locale = take(text, error);
  }

  [[nodiscard]] auto compare(
    const std::string& lhs, const std::string& rhs
  ) const -> int {
    return mlnffi_intl_collator_compare(
      resolved_locale.c_str(), sensitivity, lhs.data(), lhs.size(), rhs.data(),
      rhs.size()
    );
  }

  auto operator==(const Impl& other) const -> bool {
    return std::string_view(sensitivity) == other.sensitivity &&
           resolved_locale == other.resolved_locale;
  }

  [[nodiscard]] auto resolvedLocale() const -> std::string {
    return resolved_locale;
  }

 private:
  const char* sensitivity;
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

auto formatNumber(
  double number, const std::string& localeId, const std::string& currency,
  std::optional<uint8_t> minFractionDigits,
  std::optional<uint8_t> maxFractionDigits
) -> std::string {
  bool error = false;
  char* text = mlnffi_intl_format_number(
    number, localeId.data(), localeId.size(), currency.data(), currency.size(),
    minFractionDigits ? *minFractionDigits : -1,
    maxFractionDigits ? *maxFractionDigits : -1, &error
  );
  return take(text, error);
}

}  // namespace mln::platform
