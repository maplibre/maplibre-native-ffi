#include <cstdint>
#include <optional>
#include <string>

#include <mln/i18n/number_format.hpp>

#include "platform/rust/i18n.hpp"

namespace mln::platform {

auto formatNumber(
  double number, const std::string& locale, const std::string& currency,
  std::optional<uint8_t> minFractionDigits,
  std::optional<uint8_t> maxFractionDigits
) -> std::string {
  return rust::Text{mlnffi_rust_format_number(
                      number, rust::bytes(locale), locale.size(),
                      rust::bytes(currency), currency.size(),
                      minFractionDigits ? *minFractionDigits : -1,
                      maxFractionDigits ? *maxFractionDigits : -1
                    )}
    .value();
}

}  // namespace mln::platform
