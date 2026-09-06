// Raw C ABI coverage: null handles/outputs, unknown operation values, null
// paths, callback descriptor shape, and invalid offline unions are hidden by
// bindings, plus the network file source diagnostic that every binding
// forwards verbatim.

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static const char offline_style_url[] = "http://example.com/offline-style.json";
static const char lookup_blocking_style_url[] =
  "http://example.com/lookup-blocking-style.json";
static const char lookup_probe_style_url[] =
  "http://example.com/lookup-probe-style.json";
static const char unsupported_scheme_style_url[] =
  "jar:file:/packaged/style.json";
static const char credentialed_unsupported_scheme_style_url[] =
  "jar://user:password@archive/packaged/style.json?access_token=secret#token";
static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static mln_status set_resource_provider_committed(
  mln_runtime runtime, const mln_resource_provider* provider
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_provider(
    runtime, provider, &completion.descriptor
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(&completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(&completion) : status;
  mln_test_completion_destroy(&completion);
  return result;
}

static mln_status clear_resource_provider_committed(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status =
    mln_runtime_clear_resource_provider(runtime, &completion.descriptor);
  if (status != MLN_STATUS_OK) mln_test_completion_reject(&completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(&completion) : status;
  mln_test_completion_destroy(&completion);
  return result;
}

static mln_status set_resource_transform_committed(
  mln_runtime runtime, const mln_resource_transform* transform
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_transform(
    runtime, transform, &completion.descriptor
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(&completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(&completion) : status;
  mln_test_completion_destroy(&completion);
  return result;
}

static mln_status clear_resource_transform_committed(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status =
    mln_runtime_clear_resource_transform(runtime, &completion.descriptor);
  if (status != MLN_STATUS_OK) mln_test_completion_reject(&completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(&completion) : status;
  mln_test_completion_destroy(&completion);
  return result;
}

static mln_status set_http_header_transform_committed(
  mln_runtime runtime, const mln_http_header_transform* transform
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_http_header_transform(
    runtime, transform, &completion.descriptor
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(&completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(&completion) : status;
  mln_test_completion_destroy(&completion);
  return result;
}

static mln_status clear_http_header_transform_committed(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status =
    mln_runtime_clear_http_header_transform(runtime, &completion.descriptor);
  if (status != MLN_STATUS_OK) mln_test_completion_reject(&completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(&completion) : status;
  mln_test_completion_destroy(&completion);
  return result;
}

static bool wait_for_offline_completion(
  mln_runtime runtime, mln_test_completion* completion
) {
  (void)runtime;
  return mln_test_completion_wait(completion, 5000);
}

static bool wait_for_map_loading_failure(
  mln_runtime runtime, const mln_map map, char* out_message,
  size_t out_message_capacity
) {
  for (size_t attempt = 0; attempt < 5000; attempt += 1) {
    if (mln_test_runtime_barrier(runtime) != MLN_STATUS_OK) {
      return false;
    }
    mln_runtime_event event = {0};
    if (
      mln_test_drain_find(
        runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, &event, out_message,
        out_message_capacity
      )
    ) {
      return event.message_size < out_message_capacity;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

static mln_offline_region_definition offline_tile_definition_for_style(
  const char* style_url
) {
  return (mln_offline_region_definition){
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data = {
      .tile_pyramid = {
        .size = sizeof(mln_offline_tile_pyramid_region_definition),
        .style_url = style_url,
        .bounds =
          {
            .southwest = {.latitude = 1.0, .longitude = 2.0},
            .northeast = {.latitude = 3.0, .longitude = 4.0},
          },
        .min_zoom = 5.0,
        .max_zoom = 6.0,
        .pixel_ratio = 2.0,
        .include_ideographs = true,
      }
    },
  };
}

static mln_offline_region_definition offline_tile_definition(void) {
  return offline_tile_definition_for_style(offline_style_url);
}

static mln_offline_region_definition offline_geometry_definition(
  mln_buffer_view geometry
) {
  return (mln_offline_region_definition){
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
    .data = {
      .geometry = {
        .size = sizeof(mln_offline_geometry_region_definition),
        .style_url = offline_style_url,
        .geometry = geometry,
        .min_zoom = 5.0,
        .max_zoom = 6.0,
        .pixel_ratio = 2.0,
        .include_ideographs = true,
      }
    },
  };
}

static bool create_and_activate_offline_region(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  mln_offline_region_id* out_region_id
) {
  const uint8_t metadata[] = {1, 2, 3};
  mln_test_completion creation =
    mln_test_completion_default(sizeof(mln_offline_region_info));
  const mln_status submission = mln_runtime_offline_region_create(
    runtime, definition, metadata, sizeof(metadata), &creation.descriptor
  );
  if (submission != MLN_STATUS_OK) {
    mln_test_completion_reject(&creation);
    mln_test_completion_destroy(&creation);
    return false;
  }
  if (
    !wait_for_offline_completion(runtime, &creation) ||
    mln_test_completion_status(&creation) != MLN_STATUS_OK
  ) {
    mln_test_completion_destroy(&creation);
    return false;
  }
  mln_offline_region_info info = {.size = sizeof(mln_offline_region_info)};
  if (!mln_test_completion_copy_value(&creation, &info, sizeof(info))) {
    mln_test_completion_destroy(&creation);
    return false;
  }
  mln_test_completion_destroy(&creation);

  mln_completion download = mln_test_discard_completion();
  if (
    mln_runtime_offline_region_set_download_state(
      runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &download
    ) != MLN_STATUS_OK
  ) {
    return false;
  }
  if (out_region_id != NULL) *out_region_id = info.id;
  return true;
}

static mln_resource_response style_response(void) {
  return (mln_resource_response){
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
  };
}

static uint32_t resource_provider_stub(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)user_data;
  (void)request;
  (void)handle;
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

static void count_runtime_callback_release(void* user_data) {
  atomic_fetch_add((atomic_int*)user_data, 1);
}

static void resource_provider_registration_releases_owned_state(void) {
  atomic_int release_count;
  atomic_init(&release_count, 0);
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = resource_provider_stub,
    .user_data = &release_count,
    .release_user_data = count_runtime_callback_release,
  };

  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_resource_provider(MLN_HANDLE_NULL, &provider, &rejected)
  );
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&release_count));

  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&release_count));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&release_count));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, clear_resource_provider_committed(runtime)
  );
  TEST_ASSERT_EQUAL_INT(2, atomic_load(&release_count));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  mln_test_destroy_runtime(runtime);
  for (size_t attempt = 0; attempt < 10000 && atomic_load(&release_count) < 3;
       attempt += 1) {
    mln_test_sleep_millisecond();
  }
  TEST_ASSERT_EQUAL_INT(3, atomic_load(&release_count));
}

typedef struct inline_release_provider_state {
  atomic_bool callback_finished;
  atomic_int completion_status;
} inline_release_provider_state;

static uint32_t inline_release_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  inline_release_provider_state* state = user_data;
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = inline_style_json,
    .byte_count = sizeof(inline_style_json) - 1,
  };
  (void)request;
  atomic_store(
    &state->completion_status, mln_resource_request_complete(handle, &response)
  );
  mln_resource_request_release(handle);
  atomic_store(&state->callback_finished, true);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static mln_status resource_transform_stub(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  (void)user_data;
  (void)kind;
  (void)url;
  if (out_response == NULL) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_response->url = NULL;
  return MLN_STATUS_OK;
}

static mln_status http_header_transform_stub(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) {
  (void)user_data;
  (void)kind;
  (void)url;
  return out_response == NULL ? MLN_STATUS_INVALID_ARGUMENT : MLN_STATUS_OK;
}

static void ignore_cancel(void* user_data) { (void)user_data; }

static void custom_provider_request_handles_reject_raw_null_handles(void) {
  mln_resource_request_release(MLN_HANDLE_NULL);
  const mln_resource_response response = style_response();
  bool cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_cancelled(MLN_HANDLE_NULL, &cancelled)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_complete(MLN_HANDLE_NULL, &response)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_set_cancel_callback(
      MLN_HANDLE_NULL, ignore_cancel, NULL, &cancelled
    )
  );
}

static void network_status_get_rejects_raw_null_output(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_network_status_get(NULL)
  );
}

static void ambient_cache_operations_validate_raw_operation_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_run_ambient_cache_operation(
      runtime, (mln_ambient_cache_operation)999, &completion
    )
  );
  mln_test_destroy_runtime(runtime);
}

static void set_maximum_ambient_cache_size_rejects_raw_null_output(void) {
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_maximum_ambient_cache_size(runtime, 1024, NULL)
  );
  mln_test_destroy_runtime(runtime);
}

static void offline_regions_reject_raw_invalid_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_offline_region_definition definition = offline_tile_definition();
  const uint8_t metadata[] = {1, 2, 3};
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, NULL, metadata, sizeof(metadata), &completion
    )
  );

  definition.type = (mln_offline_region_definition_type)999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &completion
    )
  );

  definition = offline_tile_definition();
  definition.data.tile_pyramid.style_url = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &completion
    )
  );

  const mln_buffer_view geometry = MLN_BUFFER_LITERAL(
    "{\"type\":\"LineString\",\"coordinates\":[[2,1],[4,3]]}"
  );
  definition = offline_geometry_definition(geometry);
  definition.data.geometry.style_url = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &completion
    )
  );

  definition = offline_geometry_definition(geometry);
  definition.data.geometry.geometry = (mln_buffer_view){0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &completion
    )
  );
  mln_test_destroy_runtime(runtime);
}

static void offline_database_merge_rejects_raw_null_path(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_regions_merge_database(runtime, NULL, &completion)
  );
  mln_test_destroy_runtime(runtime);
}

static void offline_database_merge_rejects_a_missing_file(void) {
  static const char missing_path[] = "mln-ffi-missing-offline-side-database.db";
  (void)remove(missing_path);

  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_offline_regions_merge_database(
                                   runtime, missing_path, &completion
                                 )
  );
  // The rejection happens on the calling thread, so its diagnostic is readable
  // here rather than through the completion.
  TEST_ASSERT_NOT_NULL(
    strstr(mln_thread_last_error_message(), "readable database file")
  );
  FILE* unexpected = fopen(missing_path, "rb");
  TEST_ASSERT_NULL_MESSAGE(
    unexpected, "The rejected merge created its missing side database."
  );
  if (unexpected != NULL) {
    fclose(unexpected);
    (void)remove(missing_path);
  }
  mln_test_destroy_runtime(runtime);
}

static void offline_database_merge_rejects_sqlite_pseudo_paths(void) {
  static const char* const pseudo_paths[] = {"", ":memory:", "file::memory:"};
  mln_runtime runtime = mln_test_create_runtime();

  for (size_t index = 0; index < sizeof(pseudo_paths) / sizeof(pseudo_paths[0]);
       ++index) {
    mln_completion completion = mln_test_discard_completion();
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_INVALID_ARGUMENT, mln_runtime_offline_regions_merge_database(
                                     runtime, pseudo_paths[index], &completion
                                   )
    );
    TEST_ASSERT_NOT_NULL(
      strstr(mln_thread_last_error_message(), "readable database file")
    );
  }

  mln_test_destroy_runtime(runtime);
}

static void offline_database_merge_reports_a_corrupt_side_database(void) {
  char fixture_path[1024] = {0};
  TEST_ASSERT_TRUE(mln_test_fixture_path(
    "offline_database/corrupt-immediate.db", fixture_path, sizeof(fixture_path)
  ));

  mln_runtime runtime = mln_test_create_runtime();
  mln_test_completion merge = mln_test_completion_default(0);
  // The file opens read-only, so acceptance succeeds and the schema failure
  // reaches the completion instead.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_regions_merge_database(
                     runtime, fixture_path, &merge.descriptor
                   )
  );
  TEST_ASSERT_TRUE(mln_test_completion_wait(&merge, -1));
  TEST_ASSERT_NOT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_status(&merge));
  TEST_ASSERT_GREATER_THAN_size_t(
    0, strlen(mln_test_completion_diagnostic(&merge))
  );
  mln_test_completion_destroy(&merge);
  mln_test_destroy_runtime(runtime);
}

static void resource_transform_rejects_raw_invalid_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    clear_resource_transform_committed(MLN_HANDLE_NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, set_resource_transform_committed(runtime, NULL)
  );
  mln_resource_transform transform = {
    .size = 0, .callback = resource_transform_stub
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    set_resource_transform_committed(runtime, &transform)
  );
  transform.size = sizeof(mln_resource_transform);
  transform.callback = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    set_resource_transform_committed(runtime, &transform)
  );
  mln_test_destroy_runtime(runtime);
}

// A transport that cannot drop a transformed header when a redirect changes
// origin reports MLN_STATUS_UNSUPPORTED rather than leak the credential to the
// redirect's destination.
static void http_header_transform_registration_follows_the_transport(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = http_header_transform_stub,
  };
#if defined(__EMSCRIPTEN__) || defined(__OHOS__)
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    set_http_header_transform_committed(runtime, &transform)
  );
#else
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_http_header_transform_committed(runtime, &transform)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, clear_http_header_transform_committed(runtime)
  );
#endif
  mln_test_destroy_runtime(runtime);
}

static void http_header_transform_rejects_raw_invalid_inputs(void) {
  static const char invalid_utf8[] = {'b', 'a', 'd', (char)0xFF};
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    clear_http_header_transform_committed(MLN_HANDLE_NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    set_http_header_transform_committed(runtime, NULL)
  );
  mln_http_header_transform transform = {
    .size = 0, .callback = http_header_transform_stub
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    set_http_header_transform_committed(runtime, &transform)
  );
  transform.size = sizeof(mln_http_header_transform);
  transform.callback = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    set_http_header_transform_committed(runtime, &transform)
  );

  mln_http_header_transform_response response = {
    .size = sizeof(mln_http_header_transform_response),
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_http_header_transform_response_set(&response, "X-Test", 6, "value", 5)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_http_header_transform_response_set(&response, "Bad Name", 8, "value", 5)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_http_header_transform_response_set(
      &response, "Authorization", 13, "bad\r\nvalue", 10
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_http_header_transform_response_set(
                                   &response, "Range", 5, "bytes=0-1", 9
                                 )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_http_header_transform_response_set(
      &response, "Authorization", 13, invalid_utf8, sizeof(invalid_utf8)
    )
  );
  mln_test_destroy_runtime(runtime);
}

// The transform callback and the second runtime executor run concurrently, so
// every field crosses a thread boundary.
typedef struct teardown_probe {
  atomic_bool transform_entered;
  atomic_bool teardown_started;
  atomic_bool transform_released;
  atomic_bool other_runtime_ready;
  atomic_bool other_runtime_call_done;
  atomic_bool other_runtime_call_observed;
  atomic_int other_runtime_status;
} teardown_probe;

static void mark_transform_released(void* user_data) {
  teardown_probe* probe = user_data;
  atomic_store(&probe->transform_released, true);
}

// Wait budgets are generous on purpose: a slow machine delays the passing run
// instead of turning it red.
enum {
  teardown_probe_wait_attempts = 10000,
  teardown_transform_block_attempts = 3000,
  teardown_call_delay_milliseconds = 200,
  provider_callback_block_milliseconds = 200,
};

// The provider callback runs on a MapLibre file source thread while a runtime
// command clears the provider, so every field crosses a thread boundary.
typedef struct provider_quiescence_probe {
  atomic_bool entered;
  atomic_bool clear_started;
  atomic_bool callback_returned;
} provider_quiescence_probe;

static bool wait_for_flag(atomic_bool* flag) {
  for (size_t attempt = 0; attempt < teardown_probe_wait_attempts;
       attempt += 1) {
    if (atomic_load(flag)) {
      return true;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// Blocks so the per-runtime transform lock stays held while another thread
// starts runtime teardown. Calls no C API function while blocked.
static mln_status blocking_resource_transform(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  (void)kind;
  (void)url;
  teardown_probe* probe = user_data;
  atomic_store(&probe->transform_entered, true);
  for (size_t attempt = 0; attempt < teardown_transform_block_attempts;
       attempt += 1) {
    if (atomic_load(&probe->other_runtime_call_done)) {
      break;
    }
    mln_test_sleep_millisecond();
  }
  atomic_store(
    &probe->other_runtime_call_observed,
    atomic_load(&probe->other_runtime_call_done)
  );
  if (out_response != NULL) {
    out_response->url = NULL;
  }
  return MLN_STATUS_OK;
}

// Owns a second runtime and calls into it while the first runtime is blocked
// inside teardown.
static void other_runtime_entry(void* argument) {
  teardown_probe* probe = argument;
  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_runtime_options options = mln_runtime_options_default();
  const mln_status create_status = mln_test_runtime_create(&options, &runtime);
  if (create_status != MLN_STATUS_OK) {
    atomic_store(&probe->other_runtime_status, create_status);
    atomic_store(&probe->other_runtime_call_done, true);
    return;
  }

  atomic_store(&probe->other_runtime_ready, true);
  wait_for_flag(&probe->teardown_started);
  // Give the teardown thread time to reach the transform wait.
  mln_test_sleep_milliseconds(teardown_call_delay_milliseconds);
  atomic_store(&probe->other_runtime_status, mln_test_runtime_barrier(runtime));
  atomic_store(&probe->other_runtime_call_done, true);
  (void)mln_test_runtime_close(runtime);
}

// Creates a region for `style_url` and activates its download, which requests
// the style from the MapLibre file source thread that runs the provider and
// transform callbacks.
static bool activate_style_download(
  mln_runtime runtime, const char* style_url
) {
  const mln_offline_region_definition definition =
    offline_tile_definition_for_style(style_url);
  return create_and_activate_offline_region(runtime, &definition, NULL);
}

// Waits until `flag` is set by a MapLibre thread.
static bool wait_until_flag(mln_runtime runtime, atomic_bool* flag) {
  for (size_t attempt = 0; attempt < teardown_probe_wait_attempts;
       attempt += 1) {
    if (atomic_load(flag)) {
      return true;
    }
    if (mln_test_runtime_barrier(runtime) != MLN_STATUS_OK) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

static bool start_offline_region_download(
  mln_runtime runtime, teardown_probe* probe
) {
  return activate_style_download(runtime, offline_style_url) &&
         wait_until_flag(runtime, &probe->transform_entered);
}

// Runtime teardown waits for an in-flight resource transform callback without
// holding the process-global runtime registry lock, so calls on an unrelated
// runtime keep running. The callback comes from an offline download, which
// needs no live map.
static void runtime_teardown_leaves_other_runtimes_responsive(void) {
  teardown_probe probe = {0};
  atomic_store(&probe.other_runtime_status, MLN_STATUS_NATIVE_ERROR);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_thread* other_thread =
    mln_test_thread_start(other_runtime_entry, &probe);
  TEST_ASSERT_TRUE(wait_for_flag(&probe.other_runtime_ready));

  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = blocking_resource_transform,
    .user_data = &probe,
    .release_user_data = mark_transform_released,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_transform_committed(runtime, &transform)
  );
  TEST_ASSERT_TRUE(start_offline_region_download(runtime, &probe));

  atomic_store(&probe.teardown_started, true);
  // Teardown blocks here until the transform callback returns.
  mln_test_destroy_runtime(runtime);
  mln_test_thread_join(other_thread);
  TEST_ASSERT_TRUE(wait_for_flag(&probe.transform_released));

  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&probe.other_runtime_call_observed),
    "a call on an unrelated runtime was stalled by runtime teardown"
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, atomic_load(&probe.other_runtime_status)
  );
}

// The provider callback, the transform callback, and the second runtime's owner
// thread are three threads, so every field crosses a thread boundary.
typedef struct lookup_probe {
  atomic_bool transform_entered;
  atomic_bool provider_entered;
  atomic_bool writer_pending;
  atomic_bool lookup_reached;
  atomic_bool other_runtime_ready;
  atomic_bool other_runtime_call_done;
  atomic_bool other_runtime_call_observed;
  atomic_int other_runtime_status;
} lookup_probe;

// Blocks the transform for the first region's style so its shared transform
// lock stays held, making the pending writer below wait. Calls no C API
// function while blocked.
static mln_status lookup_blocking_transform(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  (void)kind;
  lookup_probe* probe = user_data;
  if (out_response != NULL) {
    out_response->url = NULL;
  }
  if (url == NULL || strstr(url, "lookup-blocking") == NULL) {
    return MLN_STATUS_OK;
  }

  atomic_store(&probe->transform_entered, true);
  for (size_t attempt = 0; attempt < teardown_transform_block_attempts;
       attempt += 1) {
    if (atomic_load(&probe->other_runtime_call_done)) {
      break;
    }
    mln_test_sleep_millisecond();
  }
  atomic_store(
    &probe->other_runtime_call_observed,
    atomic_load(&probe->other_runtime_call_done)
  );
  return MLN_STATUS_OK;
}

// Runs on the file source thread immediately before that thread looks up the
// resource transform. Blocking here parks the thread until the runtime worker
// has a transform writer waiting; passing the request through then enters the
// lookup under test.
static uint32_t lookup_probe_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)handle;
  lookup_probe* probe = user_data;
  if (
    request == NULL || request->requested_url == NULL ||
    strstr(request->requested_url, "lookup-probe") == NULL
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  atomic_store(&probe->provider_entered, true);
  for (size_t attempt = 0; attempt < teardown_probe_wait_attempts;
       attempt += 1) {
    if (atomic_load(&probe->writer_pending)) {
      break;
    }
    mln_test_sleep_millisecond();
  }
  // Give the runtime worker time to reach the exclusive transform lock, so the
  // lookup this thread is about to make queues behind a waiting writer.
  mln_test_sleep_milliseconds(teardown_call_delay_milliseconds);
  atomic_store(&probe->lookup_reached, true);
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

// Owns a second runtime and calls into it while a file source thread is inside
// the resource transform lookup for the first runtime.
static void lookup_other_runtime_entry(void* argument) {
  lookup_probe* probe = argument;
  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_runtime_options options = mln_runtime_options_default();
  const mln_status create_status = mln_test_runtime_create(&options, &runtime);
  if (create_status != MLN_STATUS_OK) {
    atomic_store(&probe->other_runtime_status, create_status);
    atomic_store(&probe->other_runtime_call_done, true);
    return;
  }

  atomic_store(&probe->other_runtime_ready, true);
  wait_for_flag(&probe->lookup_reached);
  // Give the file source thread time to reach the lookup itself.
  mln_test_sleep_milliseconds(teardown_call_delay_milliseconds);
  atomic_store(&probe->other_runtime_status, mln_test_runtime_barrier(runtime));
  atomic_store(&probe->other_runtime_call_done, true);
  (void)mln_test_runtime_close(runtime);
}

// A file source resource transform lookup releases the process-global runtime
// registry lock before it waits on the per-runtime transform lock. The setup
// below queues that lookup behind a pending writer, where holding the registry
// lock would stall every unrelated runtime.
static void resource_transform_lookup_leaves_other_runtimes_responsive(void) {
  lookup_probe probe = {0};
  atomic_store(&probe.other_runtime_status, MLN_STATUS_NATIVE_ERROR);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_thread* other_thread =
    mln_test_thread_start(lookup_other_runtime_entry, &probe);
  TEST_ASSERT_TRUE(wait_for_flag(&probe.other_runtime_ready));

  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = lookup_probe_resource_provider,
    .user_data = &probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = lookup_blocking_transform,
    .user_data = &probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_transform_committed(runtime, &transform)
  );

  // The first download parks a transform callback inside the shared lock.
  TEST_ASSERT_TRUE(activate_style_download(runtime, lookup_blocking_style_url));
  TEST_ASSERT_TRUE(wait_until_flag(runtime, &probe.transform_entered));

  // The second download parks a file source thread in the provider callback,
  // one step ahead of the lookup under test.
  TEST_ASSERT_TRUE(activate_style_download(runtime, lookup_probe_style_url));
  TEST_ASSERT_TRUE(wait_until_flag(runtime, &probe.provider_entered));

  atomic_store(&probe.writer_pending, true);
  // Clearing waits for the in-flight transform callback to return, so it is
  // the pending writer the lookup queues behind.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, clear_resource_transform_committed(runtime)
  );
  mln_test_thread_join(other_thread);

  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&probe.lookup_reached),
    "the file source thread never reached the resource transform lookup"
  );
  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&probe.other_runtime_call_observed),
    "a call on an unrelated runtime was stalled by a resource transform lookup"
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, atomic_load(&probe.other_runtime_status)
  );
  mln_test_destroy_runtime(runtime);
}

static void resource_provider_rejects_raw_invalid_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, set_resource_provider_committed(runtime, NULL)
  );
  mln_resource_provider provider = {
    .size = 0, .callback = resource_provider_stub
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    set_resource_provider_committed(runtime, &provider)
  );
  provider.size = sizeof(mln_resource_provider);
  provider.callback = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    set_resource_provider_committed(runtime, &provider)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    clear_resource_provider_committed(MLN_HANDLE_NULL)
  );
  mln_test_destroy_runtime(runtime);
}

// Blocks inside the provider callback so the per-runtime provider lock stays
// held while the runtime executor clears the provider. It calls no C API
// function while blocked.
static uint32_t blocking_resource_provider_for_clear(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  (void)handle;
  provider_quiescence_probe* probe = user_data;
  atomic_store(&probe->entered, true);
  for (size_t attempt = 0; attempt < teardown_probe_wait_attempts;
       attempt += 1) {
    if (atomic_load(&probe->clear_started)) {
      break;
    }
    mln_test_sleep_millisecond();
  }
  // Keep running after clear submission so an implementation that emits the
  // terminal event too early exposes callback-owned state.
  mln_test_sleep_milliseconds(provider_callback_block_milliseconds);
  atomic_store(&probe->callback_returned, true);
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

// Drives an offline region download until the provider callback runs. The
// download requests its style from a MapLibre file source thread and needs no
// live map.
static bool wait_for_clear_provider_callback(
  mln_runtime runtime, provider_quiescence_probe* probe
) {
  const mln_offline_region_definition definition = offline_tile_definition();
  if (!create_and_activate_offline_region(runtime, &definition, NULL)) {
    return false;
  }

  for (size_t attempt = 0; attempt < teardown_probe_wait_attempts;
       attempt += 1) {
    if (atomic_load(&probe->entered)) {
      return true;
    }
    if (mln_test_runtime_barrier(runtime) != MLN_STATUS_OK) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}
typedef struct cross_thread_provider_submission {
  mln_runtime runtime;
  mln_resource_provider provider;
  mln_status status;
  mln_test_completion completion;
} cross_thread_provider_submission;

static void submit_provider_from_thread(void* user_data) {
  cross_thread_provider_submission* submission = user_data;
  submission->status = mln_runtime_set_resource_provider(
    submission->runtime, &submission->provider,
    &submission->completion.descriptor
  );
}

static uint32_t recording_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  (void)handle;
  provider_quiescence_probe* probe = user_data;
  atomic_store(&probe->entered, true);
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

static void resource_provider_command_copies_cross_thread_descriptor(void) {
  provider_quiescence_probe probe = {0};
  mln_runtime runtime = mln_test_create_runtime();
  cross_thread_provider_submission submission = {
    .runtime = runtime,
    .provider =
      {
        .size = sizeof(mln_resource_provider),
        .callback = recording_resource_provider,
        .user_data = &probe,
      },
    .status = MLN_STATUS_NATIVE_ERROR,
    .completion = mln_test_completion_default(0),
  };
  mln_test_thread* thread =
    mln_test_thread_start(submit_provider_from_thread, &submission);
  TEST_ASSERT_NOT_NULL(thread);
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, submission.status);

  // The accepted command owns the descriptor shape, not this binding storage.
  submission.provider.callback = NULL;
  submission.provider.user_data = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&submission.completion)
  );
  mln_test_completion_destroy(&submission.completion);
  TEST_ASSERT_TRUE(wait_for_clear_provider_callback(runtime, &probe));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, clear_resource_provider_committed(runtime)
  );
  mln_test_destroy_runtime(runtime);
}

// The clear command reaches its terminal event only after a provider callback
// that was already running returns.
static void clearing_resource_provider_waits_for_in_flight_callback(void) {
  provider_quiescence_probe probe = {0};
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = blocking_resource_provider_for_clear,
    .user_data = &probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  TEST_ASSERT_TRUE(wait_for_clear_provider_callback(runtime, &probe));

  atomic_store(&probe.clear_started, true);
  // The terminal event waits until the provider callback returns.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, clear_resource_provider_committed(runtime)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&probe.callback_returned),
    "the clear completion ran while a provider callback was still running"
  );

  mln_test_destroy_runtime(runtime);
}

static void unsupported_style_url_scheme_names_scheme_and_url(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(map, unsupported_scheme_style_url)
  );
  char message[512];
  TEST_ASSERT_TRUE(
    wait_for_map_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, unsupported_scheme_style_url));
  TEST_ASSERT_NOT_NULL(strstr(message, "\"jar\""));
  TEST_ASSERT_NOT_NULL(strstr(message, "mln_runtime_set_resource_provider"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void unsupported_style_url_diagnostic_redacts_credentials(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_url(map, credentialed_unsupported_scheme_style_url)
  );
  char message[512];
  TEST_ASSERT_TRUE(
    wait_for_map_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "jar://archive/packaged/style.json"));
  TEST_ASSERT_NULL(strstr(message, "user"));
  TEST_ASSERT_NULL(strstr(message, "password"));
  TEST_ASSERT_NULL(strstr(message, "access_token"));
  TEST_ASSERT_NULL(strstr(message, "secret"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void unsupported_style_url_names_declining_provider(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = resource_provider_stub,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(map, unsupported_scheme_style_url)
  );
  char message[512];
  TEST_ASSERT_TRUE(
    wait_for_map_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "registered resource provider"));
  TEST_ASSERT_NOT_NULL(strstr(message, "declined"));
  TEST_ASSERT_NULL(strstr(message, "register a resource provider"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void resource_provider_defers_inline_release_until_callback_returns(
  void
) {
  inline_release_provider_state state;
  atomic_init(&state.callback_finished, false);
  atomic_init(&state.completion_status, MLN_STATUS_NATIVE_ERROR);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = inline_release_resource_provider,
    .user_data = &state,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(map, "custom://inline-style.json")
  );
  for (size_t attempt = 0;
       attempt < 5000 && !atomic_load(&state.callback_finished); attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
    mln_test_sleep_millisecond();
  }
  TEST_ASSERT_TRUE(atomic_load(&state.callback_finished));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&state.completion_status));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct provider_teardown_probe {
  atomic_bool entered;
  atomic_bool teardown_started;
  atomic_bool callback_returned;
  atomic_bool released;
} provider_teardown_probe;

static void mark_provider_released(void* user_data) {
  provider_teardown_probe* probe = user_data;
  atomic_store(&probe->released, true);
}

enum {
  provider_teardown_wait_attempts = 10000,
  provider_teardown_block_milliseconds = 200,
};

// Blocks after teardown starts so the test can distinguish teardown waiting
// for this invocation from teardown returning while callback state is live.
static uint32_t blocking_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  (void)handle;
  provider_teardown_probe* probe = user_data;
  atomic_store(&probe->entered, true);
  for (size_t attempt = 0; attempt < provider_teardown_wait_attempts;
       attempt += 1) {
    if (atomic_load(&probe->teardown_started)) {
      break;
    }
    mln_test_sleep_millisecond();
  }
  for (size_t elapsed = 0; elapsed < provider_teardown_block_milliseconds;
       elapsed += 1) {
    mln_test_sleep_millisecond();
  }
  atomic_store(&probe->callback_returned, true);
  // An unknown decision becomes a handled provider error, which keeps the
  // request off the native network teardown path.
  return UINT32_MAX;
}

// An offline download's network request runs on a MapLibre file source thread
// and needs no live map during runtime teardown.
static bool wait_for_provider_callback(
  mln_runtime runtime, provider_teardown_probe* probe
) {
  const mln_offline_region_definition definition = offline_tile_definition();
  if (!create_and_activate_offline_region(runtime, &definition, NULL)) {
    return false;
  }

  for (size_t attempt = 0; attempt < provider_teardown_wait_attempts;
       attempt += 1) {
    if (atomic_load(&probe->entered)) {
      return true;
    }
    if (mln_test_runtime_barrier(runtime) != MLN_STATUS_OK) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// Native retains provider user_data until every in-flight callback returns,
// even though public runtime release itself does not wait for teardown.
static void runtime_teardown_waits_for_in_flight_provider_callback(void) {
  provider_teardown_probe probe = {0};
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = blocking_resource_provider,
    .user_data = &probe,
    .release_user_data = mark_provider_released,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  TEST_ASSERT_TRUE(wait_for_provider_callback(runtime, &probe));

  atomic_store(&probe.teardown_started, true);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_TRUE(wait_for_flag(&probe.released));
  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&probe.callback_returned),
    "runtime teardown returned while a provider callback was still running"
  );
}

typedef struct cancel_probe {
  atomic_bool provider_entered;
  atomic_int cancel_count;
  atomic_bool release_inside_callback;
  atomic_bool complete_inline;
  atomic_bool skip_register;
  atomic_int register_status;
  atomic_bool register_reported_cancelled;
  _Atomic mln_resource_request_handle handle;
} cancel_probe;

static void count_cancel(void* user_data) {
  cancel_probe* probe = user_data;
  atomic_fetch_add(&probe->cancel_count, 1);
  if (atomic_load(&probe->release_inside_callback)) {
    mln_resource_request_release(atomic_load(&probe->handle));
  }
}

static uint32_t cancel_probe_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  cancel_probe* probe = user_data;
  atomic_store(&probe->handle, handle);
  if (!atomic_load(&probe->skip_register)) {
    bool cancelled = true;
    atomic_store(
      &probe->register_status, mln_resource_request_set_cancel_callback(
                                 handle, count_cancel, probe, &cancelled
                               )
    );
    atomic_store(&probe->register_reported_cancelled, cancelled);
  }
  if (atomic_load(&probe->complete_inline)) {
    const mln_resource_response response = {
      .size = sizeof(mln_resource_response),
      .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
      .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
      .bytes = inline_style_json,
      .byte_count = sizeof(inline_style_json) - 1,
    };
    (void)mln_resource_request_complete(handle, &response);
  }
  atomic_store(&probe->provider_entered, true);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static mln_map start_cancel_probe_request(
  mln_runtime runtime, cancel_probe* probe
) {
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = cancel_probe_resource_provider,
    .user_data = probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(map, "custom://cancel-style.json")
  );
  TEST_ASSERT_TRUE(wait_for_flag(&probe->provider_entered));
  if (!atomic_load(&probe->skip_register)) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe->register_status));
    TEST_ASSERT_FALSE(atomic_load(&probe->register_reported_cancelled));
  }
  return map;
}

static bool wait_for_cancel_count(
  cancel_probe* probe, int expected, size_t attempts
) {
  for (size_t attempt = 0; attempt < attempts; attempt += 1) {
    if (atomic_load(&probe->cancel_count) >= expected) {
      return true;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// Destroying the map discards its pending style request. MapLibre then cancels
// the handled request, which runs the registered callback once. The request
// keeps that single registration, and rejects a late completion.
static void cancel_callback_runs_when_map_discards_request(void) {
  cancel_probe probe = {0};
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));

  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(
    wait_for_cancel_count(&probe, 1, teardown_probe_wait_attempts)
  );
  const mln_resource_request_handle handle = atomic_load(&probe.handle);

  bool cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_resource_request_cancelled(handle, &cancelled)
  );
  TEST_ASSERT_TRUE(cancelled);
  const mln_resource_response response = style_response();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_resource_request_complete(handle, &response)
  );

  cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_resource_request_set_cancel_callback(
                                handle, count_cancel, &probe, &cancelled
                              )
  );
  TEST_ASSERT_FALSE(cancelled);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));

  mln_resource_request_release(handle);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_resource_request_set_cancel_callback(
                                   handle, count_cancel, &probe, &cancelled
                                 )
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));
  mln_test_destroy_runtime(runtime);
}

// A registration that arrives after cancellation stores nothing and reports
// the cancellation through out_cancelled instead of invoking the callback.
static void late_cancel_callback_registration_reports_cancelled(void) {
  cancel_probe probe = {0};
  atomic_store(&probe.skip_register, true);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);
  mln_test_destroy_map(map);
  const mln_resource_request_handle handle = atomic_load(&probe.handle);

  bool cancelled = false;
  for (size_t attempt = 0; attempt < teardown_probe_wait_attempts;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_resource_request_cancelled(handle, &cancelled)
    );
    if (cancelled) {
      break;
    }
    mln_test_sleep_millisecond();
  }
  TEST_ASSERT_TRUE(cancelled);

  cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_resource_request_set_cancel_callback(
                     handle, count_cancel, &probe, &cancelled
                   )
  );
  TEST_ASSERT_TRUE(cancelled);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));

  mln_resource_request_release(handle);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));
  mln_test_destroy_runtime(runtime);
}

// The callback runs unlocked, so releasing the cancelled handle from inside it
// retires the request without deadlocking.
static void cancel_callback_may_release_the_request(void) {
  cancel_probe probe = {0};
  atomic_store(&probe.release_inside_callback, true);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);

  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(
    wait_for_cancel_count(&probe, 1, teardown_probe_wait_attempts)
  );
  const mln_resource_request_handle handle = atomic_load(&probe.handle);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_resource_request_wait_until_retired(handle)
  );
  bool cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_resource_request_set_cancel_callback(
                                   handle, count_cancel, &probe, &cancelled
                                 )
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));
  mln_test_destroy_runtime(runtime);
}

// MapLibre runs its cancel hook on every request teardown, including after the
// response was delivered. The C API reports cancellation only for a request the
// provider has not completed.
static void cancel_callback_skips_a_completed_request(void) {
  cancel_probe probe = {0};
  atomic_store(&probe.complete_inline, true);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);
  const mln_resource_request_handle handle = atomic_load(&probe.handle);

  // Let the response reach the style before the map goes away.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_test_destroy_map(map);
  TEST_ASSERT_FALSE(wait_for_cancel_count(&probe, 1, 200));
  bool cancelled = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_resource_request_cancelled(handle, &cancelled)
  );
  TEST_ASSERT_FALSE(cancelled);

  mln_resource_request_release(handle);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));
  mln_test_destroy_runtime(runtime);
}

typedef struct blocking_cancel_probe {
  cancel_probe base;
  atomic_bool self_release;
  atomic_bool waiter_drains_instead_of_releasing;
  atomic_int status_after_self_release;
  atomic_int complete_status_after_self_release;
  atomic_bool callback_entered;
  atomic_bool release_started;
  atomic_bool release_returned;
  atomic_bool release_returned_during_callback;
  atomic_bool callback_returned;
  atomic_bool callback_returned_before_release;
} blocking_cancel_probe;

// Runs on the runtime worker thread while the map release discards the request.
// Waits for the releasing thread to enter its release call, gives it time to
// return, and records whether it did.
static void block_in_cancel(void* user_data) {
  blocking_cancel_probe* probe = user_data;
  if (atomic_load(&probe->self_release)) {
    const mln_resource_request_handle handle = atomic_load(&probe->base.handle);
    mln_resource_request_release(handle);
    // The entry outlives this callback, but the handle is released: every
    // other entry point reports it as such.
    bool cancelled = false;
    atomic_store(
      &probe->status_after_self_release,
      mln_resource_request_cancelled(handle, &cancelled)
    );
    const mln_resource_response response = style_response();
    atomic_store(
      &probe->complete_status_after_self_release,
      mln_resource_request_complete(handle, &response)
    );
  }
  atomic_store(&probe->callback_entered, true);
  wait_for_flag(&probe->release_started);
  mln_test_sleep_milliseconds(provider_teardown_block_milliseconds);
  atomic_store(
    &probe->release_returned_during_callback,
    atomic_load(&probe->release_returned)
  );
  atomic_store(&probe->callback_returned, true);
}

static uint32_t blocking_cancel_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  blocking_cancel_probe* probe = user_data;
  atomic_store(&probe->base.handle, handle);
  bool cancelled = true;
  atomic_store(
    &probe->base.register_status, mln_resource_request_set_cancel_callback(
                                    handle, block_in_cancel, probe, &cancelled
                                  )
  );
  atomic_store(&probe->base.register_reported_cancelled, cancelled);
  atomic_store(&probe->base.provider_entered, true);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static void run_release_waits_for_in_flight_cancel_callback(
  blocking_cancel_probe* probe
) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = blocking_cancel_resource_provider,
    .user_data = probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_url(map, "custom://blocking-cancel.json")
  );
  TEST_ASSERT_TRUE(wait_for_flag(&probe->base.provider_entered));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, atomic_load(&probe->base.register_status)
  );
  TEST_ASSERT_FALSE(atomic_load(&probe->base.register_reported_cancelled));

  // The map release submits and returns, so the cancel callback runs on the
  // runtime worker while this thread does the release it must wait for.
  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(wait_for_flag(&probe->callback_entered));
  atomic_store(&probe->release_started, true);
  if (atomic_load(&probe->waiter_drains_instead_of_releasing)) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_resource_request_wait_until_retired(atomic_load(&probe->base.handle))
    );
  } else {
    mln_resource_request_release(atomic_load(&probe->base.handle));
  }
  atomic_store(
    &probe->callback_returned_before_release,
    atomic_load(&probe->callback_returned)
  );
  atomic_store(&probe->release_returned, true);
  TEST_ASSERT_FALSE_MESSAGE(
    atomic_load(&probe->release_returned_during_callback),
    "releasing the request returned while the cancel callback was running"
  );
  TEST_ASSERT_TRUE(atomic_load(&probe->callback_returned_before_release));
  if (atomic_load(&probe->self_release)) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_INVALID_ARGUMENT,
      atomic_load(&probe->status_after_self_release)
    );
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_INVALID_ARGUMENT,
      atomic_load(&probe->complete_status_after_self_release)
    );
  } else {
    mln_resource_request_release(atomic_load(&probe->base.handle));
  }
  bool cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_cancelled(atomic_load(&probe->base.handle), &cancelled)
  );
  mln_test_destroy_runtime(runtime);
}

// user_data may be freed once release returns, so release waits for a callback
// still running on another thread. Destroying the map cancels its style request
// on the runtime worker, so the release runs on the test thread.
static void release_waits_for_in_flight_cancel_callback(void) {
  blocking_cancel_probe probe = {0};
  run_release_waits_for_in_flight_cancel_callback(&probe);
}

// Release is idempotent, so a release from the test thread must still find the
// request and wait after the callback released it from inside. Inside that
// window the released handle already rejects every other entry point.
static void release_waits_for_a_cancel_callback_that_released_itself(void) {
  blocking_cancel_probe probe = {0};
  atomic_store(&probe.self_release, true);
  run_release_waits_for_in_flight_cancel_callback(&probe);
}

// Teardown drains requests through the wait-until-retired call, so a request
// whose callback released it from inside is drained only once the callback
// returns.
static void wait_until_retired_waits_for_a_self_releasing_cancel_callback(
  void
) {
  blocking_cancel_probe probe = {0};
  atomic_store(&probe.self_release, true);
  atomic_store(&probe.waiter_drains_instead_of_releasing, true);
  run_release_waits_for_in_flight_cancel_callback(&probe);
}

void run_resources_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(custom_provider_request_handles_reject_raw_null_handles);
  RUN_TEST(network_status_get_rejects_raw_null_output);
  RUN_TEST(ambient_cache_operations_validate_raw_operation_values);
  RUN_TEST(set_maximum_ambient_cache_size_rejects_raw_null_output);
  RUN_TEST(offline_regions_reject_raw_invalid_descriptors);
  RUN_TEST(offline_database_merge_rejects_raw_null_path);
  RUN_TEST(offline_database_merge_rejects_a_missing_file);
  RUN_TEST(offline_database_merge_rejects_sqlite_pseudo_paths);
  RUN_TEST(offline_database_merge_reports_a_corrupt_side_database);
  RUN_TEST(resource_transform_rejects_raw_invalid_descriptors);
  RUN_TEST(http_header_transform_registration_follows_the_transport);
  RUN_TEST(http_header_transform_rejects_raw_invalid_inputs);
  RUN_TEST(runtime_teardown_leaves_other_runtimes_responsive);
  RUN_TEST(resource_transform_lookup_leaves_other_runtimes_responsive);
  RUN_TEST(resource_provider_rejects_raw_invalid_descriptors);
  RUN_TEST(resource_provider_registration_releases_owned_state);
  RUN_TEST(resource_provider_command_copies_cross_thread_descriptor);
  RUN_TEST(clearing_resource_provider_waits_for_in_flight_callback);
  RUN_TEST(unsupported_style_url_scheme_names_scheme_and_url);
  RUN_TEST(unsupported_style_url_diagnostic_redacts_credentials);
  RUN_TEST(unsupported_style_url_names_declining_provider);
  RUN_TEST(resource_provider_defers_inline_release_until_callback_returns);
  RUN_TEST(runtime_teardown_waits_for_in_flight_provider_callback);
  RUN_TEST(cancel_callback_runs_when_map_discards_request);
  RUN_TEST(late_cancel_callback_registration_reports_cancelled);
  RUN_TEST(cancel_callback_may_release_the_request);
  RUN_TEST(cancel_callback_skips_a_completed_request);
  RUN_TEST(release_waits_for_in_flight_cancel_callback);
  RUN_TEST(release_waits_for_a_cancel_callback_that_released_itself);
  RUN_TEST(wait_until_retired_waits_for_a_self_releasing_cancel_callback);
}
