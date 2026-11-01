// Snippets for the docs site's Offline page. They compile with the native
// tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

static atomic_bool finished;

static void on_done(void* user_data, const mln_completion_result* result) {
  (void)user_data;
  (void)result;
  atomic_store(&finished, true);
}

static void wait_for_done(void) {
  while (!atomic_exchange(&finished, false)) {
  }
}

static void record_progress(
  mln_offline_region_id region_id, uint64_t completed, uint64_t required,
  bool complete
) {
  (void)region_id;
  (void)completed;
  (void)required;
  (void)complete;
}

static void record_error(
  mln_offline_region_id region_id, mln_buffer_view message
) {
  (void)region_id;
  (void)message;
}

static void record_region(
  mln_offline_region_id region_id, const uint8_t* metadata, size_t size
) {
  (void)region_id;
  (void)metadata;
  (void)size;
}

// #region download
static void on_region_created(
  void* user_data, const mln_completion_result* result
) {
  if (result->status == MLN_STATUS_OK) {
    const mln_offline_region_info* region = result->value;
    *(mln_offline_region_id*)user_data = region->id;
  }
  on_done(NULL, result);
}
// #endregion download

// #region progress
static void on_events(void* user_data, const mln_event_batch_view* batch) {
  (void)user_data;
  const uint8_t* bytes = (const uint8_t*)batch->events;
  for (size_t i = 0; i < batch->event_count; i += 1) {
    // Step by event_size, not by sizeof(mln_runtime_event).
    const mln_runtime_event* event =
      (const mln_runtime_event*)(bytes + i * batch->event_size);

    if (event->type == MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED) {
      const mln_runtime_event_offline_region_status* payload =
        &event->payload.offline_region_status;
      record_progress(
        payload->region_id, payload->status.completed_resource_count,
        payload->status.required_resource_count, payload->status.complete
      );
    } else if (event->type == MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR) {
      record_error(
        event->payload.offline_region_response_error.region_id, event->message
      );
    }
  }
}
// #endregion progress

// #region manage
static void on_regions(void* user_data, const mln_completion_result* result) {
  if (result->status == MLN_STATUS_OK) {
    const uint8_t* bytes = result->value;
    for (size_t i = 0; i < result->value_count; i += 1) {
      // Step by value_size, not by sizeof(mln_offline_region_info).
      const mln_offline_region_info* region =
        (const mln_offline_region_info*)(bytes + i * result->value_size);
      record_region(region->id, region->metadata, region->metadata_size);
    }
  }
  on_done(user_data, result);
}
// #endregion manage

void mln_doc_offline(void);

void mln_doc_offline(void) {
  const mln_completion done = {
    .size = sizeof(mln_completion), .callback = on_done
  };

  // #region cache-file
  mln_runtime_options runtime_options = {
    .size = sizeof(mln_runtime_options),
    .cache_path = mln_buffer_view_from_cstr("maplibre-cache.db"),
  };
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_runtime_create(&runtime_options, &runtime, NULL);
  // #endregion cache-file

  // #region progress
  const mln_runtime_event_handler handler = {
    .size = sizeof(mln_runtime_event_handler), .callback = on_events
  };
  mln_runtime_set_event_handler(runtime, &handler, NULL, NULL);
  // #endregion progress

  // #region ambient
  // 50 MiB
  mln_runtime_set_maximum_ambient_cache_size(
    runtime, 50ULL * 1024 * 1024, &done, NULL
  );
  // #endregion ambient
  wait_for_done();

  // #region ambient
  mln_runtime_run_ambient_cache_operation(
    runtime, MLN_AMBIENT_CACHE_OPERATION_CLEAR, &done, NULL
  );
  // #endregion ambient
  wait_for_done();

  // #region download
  const mln_offline_region_definition definition = {
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data.tile_pyramid = {
      .style_url =
        mln_buffer_view_from_cstr("https://demotiles.maplibre.org/style.json"),
      .bounds =
        {
          .southwest = {.latitude = 47.5, .longitude = -122.5},
          .northeast = {.latitude = 47.8, .longitude = -122.2},
        },
      .min_zoom = 0.0,
      .max_zoom = 6.0,
      .pixel_ratio = 1.0F,
    },
  };
  static const uint8_t metadata[] = "Seattle";

  mln_offline_region_id region_id = 0;
  const mln_completion created = {
    .size = sizeof(mln_completion),
    .callback = on_region_created,
    .user_data = &region_id,
  };
  mln_runtime_create_offline_region(
    runtime, &definition, metadata, sizeof(metadata) - 1, &created, NULL
  );
  // #endregion download
  wait_for_done();

  // #region download
  mln_runtime_set_offline_region_download_state(
    runtime, region_id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &done, NULL
  );
  // #endregion download
  wait_for_done();

  // #region progress
  mln_runtime_set_offline_region_observed(
    runtime, region_id, true, &done, NULL
  );
  // #endregion progress
  wait_for_done();

  // #region manage
  const mln_completion listed = {
    .size = sizeof(mln_completion), .callback = on_regions
  };
  mln_runtime_list_offline_regions(runtime, &listed, NULL);
  // #endregion manage
  wait_for_done();

  // #region manage
  // Resume a download that stopped when the runtime closed.
  mln_runtime_set_offline_region_download_state(
    runtime, region_id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &done, NULL
  );
  // #endregion manage
  wait_for_done();

  // #region manage
  mln_runtime_invalidate_offline_region(runtime, region_id, &done, NULL);
  // #endregion manage
  wait_for_done();

  // #region manage
  mln_runtime_delete_offline_region(runtime, region_id, &done, NULL);
  // #endregion manage
  wait_for_done();

  mln_runtime_release(runtime, &done, NULL);
  wait_for_done();
}
