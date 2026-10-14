#include <memory>
#include <optional>
#include <string>

#include <mln/i18n/collator.hpp>

#include "platform/rust/i18n.hpp"

namespace mln::platform {

class Collator::Impl {
 public:
  Impl(
    bool caseSensitive, bool diacriticSensitive,
    const std::optional<std::string>& locale
  )
      : case_sensitive(caseSensitive), diacritic_sensitive(diacriticSensitive) {
    const auto requested = locale.value_or("");
    void* handle = nullptr;
    const rust::Text result{mlnffi_rust_collator_new(
      rust::bytes(requested), requested.size(), caseSensitive,
      diacriticSensitive, &handle
    )};
    collator.reset(handle);
    resolved_locale = result.value();
  }

  [[nodiscard]] auto compare(
    const std::string& lhs, const std::string& rhs
  ) const -> int {
    return mlnffi_rust_collator_compare(
      collator.get(), rust::bytes(lhs), lhs.size(), rust::bytes(rhs), rhs.size()
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

}  // namespace mln::platform
