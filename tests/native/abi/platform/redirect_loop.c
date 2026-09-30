// A redirect loop on Apple's HTTP client, which reports it as an error with no
// more specific reason. The Rust transport's redirect limit has unit tests of
// its own in src/platform/rust.

#include "support/harness.h"
#include "support/resources.h"
#include "support/test_support.h"
#include "unity.h"

#if defined(__APPLE__)
#include <string.h>

#include "support/http_server.h"

// The client gives up on a style that redirects to itself, and the map
// reports a loading failure rather than waiting on it.
static void a_redirect_loop_fails_the_style_load(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/loop.json",
     .status = 302,
     .headers = "Location: /loop.json\r\n"},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  char url[256];
  mln_test_http_server_url(server, "/loop.json", url, sizeof(url));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_set_style_url(map, url));
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_TRUE(strlen(message) > 0);
  TEST_ASSERT_GREATER_THAN_INT(
    1, mln_test_http_server_requests(server, "/loop.json")
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}
#endif

MLN_TEST_GROUP {
#if defined(__APPLE__)
  RUN_TEST(a_redirect_loop_fails_the_style_load);
#endif
}
