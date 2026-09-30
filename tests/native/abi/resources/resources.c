// Raw C ABI coverage: null handles/outputs, unknown operation values, null
// paths, callback descriptor shape, and invalid offline unions are hidden by
// bindings, plus the network file source diagnostic that every binding
// forwards verbatim.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static const char offline_style_url[] = "http://example.com/offline-style.json";
static const char unsupported_scheme_style_url[] =
  "jar:file:/packaged/style.json";
static const char credentialed_unsupported_scheme_style_url[] =
  "jar://user:password@archive/packaged/style.json?access_token=secret#token";
static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// Settles a runtime configuration command: a rejected submission leaves the
// completion with the caller, and an accepted one reports its terminal status.
static mln_status commit(mln_status status, mln_test_completion* completion) {
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(completion) : status;
  mln_test_completion_destroy(completion);
  return result;
}

static mln_status set_resource_provider_committed(
  mln_runtime runtime, const mln_resource_provider* provider
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_resource_provider(
      runtime, provider, &completion.descriptor, NULL
    ),
    &completion
  );
}

static mln_status clear_resource_provider_committed(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_resource_provider(runtime, &completion.descriptor, NULL),
    &completion
  );
}

static mln_status set_resource_transform_committed(
  mln_runtime runtime, const mln_resource_transform* transform
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_resource_transform(
      runtime, transform, &completion.descriptor, NULL
    ),
    &completion
  );
}

static mln_status clear_resource_transform_committed(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_resource_transform(runtime, &completion.descriptor, NULL),
    &completion
  );
}

static mln_status set_http_header_transform_committed(
  mln_runtime runtime, const mln_http_header_transform* transform
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_http_header_transform(
      runtime, transform, &completion.descriptor, NULL
    ),
    &completion
  );
}

static mln_status clear_http_header_transform_committed(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_http_header_transform(
      runtime, &completion.descriptor, NULL
    ),
    &completion
  );
}

static bool wait_for_map_loading_failure(
  mln_runtime runtime, const mln_map map, char* out_message,
  size_t out_message_capacity
) {
  mln_runtime_event event = {0};
  return mln_test_await_event(
           runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, &event,
           out_message, out_message_capacity
         ) &&
         event.message_size < out_message_capacity;
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
    runtime, definition, metadata, sizeof(metadata), &creation.descriptor, NULL
  );
  if (submission != MLN_STATUS_OK) {
    mln_test_completion_reject(&creation);
    mln_test_completion_destroy(&creation);
    return false;
  }
  if (
    !mln_test_completion_wait(&creation, 5000) ||
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
      runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &download, NULL
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
  mln_test_pulse();
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
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_set_resource_provider(
                                   MLN_HANDLE_NULL, &provider, &rejected, NULL
                                 )
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
  (void)mln_test_wait_for_count(&release_count, 3);
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
    &state->completion_status,
    mln_resource_request_complete(handle, &response, NULL)
  );
  mln_resource_request_release(handle);
  mln_test_flag_set(&state->callback_finished);
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
    mln_resource_request_cancelled(MLN_HANDLE_NULL, &cancelled, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_complete(MLN_HANDLE_NULL, &response, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_set_cancel_callback(
      MLN_HANDLE_NULL, ignore_cancel, NULL, NULL, &cancelled, NULL
    )
  );
}

static void network_status_get_rejects_raw_null_output(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_network_status_get(NULL, NULL)
  );
}

static void ambient_cache_operations_validate_raw_operation_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_run_ambient_cache_operation(
      runtime, (mln_ambient_cache_operation)999, &completion, NULL
    )
  );
  mln_test_destroy_runtime(runtime);
}

static void set_maximum_ambient_cache_size_rejects_raw_null_output(void) {
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_set_maximum_ambient_cache_size(runtime, 1024, NULL, NULL)
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
      runtime, NULL, metadata, sizeof(metadata), &completion, NULL
    )
  );

  definition.type = (mln_offline_region_definition_type)999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &completion, NULL
    )
  );

  definition = offline_tile_definition();
  definition.data.tile_pyramid.style_url = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &completion, NULL
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
      runtime, &definition, metadata, sizeof(metadata), &completion, NULL
    )
  );

  definition = offline_geometry_definition(geometry);
  definition.data.geometry.geometry = (mln_buffer_view){0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &definition, metadata, sizeof(metadata), &completion, NULL
    )
  );
  mln_test_destroy_runtime(runtime);
}

static void offline_database_merge_rejects_raw_null_path(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_regions_merge_database(runtime, NULL, &completion, NULL)
  );
  mln_test_destroy_runtime(runtime);
}

static void offline_database_merge_rejects_a_missing_file(void) {
  static const char missing_path[] = "mln-ffi-missing-offline-side-database.db";
  (void)remove(missing_path);

  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_regions_merge_database(
      runtime, missing_path, &completion, MLN_TEST_DIAGNOSTIC
    )
  );
  // The rejection happens on the calling thread, so its diagnostic is readable
  // here rather than through the completion.
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "readable database file"));
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
      MLN_STATUS_INVALID_ARGUMENT,
      mln_runtime_offline_regions_merge_database(
        runtime, pseudo_paths[index], &completion, MLN_TEST_DIAGNOSTIC
      )
    );
    TEST_ASSERT_NOT_NULL(
      strstr(mln_test_last_error(), "readable database file")
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
                     runtime, fixture_path, &merge.descriptor, NULL
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

typedef struct offline_reentry_probe {
  mln_runtime runtime;
  mln_test_completion* list;
  atomic_bool entered;
  atomic_bool offline_accepted;
  atomic_bool finished;
  atomic_int offline_status;
  atomic_int reentrant_status;
} offline_reentry_probe;

// Submits an offline operation from a second host thread while the runtime
// worker is occupied by the probe's completion.
static void submit_offline_list(void* argument) {
  offline_reentry_probe* probe = argument;
  (void)mln_test_wait_for_flag(&probe->entered);
  atomic_store(
    &probe->offline_status, mln_runtime_offline_regions_list(
                              probe->runtime, &probe->list->descriptor, NULL
                            )
  );
  mln_test_flag_set(&probe->offline_accepted);
}

// Occupies whichever thread delivers this completion until the offline
// submission has been accepted, then submits runtime work from it.
static void occupy_until_offline_accepted(
  void* user_data, const mln_completion_result* result
) {
  offline_reentry_probe* probe = user_data;
  (void)result;
  mln_test_flag_set(&probe->entered);
  (void)mln_test_wait_for_flag(&probe->offline_accepted);
  const mln_completion discard = mln_test_discard_completion();
  atomic_store(
    &probe->reentrant_status,
    mln_runtime_barrier(probe->runtime, &discard, NULL)
  );
  mln_test_flag_set(&probe->finished);
}

static void offline_submission_never_waits_for_the_runtime_worker(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_completion list = mln_test_completion_default(0);
  offline_reentry_probe probe = {.runtime = runtime, .list = &list};
  atomic_init(&probe.entered, false);
  atomic_init(&probe.offline_accepted, false);
  atomic_init(&probe.finished, false);
  atomic_init(&probe.offline_status, MLN_STATUS_INVALID_STATE);
  atomic_init(&probe.reentrant_status, MLN_STATUS_INVALID_STATE);
  mln_test_thread* thread = mln_test_thread_start(submit_offline_list, &probe);

  // A resource configuration completion runs on the runtime worker, so this
  // holds that worker until the other thread's offline call has been accepted.
  const mln_completion occupied = {
    .size = sizeof(mln_completion),
    .callback = occupy_until_offline_accepted,
    .user_data = &probe,
    .release_user_data = NULL,
  };
  const mln_status accepted =
    mln_runtime_clear_resource_provider(runtime, &occupied, NULL);
  if (accepted != MLN_STATUS_OK) {
    // The completion will never run, so release the waiting thread by hand.
    mln_test_flag_set(&probe.entered);
  }
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, accepted);
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe.offline_status));
  (void)mln_test_wait_for_flag(&probe.finished);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe.reentrant_status));

  TEST_ASSERT_TRUE(mln_test_completion_wait(&list, 5000));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_status(&list));
  mln_test_completion_destroy(&list);
  mln_test_destroy_runtime(runtime);
}

static mln_status offline_completion_status(
  mln_status submission, mln_test_completion* completion
) {
  if (submission != MLN_STATUS_OK) {
    mln_test_completion_reject(completion);
    return submission;
  }
  if (!mln_test_completion_wait(completion, 5000)) {
    return MLN_STATUS_INVALID_STATE;
  }
  return mln_test_completion_status(completion);
}

static void offline_operations_report_a_missing_region(void) {
  static const mln_offline_region_id missing = 987654321;
  mln_runtime runtime = mln_test_create_runtime();
  const uint8_t metadata[] = {4, 5, 6};

  // A get reports absence as an empty success, unlike every other operation.
  mln_test_completion get =
    mln_test_completion_default(sizeof(mln_offline_region_info));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    offline_completion_status(
      mln_runtime_offline_region_get(runtime, missing, &get.descriptor, NULL),
      &get
    )
  );
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&get));
  mln_test_completion_destroy(&get);

  mln_test_completion update = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND,
    offline_completion_status(
      mln_runtime_offline_region_update_metadata(
        runtime, missing, metadata, sizeof(metadata), &update.descriptor, NULL
      ),
      &update
    )
  );
  mln_test_completion_destroy(&update);

  mln_test_completion region_status = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, offline_completion_status(
                            mln_runtime_offline_region_get_status(
                              runtime, missing, &region_status.descriptor, NULL
                            ),
                            &region_status
                          )
  );
  mln_test_completion_destroy(&region_status);

  mln_test_completion observed = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, offline_completion_status(
                            mln_runtime_offline_region_set_observed(
                              runtime, missing, true, &observed.descriptor, NULL
                            ),
                            &observed
                          )
  );
  mln_test_completion_destroy(&observed);

  mln_test_completion download = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND,
    offline_completion_status(
      mln_runtime_offline_region_set_download_state(
        runtime, missing, MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
        &download.descriptor, NULL
      ),
      &download
    )
  );
  mln_test_completion_destroy(&download);

  mln_test_completion invalidate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, offline_completion_status(
                            mln_runtime_offline_region_invalidate(
                              runtime, missing, &invalidate.descriptor, NULL
                            ),
                            &invalidate
                          )
  );
  mln_test_completion_destroy(&invalidate);

  mln_test_completion removal = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, offline_completion_status(
                            mln_runtime_offline_region_delete(
                              runtime, missing, &removal.descriptor, NULL
                            ),
                            &removal
                          )
  );
  mln_test_completion_destroy(&removal);

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
    MLN_STATUS_INVALID_STATE, mln_http_header_transform_response_set(
                                &response, "X-Test", 6, "value", 5, NULL
                              )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_http_header_transform_response_set(
                                   &response, "Bad Name", 8, "value", 5, NULL
                                 )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_http_header_transform_response_set(
      &response, "Authorization", 13, "bad\r\nvalue", 10, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_http_header_transform_response_set(
                                   &response, "Range", 5, "bytes=0-1", 9, NULL
                                 )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_http_header_transform_response_set(
      &response, "Authorization", 13, invalid_utf8, sizeof(invalid_utf8), NULL
    )
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

// The provider callback runs on a MapLibre file source thread.
typedef struct provider_request_probe {
  atomic_bool entered;
} provider_request_probe;

// Drives an offline region download until the provider callback runs. The
// download requests its style from a MapLibre file source thread and needs no
// live map.
static bool wait_for_provider_request(
  mln_runtime runtime, provider_request_probe* probe
) {
  const mln_offline_region_definition definition = offline_tile_definition();
  if (!create_and_activate_offline_region(runtime, &definition, NULL)) {
    return false;
  }
  return mln_test_wait_until(runtime, &probe->entered);
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
    &submission->completion.descriptor, NULL
  );
}

static uint32_t recording_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  (void)handle;
  provider_request_probe* probe = user_data;
  mln_test_flag_set(&probe->entered);
  // An unknown decision becomes a handled provider error, which keeps the
  // request off the network.
  return UINT32_MAX;
}

static void resource_provider_command_copies_cross_thread_descriptor(void) {
  provider_request_probe probe = {0};
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
  TEST_ASSERT_TRUE(wait_for_provider_request(runtime, &probe));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, clear_resource_provider_committed(runtime)
  );
  mln_test_destroy_runtime(runtime);
}

typedef struct dropped_request_probe {
  atomic_bool release_inline;
  _Atomic mln_resource_request_handle handle;
} dropped_request_probe;

static uint32_t claim_and_drop_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  dropped_request_probe* probe = user_data;
  atomic_store(&probe->handle, handle);
  mln_test_pulse();
  if (atomic_load(&probe->release_inline)) {
    mln_resource_request_release(handle);
  }
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static bool dropped_request_claimed(void* context) {
  dropped_request_probe* probe = context;
  return atomic_load(&probe->handle) != MLN_HANDLE_NULL;
}

static void expect_dropped_request_fails(bool release_inline) {
  dropped_request_probe probe = {0};
  atomic_store(&probe.release_inline, release_inline);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = claim_and_drop_resource_provider,
    .user_data = &probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_resource_provider_committed(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_url(map, "custom://dropped-request-style.json")
  );
  if (!release_inline) {
    (void)mln_test_await(
      dropped_request_claimed, &probe, mln_test_deadline_default(),
      "the provider to claim the request"
    );
    TEST_ASSERT_NOT_EQUAL(MLN_HANDLE_NULL, atomic_load(&probe.handle));
    mln_resource_request_release(atomic_load(&probe.handle));
  }
  char message[512];
  TEST_ASSERT_TRUE(
    wait_for_map_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A provider that claims a request and releases it unanswered fails it, so the
// load reports an error instead of waiting forever.
static void releasing_a_claimed_request_without_a_response_fails_it(void) {
  expect_dropped_request_fails(false);
}

// The same holds when the provider releases the handle inside its callback
// before answering HANDLE.
static void releasing_a_request_inside_its_callback_then_claiming_fails_it(
  void
) {
  expect_dropped_request_fails(true);
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
  (void)mln_test_wait_for_flag(&state.callback_finished);
  TEST_ASSERT_TRUE(atomic_load(&state.callback_finished));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&state.completion_status));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct cancel_probe {
  atomic_bool provider_entered;
  atomic_int cancel_count;
  atomic_int release_count;
  atomic_bool released_before_cancel;
  atomic_bool release_inside_callback;
  atomic_bool complete_inline;
  atomic_bool skip_register;
  atomic_int register_status;
  atomic_bool register_reported_cancelled;
  _Atomic mln_resource_request_handle handle;
} cancel_probe;

static void count_cancel(void* user_data) {
  cancel_probe* probe = user_data;
  if (atomic_load(&probe->release_count) != 0) {
    mln_test_flag_set(&probe->released_before_cancel);
  }
  atomic_fetch_add(&probe->cancel_count, 1);
  if (atomic_load(&probe->release_inside_callback)) {
    mln_resource_request_release(atomic_load(&probe->handle));
  }
  mln_test_pulse();
}

static void count_cancel_release(void* user_data) {
  cancel_probe* probe = user_data;
  atomic_fetch_add(&probe->release_count, 1);
  mln_test_pulse();
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
      &probe->register_status,
      mln_resource_request_set_cancel_callback(
        handle, count_cancel, probe, count_cancel_release, &cancelled, NULL
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
    (void)mln_resource_request_complete(handle, &response, NULL);
  }
  mln_test_flag_set(&probe->provider_entered);
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
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe->provider_entered));
  if (!atomic_load(&probe->skip_register)) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe->register_status));
    TEST_ASSERT_FALSE(atomic_load(&probe->register_reported_cancelled));
  }
  return map;
}

typedef struct cancel_count_target {
  cancel_probe* probe;
  int expected;
} cancel_count_target;

static bool cancel_count_reached(void* context) {
  const cancel_count_target* target = context;
  return atomic_load(&target->probe->cancel_count) >= target->expected;
}

static bool wait_for_cancel_count(
  cancel_probe* probe, int expected, mln_test_deadline deadline
) {
  cancel_count_target target = {.probe = probe, .expected = expected};
  return mln_test_await(
    cancel_count_reached, &target, deadline, "the cancel callback"
  );
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
    wait_for_cancel_count(&probe, 1, mln_test_deadline_default())
  );
  const mln_resource_request_handle handle = atomic_load(&probe.handle);
  // The context retires as the callback returns, before the request does.
  (void)mln_test_wait_for_count(&probe.release_count, 1);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.release_count));
  TEST_ASSERT_FALSE(atomic_load(&probe.released_before_cancel));

  bool cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_resource_request_cancelled(handle, &cancelled, NULL)
  );
  TEST_ASSERT_TRUE(cancelled);
  const mln_resource_response response = style_response();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_resource_request_complete(handle, &response, NULL)
  );

  cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_resource_request_set_cancel_callback(
      handle, count_cancel, &probe, NULL, &cancelled, NULL
    )
  );
  TEST_ASSERT_FALSE(cancelled);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));

  mln_resource_request_release(handle);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_set_cancel_callback(
      handle, count_cancel, &probe, NULL, &cancelled, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

// A registration that arrives after cancellation stores nothing and reports
// the cancellation through out_cancelled instead of invoking the callback.
typedef struct cancelled_poll {
  mln_resource_request_handle handle;
  bool cancelled;
  mln_status status;
} cancelled_poll;

// Cancellation lands on a MapLibre thread with no callback registered to
// report it, so the wait re-checks the request's cancelled state.
static bool request_reports_cancelled(void* context) {
  cancelled_poll* poll = context;
  poll->status =
    mln_resource_request_cancelled(poll->handle, &poll->cancelled, NULL);
  return poll->status != MLN_STATUS_OK || poll->cancelled;
}

static void late_cancel_callback_registration_reports_cancelled(void) {
  cancel_probe probe = {0};
  mln_test_flag_set(&probe.skip_register);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);
  mln_test_destroy_map(map);
  const mln_resource_request_handle handle = atomic_load(&probe.handle);

  cancelled_poll poll = {.handle = handle, .status = MLN_STATUS_OK};
  (void)mln_test_await(
    request_reports_cancelled, &poll, mln_test_deadline_default(),
    "the request to report cancelled"
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, poll.status);
  bool cancelled = poll.cancelled;
  TEST_ASSERT_TRUE(cancelled);

  cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_resource_request_set_cancel_callback(
      handle, count_cancel, &probe, count_cancel_release, &cancelled, NULL
    )
  );
  TEST_ASSERT_TRUE(cancelled);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));

  // A registration the C API did not keep leaves user_data with the caller.
  mln_resource_request_release(handle);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

// The callback runs unlocked, so releasing the cancelled handle from inside it
// retires the request without deadlocking.
static void cancel_callback_may_release_the_request(void) {
  cancel_probe probe = {0};
  mln_test_flag_set(&probe.release_inside_callback);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);

  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(
    wait_for_cancel_count(&probe, 1, mln_test_deadline_default())
  );
  const mln_resource_request_handle handle = atomic_load(&probe.handle);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_resource_request_wait_until_retired(handle, NULL)
  );
  bool cancelled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_resource_request_set_cancel_callback(
      handle, count_cancel, &probe, NULL, &cancelled, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(custom_provider_request_handles_reject_raw_null_handles);
  RUN_TEST(network_status_get_rejects_raw_null_output);
  RUN_TEST(ambient_cache_operations_validate_raw_operation_values);
  RUN_TEST(set_maximum_ambient_cache_size_rejects_raw_null_output);
  RUN_TEST(offline_regions_reject_raw_invalid_descriptors);
  RUN_TEST(offline_database_merge_rejects_raw_null_path);
  RUN_TEST(offline_database_merge_rejects_a_missing_file);
  RUN_TEST(offline_database_merge_rejects_sqlite_pseudo_paths);
  RUN_TEST(offline_database_merge_reports_a_corrupt_side_database);
  RUN_TEST(offline_submission_never_waits_for_the_runtime_worker);
  RUN_TEST(offline_operations_report_a_missing_region);
  RUN_TEST(resource_transform_rejects_raw_invalid_descriptors);
  RUN_TEST(http_header_transform_registration_follows_the_transport);
  RUN_TEST(http_header_transform_rejects_raw_invalid_inputs);
  RUN_TEST(resource_provider_rejects_raw_invalid_descriptors);
  RUN_TEST(resource_provider_registration_releases_owned_state);
  RUN_TEST(resource_provider_command_copies_cross_thread_descriptor);
  RUN_TEST(unsupported_style_url_scheme_names_scheme_and_url);
  RUN_TEST(releasing_a_claimed_request_without_a_response_fails_it);
  RUN_TEST(releasing_a_request_inside_its_callback_then_claiming_fails_it);
  RUN_TEST(unsupported_style_url_diagnostic_redacts_credentials);
  RUN_TEST(unsupported_style_url_names_declining_provider);
  RUN_TEST(resource_provider_defers_inline_release_until_callback_returns);
  RUN_TEST(cancel_callback_runs_when_map_discards_request);
  RUN_TEST(late_cancel_callback_registration_reports_cancelled);
  RUN_TEST(cancel_callback_may_release_the_request);
}
