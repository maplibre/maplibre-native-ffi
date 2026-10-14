#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

extern "C" {

struct MlnRustText {
  uint8_t* data;
  std::size_t length;
  bool failed;
};

void mlnffi_rust_text_free(MlnRustText text);
auto mlnffi_rust_collator_new(
  const uint8_t* locale, std::size_t length, bool caseSensitive,
  bool diacriticSensitive, void** output
) -> MlnRustText;
void mlnffi_rust_collator_free(void* collator);
auto mlnffi_rust_collator_compare(
  const void* collator, const uint8_t* lhs, std::size_t lhsLength,
  const uint8_t* rhs, std::size_t rhsLength
) -> int32_t;
auto mlnffi_rust_format_number(
  double number, const uint8_t* locale, std::size_t localeLength,
  const uint8_t* currency, std::size_t currencyLength, int16_t min, int16_t max
) -> MlnRustText;

}  // extern "C"

namespace mln::platform::rust {

inline auto bytes(const std::string& text) -> const uint8_t* {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const uint8_t*>(text.data());
}

class Text {
 public:
  explicit Text(MlnRustText text_) : text(text_) {}
  Text(const Text&) = delete;
  auto operator=(const Text&) -> Text& = delete;
  Text(Text&&) = delete;
  auto operator=(Text&&) -> Text& = delete;
  ~Text() { mlnffi_rust_text_free(text); }

  [[nodiscard]] auto value() const -> std::string {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string result(reinterpret_cast<const char*>(text.data), text.length);
    if (text.failed) {
      throw std::runtime_error(result);
    }
    return result;
  }

 private:
  MlnRustText text;
};

}  // namespace mln::platform::rust
