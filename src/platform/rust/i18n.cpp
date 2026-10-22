#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <mln/i18n/collator.hpp>
#include <mln/i18n/number_format.hpp>
#include <mln/util/exception.hpp>

extern "C" {

struct MlnRustText {
  char* data;
  std::size_t length;
  bool error;
};

void mlnffi_rust_text_free(MlnRustText text);
auto mlnffi_rust_collator_new(
  const char* locale, std::size_t length, bool caseSensitive,
  bool diacriticSensitive, void** collator
) -> MlnRustText;
void mlnffi_rust_collator_free(void* collator);
auto mlnffi_rust_collator_compare(
  const void* collator, const char* lhs, std::size_t lhsLength, const char* rhs,
  std::size_t rhsLength
) -> int32_t;
auto mlnffi_rust_format_number(
  double number, const char* locale, std::size_t localeLength,
  const char* currency, std::size_t currencyLength, int16_t minFractionDigits,
  int16_t maxFractionDigits
) -> MlnRustText;

}  // extern "C"

namespace {

// Copies and frees text from Rust, throwing its error as a locale failure.
auto take(MlnRustText text) -> std::string {
  std::string value(text.data, text.length);
  const bool error = text.error;
  mlnffi_rust_text_free(text);
  if (error) {
    throw mln::util::LocaleException(value);
  }
  return value;
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
    const auto requested = locale.value_or("");
    void* created = nullptr;
    auto text = mlnffi_rust_collator_new(
      requested.data(), requested.size(), caseSensitive, diacriticSensitive,
      &created
    );
    collator.reset(created);
    resolved_locale = take(text);
  }

  [[nodiscard]] auto compare(
    const std::string& lhs, const std::string& rhs
  ) const -> int {
    return mlnffi_rust_collator_compare(
      collator.get(), lhs.data(), lhs.size(), rhs.data(), rhs.size()
    );
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
  std::unique_ptr<void, decltype(&mlnffi_rust_collator_free)> collator{
    nullptr, mlnffi_rust_collator_free
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

auto formatNumber(
  double number, const std::string& localeId, const std::string& currency,
  std::optional<uint8_t> minFractionDigits,
  std::optional<uint8_t> maxFractionDigits
) -> std::string {
  return take(mlnffi_rust_format_number(
    number, localeId.data(), localeId.size(), currency.data(), currency.size(),
    minFractionDigits ? *minFractionDigits : -1,
    maxFractionDigits ? *maxFractionDigits : -1
  ));
}

}  // namespace mln::platform
