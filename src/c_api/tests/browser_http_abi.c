// The browser's HTTP transport, end to end: a style loaded from a real origin.
//
// This is the one test that puts a request on the network; everything else in
// the suite reads embedded fixtures or answers through a resource provider. The
// origin comes from the browser runner, which serves the style document, so
// this covers the browser target alone.

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

#if defined(__EMSCRIPTEN__)

#include <maplibre_native_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The layer id the runner's document carries, which identifies the response as
// the served one.
static const char fixture_layer_id[] = "http-fixture";

// Waits for the style to load, or reports the failure the map produced instead.
// A transport that never answers reports neither, so the timeout is the failure
// this test catches.
static bool wait_for_style_loaded(
  mln_runtime runtime, mln_map map, char* out_message, size_t capacity
) {
  out_message[0] = '\0';
  for (unsigned int attempt = 0; attempt < 600; attempt += 1) {
    if (mln_test_runtime_barrier(runtime) != MLN_STATUS_OK) {
      return false;
    }
    while (true) {
      mln_test_event_batch batch = mln_test_event_batch_default();
      if (mln_test_drain_events(runtime, &batch) != MLN_STATUS_OK) {
        return false;
      }
      if (batch.event_count == 0) {
        break;
      }
      for (size_t index = 0; index < batch.event_count; index += 1) {
        const mln_runtime_event* event =
          (const mln_runtime_event*)((const char*)batch.events +
                                     (index * batch.event_size));
        if (event->source != map) {
          continue;
        }
        if (event->type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
          return true;
        }
        if (
          event->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED &&
          event->message_size < capacity
        ) {
          memcpy(
            out_message, batch.messages + event->message_offset,
            event->message_size
          );
          out_message[event->message_size] = '\0';
          return false;
        }
      }
    }
    mln_test_sleep_millisecond();
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
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_set_style_url(map, url));

  char failure[512];
  const bool loaded =
    wait_for_style_loaded(runtime, map, failure, sizeof(failure));
  if (!loaded) {
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
    TEST_FAIL_MESSAGE(failure);
    return;
  }

  // Checks which document loaded: a response from anywhere else lacks this
  // layer.
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_layer_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_layer_info(
      map, mln_test_buffer_view(fixture_layer_id, strlen(fixture_layer_id)),
      &completion.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  mln_test_completion_destroy(&completion);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  if (!found) {
    char message[512];
    (void)snprintf(
      message, sizeof(message),
      "the loaded style has no %s layer, so it did not come from the runner's "
      "document",
      fixture_layer_id
    );
    TEST_FAIL_MESSAGE(message);
  }
}

void run_browser_http_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_loads_over_http_from_the_runner_origin);
}

#else

// Only the browser runner hosts the origin this test needs.
void run_browser_http_abi_tests(void) {}

#endif
