// MapLibre's process-global network status, and its effect: offline, a
// runtime answers from its database file. Every case here changes the status,
// so each runs inside restoring_network_status(), which sets it back online
// even when an assertion fails.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "support/harness.h"
#include "support/resources.h"
#include "support/test_support.h"
#include "unity.h"

#if !defined(__EMSCRIPTEN__)
#include "support/http_server.h"
#endif

static void restoring_network_status(void (*body)(void)) {
  if (TEST_PROTECT()) {
    body();
  }
  (void)mln_network_status_set(MLN_NETWORK_STATUS_ONLINE, NULL);
}

#define NETWORK_STATUS_CASE(name)                                   \
  static void name##_body(void);                                    \
  static void name(void) { restoring_network_status(name##_body); } \
  static void name##_body(void)

static uint32_t current_network_status(void) {
  uint32_t status = 0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_network_status_get(&status, NULL));
  return status;
}

NETWORK_STATUS_CASE(the_network_status_round_trips) {
  TEST_ASSERT_EQUAL_UINT32(MLN_NETWORK_STATUS_ONLINE, current_network_status());
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_OFFLINE, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NETWORK_STATUS_OFFLINE, current_network_status()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_OFFLINE, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NETWORK_STATUS_OFFLINE, current_network_status()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_ONLINE, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_NETWORK_STATUS_ONLINE, current_network_status());
}

static const uint32_t unknown_statuses[] = {0, 3, UINT32_MAX};

// A value that names no status is rejected and leaves the status as it was.
NETWORK_STATUS_CASE(the_network_status_rejects_a_value_that_names_no_status) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_OFFLINE, NULL)
  );
  for (size_t index = 0;
       index < sizeof(unknown_statuses) / sizeof(*unknown_statuses);
       index += 1) {
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_network_status_set(unknown_statuses[index], &diagnostic)
    );
    TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "network status"));
    TEST_ASSERT_EQUAL_UINT32(
      MLN_NETWORK_STATUS_OFFLINE, current_network_status()
    );
  }

  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_network_status_get(NULL, &diagnostic)
  );
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "out_status"));
}

#if !defined(__EMSCRIPTEN__)

static const char cached_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static void load_style_with_cache(const char* url, const char* cache_path) {
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = cache_path;
  mln_runtime runtime = mln_test_create_runtime_with_options(options);
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  mln_test_destroy_map(map);
  // The release completes once the runtime has closed its database.
  mln_test_destroy_runtime(runtime);
}

// A style loaded online into a runtime's database file reloads from that file
// in a later runtime while MapLibre is offline. The server is gone by then, so
// only the database can answer.
NETWORK_STATUS_CASE(a_cached_style_reloads_from_the_database_while_offline) {
  static const mln_test_http_route routes[] = {
    {.path = "/cached.json",
     .body = cached_style_json,
     .body_size = sizeof(cached_style_json) - 1},
  };
  char cache_path[1024];
  mln_test_temp_path("offline-reload.db", cache_path, sizeof(cache_path));
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char url[256];
  mln_test_http_server_url(server, "/cached.json", url, sizeof(url));
  load_style_with_cache(url, cache_path);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/cached.json")
  );
  mln_test_http_server_stop(server);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_OFFLINE, NULL)
  );
  load_style_with_cache(url, cache_path);
  (void)remove(cache_path);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(the_network_status_round_trips);
  RUN_TEST(the_network_status_rejects_a_value_that_names_no_status);
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_cached_style_reloads_from_the_database_while_offline);
#endif
}
