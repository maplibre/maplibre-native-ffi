#ifndef MLN_NATIVE_TESTS_STYLE_H
#define MLN_NATIVE_TESTS_STYLE_H

// Helpers for the style suites: stepped frames, feature queries whose borrowed
// results are copied out, local resources served through a resource provider,
// and style reads whose results are arrays of borrowed views.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

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

// Copies of the first MLN_TEST_FEATURE_CAPACITY features a query returned.
// Each string is null-terminated and truncated to its buffer.
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

// Queries the features `source_id` holds. `source_layer` names the one source
// layer to read from a vector source, or is null for any other source.
mln_test_feature_list mln_test_style_query_source(
  const mln_test_render_fixture* fixture, const char* source_id,
  const char* source_layer
);

// One resource that mln_test_style_serve() answers. `requests` counts the
// requests for `url`, and the provider pulses after each one.
typedef struct mln_test_style_route {
  const char* url;
  const char* body;
  atomic_int requests;
} mln_test_style_route;

// Installs a resource provider on `runtime` that completes a request for a
// route's URL with its body, and fails every other request as not found, so a
// case that adds URL sources never reaches the network. The routes must
// outlive the runtime.
void mln_test_style_serve(
  mln_runtime runtime, mln_test_style_route* routes, size_t route_count
);

// Copies of the views a list query returned: layer or source IDs, or a layer
// entry's fields. Each string is null-terminated and truncated to its buffer.
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
  mln_test_style_entry entries[MLN_TEST_STYLE_LIST_CAPACITY];
} mln_test_style_list;

mln_test_style_list mln_test_style_list_layer_ids(mln_map map);
mln_test_style_list mln_test_style_list_layers(mln_map map);
mln_test_style_list mln_test_style_list_source_ids(mln_map map);

// Finishes a completion from mln_test_completion_buffer_view(), copies the
// view it delivered into `out` null-terminated, destroys the completion, and
// returns its terminal status. `*out_found` reports whether a value arrived; a
// missing one leaves `out` empty.
mln_status mln_test_style_finish_text(
  mln_test_completion* completion, char* out, size_t capacity, bool* out_found
);

#ifdef __cplusplus
}
#endif

#endif
