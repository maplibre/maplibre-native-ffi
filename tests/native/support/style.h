#ifndef MLN_NATIVE_TESTS_STYLE_H
#define MLN_NATIVE_TESTS_STYLE_H

// Helpers for the style suites: stepped frames, feature queries whose borrowed
// results are copied out, local resources served through a resource provider,
// and style reads whose results are arrays of borrowed views.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "env.h"
#include "maplibre_native_c.h"
#include "render.h"
#include "status.h"
#include "unity.h"
#include "wait.h"

#ifdef __cplusplus
extern "C" {
#endif

// A GeoJSON source with no features, for cases that need a source to exist.
#define MLN_TEST_EMPTY_GEOJSON_SOURCE                               \
  "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\"," \
  "\"features\":[]}}"

static inline mln_buffer_view mln_test_view_of(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

// Runs a command that `expression` submits with `completion.descriptor`,
// expects the submission to succeed and the command to fail with
// `terminal_status`, and expects the failure's diagnostic to contain
// `fragment`.
#define MLN_TEST_EXPECT_COMMAND_FAILED(terminal_status, fragment, expression) \
  do {                                                                        \
    mln_test_completion completion = mln_test_completion_default(0);          \
    TEST_ASSERT_EQUAL_INT_MESSAGE(MLN_STATUS_OK, (expression), #expression);  \
    TEST_ASSERT_EQUAL_INT_MESSAGE(                                            \
      (terminal_status), mln_test_completion_finish(&completion), #expression \
    );                                                                        \
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(                                         \
      MLN_COMMAND_DISPOSITION_FAILED,                                         \
      mln_test_completion_disposition(&completion), #expression               \
    );                                                                        \
    TEST_ASSERT_NOT_NULL_MESSAGE(                                             \
      strstr(mln_test_completion_diagnostic(&completion), (fragment)),        \
      mln_test_completion_diagnostic(&completion)                             \
    );                                                                        \
    mln_test_completion_destroy(&completion);                                 \
  } while (false)

// Expects `expression`, which submits a command with `completion.descriptor`
// and MLN_TEST_DIAGNOSTIC, to reject it as INVALID_ARGUMENT with a diagnostic
// that contains `fragment`. A rejection leaves the completion with the caller
// and never runs its callback.
#define MLN_TEST_EXPECT_COMMAND_REJECTED(fragment, expression)         \
  do {                                                                 \
    mln_test_completion completion = mln_test_completion_default(0);   \
    MLN_TEST_INVALID(expression);                                      \
    TEST_ASSERT_NOT_NULL_MESSAGE(                                      \
      strstr(mln_test_last_error(), (fragment)), mln_test_last_error() \
    );                                                                 \
    TEST_ASSERT_FALSE_MESSAGE(                                         \
      mln_test_completion_poll(&completion), #expression               \
    );                                                                 \
    mln_test_completion_reject(&completion);                           \
    mln_test_completion_destroy(&completion);                          \
  } while (false)

// Requests one frame that renders whether or not the map changed, services the
// fixture until that frame's result arrives, and returns its mln_render_result.
uint32_t mln_test_style_render_frame(const mln_test_render_fixture* fixture);

// Renders frames until `ready(context)` holds after one of them, and reports
// whether it held before the default deadline. Each step waits on a frame
// result, never on elapsed time.
bool mln_test_style_render_until(
  const mln_test_render_fixture* fixture, bool (*ready)(void* context),
  void* context, const char* what
);

// Copies of the features a query returned, each string null-terminated. A
// query that returns more than MLN_TEST_FEATURE_CAPACITY features, or a string
// longer than its buffer, fails the test.
#define MLN_TEST_FEATURE_CAPACITY 8

typedef struct mln_test_feature {
  char feature[512];
  char source_id[64];
  char source_layer_id[64];
  char state[256];
  bool has_state;
} mln_test_feature;

typedef struct mln_test_feature_list {
  mln_status status;
  size_t count;
  mln_test_feature features[MLN_TEST_FEATURE_CAPACITY];
} mln_test_feature_list;

// Queries every feature rendered inside the fixture's whole viewport.
mln_test_feature_list mln_test_style_query_rendered(
  const mln_test_render_fixture* fixture
);

// Queries the features rendered along `geometry` with `options`, which may be
// null.
mln_test_feature_list mln_test_style_query_rendered_with(
  const mln_test_render_fixture* fixture,
  const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options
);

// Queries the features `source_id` holds. `source_layer` names the one source
// layer to read from a vector source, or is null for any other source.
mln_test_feature_list mln_test_style_query_source(
  const mln_test_render_fixture* fixture, const char* source_id,
  const char* source_layer
);

// The same with the caller's options, which may be null.
mln_test_feature_list mln_test_style_query_source_with(
  const mln_test_render_fixture* fixture, const char* source_id,
  const mln_source_feature_query_options* options
);

// One resource that mln_test_style_serve() answers. `requests` counts the
// requests for `url`. The provider counts and pulses before it completes a
// request, so a case that observes the response also observes the count.
typedef struct mln_test_style_route {
  const char* url;
  const char* body;
  atomic_int requests;
} mln_test_style_route;

// Installs a resource provider on `runtime` that completes a request for a
// route's URL with its body, and fails every other request as not found, so a
// case that adds URL sources never reaches the network. The routes need static
// storage: a failing case leaves its runtime for the harness to reclaim, and
// the provider can still run until then.
void mln_test_style_serve(
  mln_runtime runtime, mln_test_style_route* routes, size_t route_count
);

// Copies of the views a list query returned: layer or source IDs, or a layer
// entry's fields, each string null-terminated. A list longer than
// MLN_TEST_STYLE_LIST_CAPACITY, or a string longer than its buffer, fails the
// test.
#define MLN_TEST_STYLE_LIST_CAPACITY 8

typedef struct mln_test_style_entry {
  char id[64];
  char type[32];
  char source_id[64];
  char source_layer[64];
} mln_test_style_entry;

typedef struct mln_test_style_list {
  mln_status status;
  size_t count;
  // The completion's value_size, the stride that the entries were read by.
  uint32_t value_size;
  mln_test_style_entry entries[MLN_TEST_STYLE_LIST_CAPACITY];
} mln_test_style_list;

mln_test_style_list mln_test_style_list_layer_ids(mln_map map);
mln_test_style_list mln_test_style_list_layers(mln_map map);
mln_test_style_list mln_test_style_list_source_ids(mln_map map);

// Finishes a completion from mln_test_completion_buffer_view(), copies the
// view it delivered into `out` null-terminated, destroys the completion, and
// returns its terminal status. `*out_found` reports whether a value arrived; a
// missing one leaves `out` empty. A value longer than `capacity - 1` bytes
// fails the test.
mln_status mln_test_style_finish_text(
  mln_test_completion* completion, char* out, size_t capacity, bool* out_found
);

#ifdef __cplusplus
}
#endif

#endif
