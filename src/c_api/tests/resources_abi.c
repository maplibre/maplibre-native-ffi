// Raw C ABI coverage: null handles/outputs, unknown operation values, null
// paths, callback descriptor shape, and invalid offline unions are hidden by
// bindings.

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
  RUN_TEST(resource_provider_rejects_raw_invalid_descriptors);
}
