// The browser's HTTP transport, end to end: a style loaded from a real origin.
//
// This is the one test that puts a request on the network. Everything else in
// the suite reads embedded fixtures or answers through a resource provider,
// which is how a transport that reached the server and dropped every response
// stayed invisible: emscripten_fetch reports through the calling thread's event
// loop, and every MapLibre thread parks in its run loop instead of returning to
// one. See src/platform/emscripten/http_file_source.cpp.
//
// The origin comes from the runner, which serves the style document; there is
// no server on the other targets, so this covers the browser alone.

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

#if defined(__EMSCRIPTEN__)

#include <maplibre_native_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The layer id the runner's document carries, which is what says the response
// came from the server rather than from anywhere else.
static const char fixture_layer_id[] = "http-fixture";

static mln_runtime_event empty_event(void) {
  mln_runtime_event event = {
    .size = sizeof(mln_runtime_event),
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_NONE,
  };
  return event;
}

// Waits for the style to load, or reports the failure the map produced instead.
// A transport that never answers reports neither, so the timeout is the failure
// this test exists to catch.
static bool wait_for_style_loaded(
  mln_runtime runtime, mln_map map, char* out_message, size_t capacity
) {
  out_message[0] = '\0';
  for (unsigned int attempt = 0; attempt < 600; attempt += 1) {
    if (mln_runtime_pump(runtime, 10) != MLN_STATUS_OK) {
      return false;
    }
    while (true) {
      mln_runtime_event event = empty_event();
      bool has_event = false;
      if (
        mln_runtime_poll_event(runtime, &event, &has_event) != MLN_STATUS_OK
      ) {
        return false;
      }
      if (!has_event) {
        break;
      }
      if (event.source != map) {
        continue;
      }
      if (event.type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
        return true;
      }
      if (
        event.type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED &&
        event.message != NULL && event.message_size < capacity
      ) {
        memcpy(out_message, event.message, event.message_size);
        out_message[event.message_size] = '\0';
        return false;
      }
    }
  }
  snprintf(
    out_message, capacity,
    "no style-loaded or loading-failed event arrived; the transport answered "
    "nothing"
  );
  return false;
}

static void style_loads_over_http_from_the_runner_origin(void) {
  const char* origin = getenv("MLN_FFI_TEST_FIXTURE_ORIGIN");
  TEST_ASSERT_TRUE_MESSAGE(
    origin != NULL && origin[0] != '\0',
    "MLN_FFI_TEST_FIXTURE_ORIGIN is unset; run the suite through ctest"
  );

  char url[512];
  const int written =
    snprintf(url, sizeof(url), "%s/__fixture/http-style.json", origin);
  TEST_ASSERT_TRUE(written > 0 && (size_t)written < sizeof(url));

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_set_style_url(map, url));

  char failure[512];
  const bool loaded =
    wait_for_style_loaded(runtime, map, failure, sizeof(failure));
  if (!loaded) {
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
    TEST_FAIL_MESSAGE(failure);
    return;
  }

  // Which document loaded, not merely that one did: the runner answers this
  // path alone, so a response from anywhere else carries a different layer.
  mln_style_id_list layers = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_list_style_layer_ids(map, &layers)
  );
  size_t count = 0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_style_id_list_count(layers, &count));

  bool found = false;
  char seen[256];
  seen[0] = '\0';
  for (size_t index = 0; index < count; index += 1) {
    mln_string_view id = {0};
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_style_id_list_get(layers, index, &id)
    );
    if (
      id.size == strlen(fixture_layer_id) &&
      memcmp(id.data, fixture_layer_id, id.size) == 0
    ) {
      found = true;
    }
    const size_t used = strlen(seen);
    (void)snprintf(
      seen + used, sizeof(seen) - used, "%s%.*s", used == 0 ? "" : ", ",
      (int)id.size, id.data
    );
  }
  mln_style_id_list_destroy(layers);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  if (!found) {
    char message[512];
    (void)snprintf(
      message, sizeof(message),
      "the loaded style has no %s layer, so it did not come from the runner's "
      "document; layers: [%s]",
      fixture_layer_id, seen
    );
    TEST_FAIL_MESSAGE(message);
  }
}

void run_browser_http_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_loads_over_http_from_the_runner_origin);
}

#else

// Reaching a real origin needs the server the browser runner hosts, so the
// other targets cover HTTP through their own platform's transport tests.
void run_browser_http_abi_tests(void) {}

#endif
