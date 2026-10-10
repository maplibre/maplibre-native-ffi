// Stepped frames, copied feature queries, served resources, and copied list
// reads for the style suites.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "style.h"

#include "env.h"
#include "render.h"
#include "status.h"
#include "unity.h"
#include "wait.h"

// Copies `view` into `out` null-terminated, and reports whether it fit. A view
// that does not fit leaves `out` truncated. Completion callbacks can run off
// the test thread, so callers record a miss and fail on the test thread.
static bool copy_text(mln_buffer_view view, char* out, size_t capacity) {
  const bool fits = view.size < capacity;
  const size_t size = fits ? view.size : capacity - 1;
  if (size != 0) {
    memcpy(out, view.data, size);
  }
  out[size] = '\0';
  return fits;
}

static const char overflow_message[] =
  "a copied result is larger than its test buffer; raise the capacity";

typedef struct frame_wait {
  mln_render_session session;
  uint64_t token;
  bool found;
  uint32_t disposition;
} frame_wait;

static bool frame_arrived(void* context) {
  frame_wait* wait = context;
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  if (
    mln_render_session_drain_frame_results(wait->session, &batch, NULL) !=
    MLN_STATUS_OK
  ) {
    return wait->found;
  }
  mln_render_frame_batch_view view = {
    .size = sizeof(mln_render_frame_batch_view)
  };
  // A failed read leaves the view empty.
  (void)mln_render_frame_batch_get(batch, &view, NULL);
  for (size_t index = 0; index < view.result_count; index += 1) {
    const mln_render_frame_result* result =
      (const mln_render_frame_result*)((const char*)view.results +
                                       (index * view.result_size));
    if (result->token == wait->token) {
      wait->found = true;
      wait->disposition = result->disposition;
    }
  }
  mln_render_frame_batch_release(batch);
  return wait->found;
}

uint32_t mln_test_style_render_frame(const mln_test_render_fixture* fixture) {
  static atomic_uint_fast64_t next_token = 1;
  frame_wait wait = {
    .session = fixture->session,
    .token = atomic_fetch_add(&next_token, 1),
  };
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = wait.token;
  MLN_TEST_OK(mln_render_session_request_frame(
    fixture->session, &demand, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK_MESSAGE(
    mln_test_render_step_until(
      fixture, frame_arrived, &wait, mln_test_deadline_default(),
      "a frame result"
    ),
    "the frame never produced a result"
  );
  return wait.disposition;
}

bool mln_test_style_render_until(
  const mln_test_render_fixture* fixture, bool (*ready)(void* context),
  void* context, const char* what
) {
  const mln_test_deadline deadline = mln_test_deadline_default();
  while (!mln_test_deadline_passed(deadline)) {
    (void)mln_test_style_render_frame(fixture);
    if (ready(context)) {
      return true;
    }
  }
  TEST_MESSAGE(what);
  return false;
}

// Heap-allocated so a query delivered after its wait gave up writes into
// memory the helper leaked, not into a returned stack frame.
typedef struct feature_probe {
  atomic_bool done;
  bool overflowed;
  mln_test_feature_list list;
} feature_probe;

static void copy_features(
  void* user_data, const mln_completion_result* result
) {
  feature_probe* probe = user_data;
  probe->list.status = result->status;
  probe->list.count = result->value_count;
  probe->overflowed = result->value_count > MLN_TEST_FEATURE_CAPACITY;
  const mln_queried_feature* features = result->value;
  bool fits = true;
  for (size_t index = 0;
       index < result->value_count && index < MLN_TEST_FEATURE_CAPACITY;
       index += 1) {
    const mln_queried_feature* source = &features[index];
    mln_test_feature* copy = &probe->list.features[index];
    fits &= copy_text(source->feature, copy->feature, sizeof(copy->feature));
    if ((source->fields & MLN_QUERIED_FEATURE_SOURCE_ID) != 0) {
      fits &=
        copy_text(source->source_id, copy->source_id, sizeof(copy->source_id));
    }
    if ((source->fields & MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0) {
      fits &= copy_text(
        source->source_layer_id, copy->source_layer_id,
        sizeof(copy->source_layer_id)
      );
    }
    copy->has_state = (source->fields & MLN_QUERIED_FEATURE_STATE) != 0;
    if (copy->has_state) {
      fits &= copy_text(source->state, copy->state, sizeof(copy->state));
    }
  }
  probe->overflowed |= !fits;
  mln_test_flag_set(&probe->done);
}

static bool probe_done(void* context) {
  return atomic_load(&((feature_probe*)context)->done);
}

static feature_probe* new_feature_probe(mln_completion* out_completion) {
  feature_probe* probe = calloc(1, sizeof(*probe));
  TEST_ASSERT_NOT_NULL(probe);
  atomic_init(&probe->done, false);
  probe->list.status = MLN_STATUS_INVALID_STATE;
  *out_completion = (mln_completion){
    .size = sizeof(mln_completion),
    .callback = copy_features,
    .user_data = probe,
  };
  return probe;
}

static mln_test_feature_list finish_feature_probe(
  const mln_test_render_fixture* fixture, feature_probe* probe
) {
  MLN_TEST_OK_MESSAGE(
    mln_test_render_step_until(
      fixture, probe_done, probe, mln_test_deadline_default(), "a feature query"
    ),
    "the feature query never completed"
  );
  const mln_test_feature_list list = probe->list;
  const bool overflowed = probe->overflowed;
  free(probe);
  TEST_ASSERT_FALSE_MESSAGE(overflowed, overflow_message);
  return list;
}

mln_test_feature_list mln_test_style_query_rendered(
  const mln_test_render_fixture* fixture
) {
  const mln_rendered_query_geometry geometry =
    mln_rendered_query_geometry_box((mln_screen_box){
      .min = {.x = -4096.0, .y = -4096.0},
      .max = {.x = 4096.0, .y = 4096.0},
    });
  return mln_test_style_query_rendered_with(fixture, &geometry, NULL);
}

mln_test_feature_list mln_test_style_query_rendered_with(
  const mln_test_render_fixture* fixture,
  const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options
) {
  mln_completion completion;
  feature_probe* probe = new_feature_probe(&completion);
  const mln_status status = mln_render_session_query_rendered_features(
    fixture->session, geometry, options, &completion, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) {
    free(probe);
    MLN_TEST_OK_MESSAGE(status, mln_test_last_error());
  }
  return finish_feature_probe(fixture, probe);
}

mln_test_feature_list mln_test_style_query_source(
  const mln_test_render_fixture* fixture, const char* source_id,
  const char* source_layer
) {
  mln_source_feature_query_options options =
    mln_source_feature_query_options_default();
  mln_buffer_view layer = {0};
  if (source_layer != NULL) {
    layer =
      (mln_buffer_view){.data = source_layer, .size = strlen(source_layer)};
    options.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
    options.source_layer_ids = &layer;
    options.source_layer_id_count = 1;
  }
  return mln_test_style_query_source_with(fixture, source_id, &options);
}

mln_test_feature_list mln_test_style_query_source_with(
  const mln_test_render_fixture* fixture, const char* source_id,
  const mln_source_feature_query_options* options
) {
  mln_completion completion;
  feature_probe* probe = new_feature_probe(&completion);
  const mln_status status = mln_render_session_query_source_features(
    fixture->session,
    (mln_buffer_view){.data = source_id, .size = strlen(source_id)}, options,
    &completion, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) {
    free(probe);
    MLN_TEST_OK_MESSAGE(status, mln_test_last_error());
  }
  return finish_feature_probe(fixture, probe);
}

static mln_test_style_route* find_route(
  mln_test_style_route* routes, size_t route_count, const char* url
) {
  for (size_t index = 0; url != NULL && index < route_count; index += 1) {
    if (strcmp(routes[index].url, url) == 0) {
      return &routes[index];
    }
  }
  return NULL;
}

typedef struct served_routes {
  mln_test_style_route* routes;
  size_t count;
} served_routes;

static uint32_t serve_route(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  const served_routes* served = user_data;
  mln_test_style_route* route =
    find_route(served->routes, served->count, request->requested_url);
  mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
    .error_message = "the style suite serves no such resource",
  };
  if (route != NULL) {
    response = (mln_resource_response){
      .size = sizeof(mln_resource_response),
      .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
      .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
      .bytes = (const uint8_t*)route->body,
      .byte_count = strlen(route->body),
    };
  }
  // Count before completing: the response can reach a query before this
  // thread runs again.
  if (route != NULL) {
    atomic_fetch_add(&route->requests, 1);
  }
  mln_test_pulse();
  (void)mln_resource_request_complete(handle, &response, NULL);
  mln_resource_request_release(handle);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static void release_served_routes(void* user_data) { free(user_data); }

void mln_test_style_serve(
  mln_runtime runtime, mln_test_style_route* routes, size_t route_count
) {
  served_routes* served = malloc(sizeof(*served));
  TEST_ASSERT_NOT_NULL(served);
  *served = (served_routes){.routes = routes, .count = route_count};
  for (size_t index = 0; index < route_count; index += 1) {
    atomic_init(&routes[index].requests, 0);
  }
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = serve_route,
    .user_data = served,
    .release_user_data = release_served_routes,
  };
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_provider(
    runtime, &provider, &completion.descriptor, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) {
    free(served);
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
    TEST_FAIL_MESSAGE(mln_test_last_error());
  }
  MLN_TEST_OK(mln_test_completion_settle(&completion));
}

typedef struct list_probe {
  atomic_bool done;
  bool entries;
  bool overflowed;
  mln_test_style_list list;
} list_probe;

static void copy_list(void* user_data, const mln_completion_result* result) {
  list_probe* probe = user_data;
  probe->list.status = result->status;
  probe->list.count = result->value_count;
  bool fits = result->value_count <= MLN_TEST_STYLE_LIST_CAPACITY;
  for (size_t index = 0;
       index < result->value_count && index < MLN_TEST_STYLE_LIST_CAPACITY;
       index += 1) {
    mln_test_style_entry* copy = &probe->list.entries[index];
    if (probe->entries) {
      const mln_style_layer_entry* entry =
        &((const mln_style_layer_entry*)result->value)[index];
      fits &= copy_text(entry->id, copy->id, sizeof(copy->id));
      fits &= copy_text(entry->type, copy->type, sizeof(copy->type));
      fits &=
        copy_text(entry->source_id, copy->source_id, sizeof(copy->source_id));
      fits &= copy_text(
        entry->source_layer, copy->source_layer, sizeof(copy->source_layer)
      );
    } else {
      fits &= copy_text(
        ((const mln_buffer_view*)result->value)[index], copy->id,
        sizeof(copy->id)
      );
    }
  }
  probe->overflowed = !fits;
  mln_test_flag_set(&probe->done);
}

typedef mln_status (*list_query)(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
);

static mln_test_style_list run_list_query(
  mln_map map, list_query query, bool entries
) {
  list_probe* probe = calloc(1, sizeof(*probe));
  TEST_ASSERT_NOT_NULL(probe);
  atomic_init(&probe->done, false);
  probe->entries = entries;
  probe->list.status = MLN_STATUS_INVALID_STATE;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_list,
    .user_data = probe,
  };
  MLN_TEST_OK(query(map, &completion, MLN_TEST_DIAGNOSTIC));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&probe->done), "the list query never completed"
  );
  const mln_test_style_list list = probe->list;
  const bool overflowed = probe->overflowed;
  free(probe);
  TEST_ASSERT_FALSE_MESSAGE(overflowed, overflow_message);
  return list;
}

mln_test_style_list mln_test_style_list_layer_ids(mln_map map) {
  return run_list_query(map, mln_map_list_style_layer_ids, false);
}

mln_test_style_list mln_test_style_list_layers(mln_map map) {
  return run_list_query(map, mln_map_list_style_layers, true);
}

mln_test_style_list mln_test_style_list_source_ids(mln_map map) {
  return run_list_query(map, mln_map_list_style_source_ids, false);
}

mln_status mln_test_style_finish_text(
  mln_test_completion* completion, char* out, size_t capacity, bool* out_found
) {
  out[0] = '\0';
  const mln_status status = mln_test_completion_finish(completion);
  const bool found = mln_test_completion_value_count(completion) == 1;
  if (status == MLN_STATUS_OK && found) {
    mln_buffer_view view = {0};
    TEST_ASSERT_TRUE(
      mln_test_completion_copy_value(completion, &view, sizeof(view))
    );
    const bool fits = copy_text(view, out, capacity);
    mln_test_completion_destroy(completion);
    TEST_ASSERT_TRUE_MESSAGE(fits, overflow_message);
  } else {
    mln_test_completion_destroy(completion);
  }
  if (out_found != NULL) {
    *out_found = found;
  }
  return status;
}
