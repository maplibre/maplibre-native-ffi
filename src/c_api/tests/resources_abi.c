// Raw C ABI coverage: null handles/outputs, unknown operation values, null
// paths, callback descriptor shape, and invalid offline unions are hidden by
// bindings.

#include <stdatomic.h>
#include <stdint.h>

#include "test_support.h"
#include "unity.h"

static const char offline_style_url[] = "http://example.com/offline-style.json";

static mln_runtime_event empty_event(void) {
  return (mln_runtime_event){
    .size = sizeof(mln_runtime_event),
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_NONE,
  };
}

static bool wait_for_offline_completion(
  mln_runtime* runtime, mln_offline_operation_id operation_id
) {
  for (size_t attempt = 0; attempt < 5000; attempt += 1) {
    if (mln_runtime_run_once(runtime) != MLN_STATUS_OK) {
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
      if (
        event.type != MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED ||
        event.payload_type !=
          MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
      ) {
        continue;
      }
      if (event.payload == NULL) {
        return false;
      }
      const mln_runtime_event_offline_operation_completed* payload =
        event.payload;
      if (payload->operation_id == operation_id) {
        return true;
      }
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

static mln_offline_region_definition offline_tile_definition(void) {
  return (mln_offline_region_definition){
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data = {
      .tile_pyramid = {
        .size = sizeof(mln_offline_tile_pyramid_region_definition),
        .style_url = offline_style_url,
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

static mln_offline_region_definition offline_geometry_definition(
  const mln_geometry* geometry
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

static mln_resource_response style_response(void) {
  return (mln_resource_response){
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
  };
}

static uint32_t resource_provider_stub(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle* handle
) {
  (void)user_data;
  (void)request;
  (void)handle;
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
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

// This verifies null request-handle behavior for release, cancellation, and
// completion below binding wrappers.
static void custom_provider_request_handles_reject_raw_null_handles(void) {
  mln_resource_request_release(NULL);
  bool cancelled = true;
  const mln_resource_response response = style_response();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_cancelled(NULL, &cancelled)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_resource_request_complete(NULL, &response)
  );
}

// This verifies the process-global getter rejects a null C output pointer that
// binding APIs hide.
static void network_status_get_rejects_raw_null_output(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_network_status_get(NULL)
  );
}

// This verifies unknown raw operation discriminants and failure-time output
// initialization.
static void ambient_cache_operations_validate_raw_operation_values(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_offline_operation_id operation_id = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_run_ambient_cache_operation_start(
      runtime, (mln_ambient_cache_operation)999, &operation_id
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, operation_id);
  mln_test_destroy_runtime(runtime);
}

// This verifies raw union discriminants, required nested pointers, and
// failure-time output initialization.
static void offline_regions_reject_raw_invalid_descriptors(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_offline_region_definition definition = offline_tile_definition();
  const uint8_t metadata[] = {1, 2, 3};
  mln_offline_operation_id operation_id = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create_start(
      runtime, NULL, metadata, sizeof(metadata), &operation_id
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, operation_id);

  definition.type = (mln_offline_region_definition_type)999;
  operation_id = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create_start(
      runtime, &definition, metadata, sizeof(metadata), &operation_id
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, operation_id);

  definition = offline_tile_definition();
  definition.data.tile_pyramid.style_url = NULL;
  operation_id = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create_start(
      runtime, &definition, metadata, sizeof(metadata), &operation_id
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, operation_id);

  const mln_lat_lng coordinates[] = {
    {.latitude = 1.0, .longitude = 2.0},
    {.latitude = 3.0, .longitude = 4.0},
  };
  const mln_geometry geometry = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_LINE_STRING,
    .data = {
      .line_string = {
        .coordinates = coordinates,
        .coordinate_count = sizeof(coordinates) / sizeof(coordinates[0]),
      }
    },
  };
  definition = offline_geometry_definition(&geometry);
  definition.data.geometry.style_url = NULL;
  operation_id = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create_start(
      runtime, &definition, metadata, sizeof(metadata), &operation_id
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, operation_id);

  definition = offline_geometry_definition(&geometry);
  definition.data.geometry.geometry = NULL;
  operation_id = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create_start(
      runtime, &definition, metadata, sizeof(metadata), &operation_id
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, operation_id);
  mln_test_destroy_runtime(runtime);
}

// This verifies a null borrowed database path is rejected before any
// asynchronous operation is created.
static void offline_database_merge_rejects_raw_null_path(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_offline_operation_id operation_id = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_regions_merge_database_start(
      runtime, NULL, &operation_id
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, operation_id);
  mln_test_destroy_runtime(runtime);
}

// This verifies wrong-result-kind rejection because typed binding operation
// variants prevent requesting a mismatched result.
static void offline_take_rejects_mismatched_result_kind(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  const uint8_t metadata[] = {1, 2, 3};
  const mln_offline_region_definition definition = offline_tile_definition();
  mln_offline_operation_id operation_id = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_offline_region_create_start(
      runtime, &definition, metadata, sizeof(metadata), &operation_id
    )
  );
  TEST_ASSERT_NOT_EQUAL(0, operation_id);
  TEST_ASSERT_TRUE(wait_for_offline_completion(runtime, operation_id));
  mln_offline_region_list* wrong_list = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_runtime_offline_regions_list_take_result(
                                runtime, operation_id, &wrong_list
                              )
  );
  TEST_ASSERT_NULL(wrong_list);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_operation_discard(runtime, operation_id)
  );
  mln_test_destroy_runtime(runtime);
}

// This verifies null, undersized, and missing-callback descriptors that binding
// constructors cannot produce.
static void resource_transform_rejects_raw_invalid_descriptors(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_clear_resource_transform(NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_resource_transform(runtime, NULL)
  );
  mln_resource_transform transform = {
    .size = 0, .callback = resource_transform_stub
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_resource_transform(runtime, &transform)
  );
  transform.size = sizeof(mln_resource_transform);
  transform.callback = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_resource_transform(runtime, &transform)
  );
  mln_test_destroy_runtime(runtime);
}

// Shared state for the teardown lock-scope test below. The transform callback
// runs on a MapLibre file source thread and the second runtime lives on its own
// owner thread, so every field crosses a thread boundary.
typedef struct teardown_probe {
  atomic_bool transform_entered;
  atomic_bool teardown_started;
  atomic_bool other_runtime_ready;
  atomic_bool other_runtime_call_done;
  atomic_bool other_runtime_call_observed;
  atomic_int other_runtime_status;
} teardown_probe;

// Wait budgets are generous on purpose: a slow machine delays the passing run
// instead of turning it red.
enum {
  teardown_probe_wait_attempts = 10000,
  teardown_transform_block_attempts = 3000,
  teardown_call_delay_milliseconds = 200,
};

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

// The C API asks transform callbacks to return quickly. This one blocks on
// purpose so the per-runtime transform lock stays held while the owner thread
// tears the runtime down, and it calls no C API function while blocked.
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
  mln_runtime* runtime = NULL;
  const mln_runtime_options options = mln_runtime_options_default();
  const mln_status create_status = mln_runtime_create(&options, &runtime);
  if (create_status != MLN_STATUS_OK) {
    atomic_store(&probe->other_runtime_status, create_status);
    atomic_store(&probe->other_runtime_call_done, true);
    return;
  }

  atomic_store(&probe->other_runtime_ready, true);
  wait_for_flag(&probe->teardown_started);
  // Give the owner thread time to reach the transform wait inside teardown.
  mln_test_sleep_milliseconds(teardown_call_delay_milliseconds);
  atomic_store(&probe->other_runtime_status, mln_runtime_run_once(runtime));
  atomic_store(&probe->other_runtime_call_done, true);
  mln_runtime_destroy(runtime);
}

static bool start_offline_region_download(
  mln_runtime* runtime, teardown_probe* probe
) {
  const mln_offline_region_definition definition = offline_tile_definition();
  const uint8_t metadata[] = {1, 2, 3};
  mln_offline_operation_id operation_id = 0;
  if (
    mln_runtime_offline_region_create_start(
      runtime, &definition, metadata, sizeof(metadata), &operation_id
    ) != MLN_STATUS_OK ||
    !wait_for_offline_completion(runtime, operation_id)
  ) {
    return false;
  }

  mln_offline_region_snapshot* snapshot = NULL;
  if (
    mln_runtime_offline_region_create_take_result(
      runtime, operation_id, &snapshot
    ) != MLN_STATUS_OK
  ) {
    return false;
  }
  mln_offline_region_info info = {.size = sizeof(mln_offline_region_info)};
  const mln_status info_status =
    mln_offline_region_snapshot_get(snapshot, &info);
  const mln_offline_region_id region_id = info.id;
  mln_offline_region_snapshot_destroy(snapshot);
  if (info_status != MLN_STATUS_OK) {
    return false;
  }

  mln_offline_operation_id download_operation_id = 0;
  if (
    mln_runtime_offline_region_set_download_state_start(
      runtime, region_id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
      &download_operation_id
    ) != MLN_STATUS_OK
  ) {
    return false;
  }

  // The download requests the region style URL from a MapLibre file source
  // thread, which is where the transform callback runs.
  for (size_t attempt = 0; attempt < teardown_probe_wait_attempts;
       attempt += 1) {
    if (atomic_load(&probe->transform_entered)) {
      return true;
    }
    if (mln_runtime_run_once(runtime) != MLN_STATUS_OK) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// This verifies that runtime teardown waits for an in-flight resource transform
// callback without holding the process-global runtime registry lock, so calls
// on an unrelated runtime keep running while teardown is blocked. An offline
// download supplies the callback because it needs no live map, which teardown
// forbids.
static void runtime_teardown_leaves_other_runtimes_responsive(void) {
  teardown_probe probe = {0};
  atomic_store(&probe.other_runtime_status, MLN_STATUS_NATIVE_ERROR);
  mln_runtime* runtime = mln_test_create_runtime();
  mln_test_thread* other_thread =
    mln_test_thread_start(other_runtime_entry, &probe);
  TEST_ASSERT_TRUE(wait_for_flag(&probe.other_runtime_ready));

  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = blocking_resource_transform,
    .user_data = &probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_set_resource_transform(runtime, &transform)
  );
  TEST_ASSERT_TRUE(start_offline_region_download(runtime, &probe));

  atomic_store(&probe.teardown_started, true);
  // Teardown blocks here until the transform callback returns.
  mln_test_destroy_runtime(runtime);
  mln_test_thread_join(other_thread);

  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&probe.other_runtime_call_observed),
    "a call on an unrelated runtime was stalled by runtime teardown"
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, atomic_load(&probe.other_runtime_status)
  );
}

// This verifies null, undersized, and missing-callback provider descriptors
// below binding-owned validation.
static void resource_provider_rejects_raw_invalid_descriptors(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_resource_provider(runtime, NULL)
  );
  mln_resource_provider provider = {
    .size = 0, .callback = resource_provider_stub
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_resource_provider(runtime, &provider)
  );
  provider.size = sizeof(mln_resource_provider);
  provider.callback = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_resource_provider(runtime, &provider)
  );
  mln_test_destroy_runtime(runtime);
}

void run_resources_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(custom_provider_request_handles_reject_raw_null_handles);
  RUN_TEST(network_status_get_rejects_raw_null_output);
  RUN_TEST(ambient_cache_operations_validate_raw_operation_values);
  RUN_TEST(offline_regions_reject_raw_invalid_descriptors);
  RUN_TEST(offline_database_merge_rejects_raw_null_path);
  RUN_TEST(offline_take_rejects_mismatched_result_kind);
  RUN_TEST(resource_transform_rejects_raw_invalid_descriptors);
  RUN_TEST(runtime_teardown_leaves_other_runtimes_responsive);
  RUN_TEST(resource_provider_rejects_raw_invalid_descriptors);
}
