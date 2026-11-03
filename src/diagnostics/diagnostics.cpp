#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>

#include "diagnostics/diagnostics.hpp"

namespace {
constexpr auto diagnostic_capacity = std::size_t{4096};

auto thread_last_error_buffer() noexcept
  -> std::array<char, diagnostic_capacity>& {
  thread_local std::array<char, diagnostic_capacity> value{};
  return value;
}

// The longest prefix of message that fits in capacity - 1 bytes and doesn't
// end inside a UTF-8 sequence, so a truncated message stays valid UTF-8.
auto truncated(const char* message, std::size_t capacity) noexcept
  -> std::string_view {
  const auto full = std::string_view{message};
  if (full.size() < capacity) return full;
  auto length = capacity - 1;
  while (length > 0 &&
         (static_cast<unsigned char>(full[length]) & 0xC0U) == 0x80U) {
    length -= 1;
  }
  return full.substr(0, length);
}
}  // namespace

namespace mln::core {

auto thread_last_error_message() noexcept -> const char* {
  return thread_last_error_buffer().data();
}

auto clear_thread_error() noexcept -> void {
  thread_last_error_buffer().front() = '\0';
}

auto set_thread_error(const char* message) noexcept -> void {
  auto& buffer = thread_last_error_buffer();
  if (message == nullptr) {
    buffer.front() = '\0';
    return;
  }

  const auto result =
    std::ranges::copy(truncated(message, buffer.size()), buffer.begin());
  *result.out = '\0';
}

auto set_thread_error(const std::exception& exception) noexcept -> void {
  set_thread_error(exception.what());
}

auto write_diagnostic(mln_diagnostic* diagnostic, const char* message) noexcept
  -> void {
  constexpr auto message_offset = offsetof(mln_diagnostic, message);
  if (diagnostic == nullptr || diagnostic->size <= message_offset) {
    return;
  }
  const auto capacity = std::min<std::size_t>(
    diagnostic->size - message_offset, sizeof(diagnostic->message)
  );
  const auto result =
    std::ranges::copy(truncated(message, capacity), diagnostic->message);
  *result.out = '\0';
}

}  // namespace mln::core
