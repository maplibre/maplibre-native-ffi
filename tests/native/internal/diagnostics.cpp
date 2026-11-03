// Diagnostics hold at most 4095 bytes of message. Callers decode them, and
// the bindings' caller-sized text, as UTF-8, so a message cut at the limit
// must not end inside a multibyte character. No public call takes a message
// long enough to reach the limit, so these cases call the writers directly.

#include <cstddef>
#include <cstring>
#include <string>

#include "diagnostics/diagnostics.hpp"

#include "maplibre_native_c.h"
#include "support/test_support.h"

namespace {

constexpr auto message_limit = sizeof(mln_diagnostic{}.message) - 1;

// message_limit - 1 ASCII bytes, then a two-byte character that straddles
// the limit, then more text.
auto straddling_message() -> std::string {
  return std::string(message_limit - 1, 'a') + "\xC3\xA9" + "tail";
}

void truncation_keeps_whole_utf8_characters() {
  const auto message = straddling_message();

  auto diagnostic = mln_diagnostic{.size = sizeof(mln_diagnostic)};
  mln::core::write_diagnostic(&diagnostic, message.c_str());
  TEST_ASSERT_EQUAL_size_t(message_limit - 1, std::strlen(diagnostic.message));

  mln::core::set_thread_error(message.c_str());
  TEST_ASSERT_EQUAL_size_t(
    message_limit - 1, std::strlen(mln::core::thread_last_error_message())
  );
}

void a_message_that_fits_is_kept_whole() {
  const auto message = std::string(message_limit - 2, 'a') + "\xC3\xA9";

  auto diagnostic = mln_diagnostic{.size = sizeof(mln_diagnostic)};
  mln::core::write_diagnostic(&diagnostic, message.c_str());
  TEST_ASSERT_EQUAL_STRING(message.c_str(), diagnostic.message);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(truncation_keeps_whole_utf8_characters);
  RUN_TEST(a_message_that_fits_is_kept_whole);
}
