// Offline regions: definition validation, the region lifecycle from creation
// to deletion, merging another database's regions, and a download that a
// resource provider serves.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

#include "support/harness.h"
#include "support/resources.h"
#include "support/tables.h"
#include "support/test_support.h"
#include "unity.h"

static const char offline_style_url[] = "custom://offline/style.json";
static const char offline_tile_url[] = "custom://offline/tiles/0/0/0.pbf";
static const char offline_style_json[] =
  "{\"version\":8,\"sources\":{\"tiles\":{\"type\":\"vector\",\"tiles\":"
  "[\"custom://offline/tiles/{z}/{x}/{y}.pbf\"],\"minzoom\":0,\"maxzoom\":0}},"
  "\"layers\":[{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"tiles\","
  "\"source-layer\":\"any\"}]}";
static const char line_geometry[] =
  "{\"type\":\"LineString\",\"coordinates\":[[2,1],[4,3]]}";

static mln_offline_region_definition tile_definition(void) {
  return (mln_offline_region_definition){
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data.tile_pyramid = {
      .size = sizeof(mln_offline_tile_pyramid_region_definition),
      .style_url = offline_style_url,
      .bounds =
        {
          .southwest = {.latitude = 1.0, .longitude = 2.0},
          .northeast = {.latitude = 3.0, .longitude = 4.0},
        },
      .min_zoom = 0.0,
      .max_zoom = 0.0,
      .pixel_ratio = 2.0F,
      .include_ideographs = true,
    },
  };
}

static mln_offline_region_definition geometry_definition(void) {
  return (mln_offline_region_definition){
    .size = sizeof(mln_offline_region_definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
    .data.geometry = {
      .size = sizeof(mln_offline_geometry_region_definition),
      .style_url = offline_style_url,
      .geometry = MLN_BUFFER_LITERAL(line_geometry),
      .min_zoom = 5.0,
      .max_zoom = 6.0,
      .pixel_ratio = 1.0F,
      .include_ideographs = false,
    },
  };
}

static mln_status submit_region(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  static const uint8_t metadata[] = {1, 2, 3};
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_offline_region_create(
    *(const mln_runtime*)context, descriptor, metadata, sizeof(metadata),
    &completion.descriptor, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    (void)mln_test_completion_finish(&completion);
  } else {
    mln_test_completion_reject(&completion);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

static void with_unknown_type(void* descriptor) {
  ((mln_offline_region_definition*)descriptor)->type = 999;
}

static void with_zero_size(void* descriptor) {
  ((mln_offline_region_definition*)descriptor)->size = 0;
}

static void pyramid_without_style(void* descriptor) {
  ((mln_offline_region_definition*)descriptor)->data.tile_pyramid.style_url =
    NULL;
}

static void as_geometry_without_style(void* descriptor) {
  mln_offline_region_definition* definition = descriptor;
  *definition = geometry_definition();
  definition->data.geometry.style_url = NULL;
}

static void as_geometry_without_bytes(void* descriptor) {
  mln_offline_region_definition* definition = descriptor;
  *definition = geometry_definition();
  definition->data.geometry.geometry = (mln_buffer_view){0};
}

static void as_geometry_that_is_not_json(void* descriptor) {
  mln_offline_region_definition* definition = descriptor;
  *definition = geometry_definition();
  definition->data.geometry.geometry = MLN_BUFFER_LITERAL("not json");
}

static const mln_test_validation_case definition_cases[] = {
  {"a tile pyramid", NULL, MLN_STATUS_OK, NULL},
  {"a zero size", with_zero_size, MLN_STATUS_INVALID_ARGUMENT, NULL},
  {"an unknown type", with_unknown_type, MLN_STATUS_INVALID_ARGUMENT, NULL},
  {"a pyramid without a style", pyramid_without_style,
   MLN_STATUS_INVALID_ARGUMENT, NULL},
  {"a geometry without a style", as_geometry_without_style,
   MLN_STATUS_INVALID_ARGUMENT, NULL},
  {"a geometry without bytes", as_geometry_without_bytes,
   MLN_STATUS_INVALID_ARGUMENT, NULL},
  {"a geometry that is not JSON", as_geometry_that_is_not_json,
   MLN_STATUS_INVALID_ARGUMENT, NULL},
};

static void offline_region_creation_validates_its_definition(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_offline_region_definition defaults = tile_definition();
  mln_test_run_validation_table(
    definition_cases, sizeof(definition_cases) / sizeof(definition_cases[0]),
    &defaults, sizeof(defaults), submit_region, &runtime
  );
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(runtime, NULL, NULL, 0, &completion, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_region_create(
      runtime, &defaults, NULL, 3, &completion, NULL
    )
  );
  mln_test_destroy_runtime(runtime);
}

// Deep copies of the regions an offline completion borrows.
#define REGION_CAPACITY 4

typedef struct region_copy {
  mln_offline_region_id id;
  uint32_t type;
  char style_url[128];
  mln_lat_lng_bounds bounds;
  double min_zoom;
  double max_zoom;
  float pixel_ratio;
  bool include_ideographs;
  char geometry[256];
  uint8_t metadata[16];
  size_t metadata_size;
} region_copy;

typedef struct region_probe {
  atomic_bool done;
  mln_status status;
  size_t count;
  bool overflowed;
  region_copy regions[REGION_CAPACITY];
} region_probe;

static void copy_regions(void* user_data, const mln_completion_result* result) {
  region_probe* probe = user_data;
  probe->status = result->status;
  probe->count = result->value_count;
  const mln_offline_region_info* infos = result->value;
  for (size_t index = 0; index < result->value_count; index += 1) {
    if (index >= REGION_CAPACITY) {
      probe->overflowed = true;
      break;
    }
    const mln_offline_region_info* info = &infos[index];
    region_copy* copy = &probe->regions[index];
    copy->id = info->id;
    copy->type = info->definition.type;
    const char* style_url = NULL;
    if (info->definition.type == MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID) {
      const mln_offline_tile_pyramid_region_definition* pyramid =
        &info->definition.data.tile_pyramid;
      style_url = pyramid->style_url;
      copy->bounds = pyramid->bounds;
      copy->min_zoom = pyramid->min_zoom;
      copy->max_zoom = pyramid->max_zoom;
      copy->pixel_ratio = pyramid->pixel_ratio;
      copy->include_ideographs = pyramid->include_ideographs;
    } else {
      const mln_offline_geometry_region_definition* geometry =
        &info->definition.data.geometry;
      style_url = geometry->style_url;
      copy->min_zoom = geometry->min_zoom;
      copy->max_zoom = geometry->max_zoom;
      copy->pixel_ratio = geometry->pixel_ratio;
      copy->include_ideographs = geometry->include_ideographs;
      if (geometry->geometry.size < sizeof(copy->geometry)) {
        memcpy(
          copy->geometry, geometry->geometry.data, geometry->geometry.size
        );
      } else {
        probe->overflowed = true;
      }
    }
    (void)snprintf(
      copy->style_url, sizeof(copy->style_url), "%s",
      style_url == NULL ? "" : style_url
    );
    if (info->metadata_size <= sizeof(copy->metadata)) {
      if (info->metadata_size != 0) {
        memcpy(copy->metadata, info->metadata, info->metadata_size);
      }
      copy->metadata_size = info->metadata_size;
    } else {
      probe->overflowed = true;
    }
  }
  mln_test_flag_set(&probe->done);
}

static void release_region_probe(void* user_data) { (void)user_data; }

// Heap-allocated so a delivery after a failed wait writes into leaked memory.
static region_probe* new_region_probe(mln_completion* out_completion) {
  region_probe* probe = calloc(1, sizeof(*probe));
  TEST_ASSERT_NOT_NULL(probe);
  atomic_init(&probe->done, false);
  probe->status = MLN_STATUS_INVALID_STATE;
  *out_completion = (mln_completion){
    .size = sizeof(mln_completion),
    .callback = copy_regions,
    .user_data = probe,
    .release_user_data = release_region_probe,
  };
  return probe;
}

// Waits for the probe's delivery and returns it; the caller frees it.
static region_probe* finish_region_probe(
  mln_status submission, region_probe* probe
) {
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    MLN_STATUS_OK, submission, mln_test_last_error()
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe->done));
  TEST_ASSERT_FALSE_MESSAGE(probe->overflowed, "a region did not fit its copy");
  return probe;
}

static region_probe* create_region(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size
) {
  mln_completion completion;
  region_probe* probe = new_region_probe(&completion);
  return finish_region_probe(
    mln_runtime_offline_region_create(
      runtime, definition, metadata, metadata_size, &completion,
      MLN_TEST_DIAGNOSTIC
    ),
    probe
  );
}

static region_probe* list_regions(mln_runtime runtime) {
  mln_completion completion;
  region_probe* probe = new_region_probe(&completion);
  return finish_region_probe(
    mln_runtime_offline_regions_list(runtime, &completion, MLN_TEST_DIAGNOSTIC),
    probe
  );
}

static region_probe* get_region(mln_runtime runtime, mln_offline_region_id id) {
  mln_completion completion;
  region_probe* probe = new_region_probe(&completion);
  return finish_region_probe(
    mln_runtime_offline_region_get(
      runtime, id, &completion, MLN_TEST_DIAGNOSTIC
    ),
    probe
  );
}

static region_probe* merge_database(mln_runtime runtime, const char* path) {
  mln_completion completion;
  region_probe* probe = new_region_probe(&completion);
  return finish_region_probe(
    mln_runtime_offline_regions_merge_database(
      runtime, path, &completion, MLN_TEST_DIAGNOSTIC
    ),
    probe
  );
}

static const region_copy* find_region(
  const region_probe* probe, mln_offline_region_id id
) {
  for (size_t index = 0; index < probe->count; index += 1) {
    if (probe->regions[index].id == id) {
      return &probe->regions[index];
    }
  }
  return NULL;
}

static void expect_tile_region(
  const region_copy* region, const uint8_t* metadata, size_t metadata_size
) {
  TEST_ASSERT_NOT_NULL(region);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID, region->type
  );
  TEST_ASSERT_EQUAL_STRING(offline_style_url, region->style_url);
  TEST_ASSERT_EQUAL_DOUBLE(1.0, region->bounds.southwest.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(2.0, region->bounds.southwest.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(3.0, region->bounds.northeast.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(4.0, region->bounds.northeast.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, region->min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, region->max_zoom);
  TEST_ASSERT_EQUAL_FLOAT(2.0F, region->pixel_ratio);
  TEST_ASSERT_TRUE(region->include_ideographs);
  TEST_ASSERT_EQUAL_size_t(metadata_size, region->metadata_size);
  if (metadata_size != 0) {
    TEST_ASSERT_EQUAL_MEMORY(metadata, region->metadata, metadata_size);
  }
}

typedef struct status_probe {
  atomic_bool done;
  mln_status status;
  size_t count;
  mln_offline_region_status value;
} status_probe;

static void copy_status(void* user_data, const mln_completion_result* result) {
  status_probe* probe = user_data;
  probe->status = result->status;
  probe->count = result->value_count;
  if (result->value != NULL) {
    memcpy(&probe->value, result->value, sizeof(probe->value));
  }
  mln_test_flag_set(&probe->done);
}

// The region's status, or MLN_STATUS_NOT_FOUND and a zeroed status.
static mln_status get_region_status(
  mln_runtime runtime, mln_offline_region_id id,
  mln_offline_region_status* out_status
) {
  status_probe* probe = calloc(1, sizeof(*probe));
  TEST_ASSERT_NOT_NULL(probe);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_status,
    .user_data = probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_region_get_status(
                     runtime, id, &completion, MLN_TEST_DIAGNOSTIC
                   )
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe->done));
  const mln_status status = probe->status;
  *out_status = probe->value;
  free(probe);
  return status;
}

// A region from creation through listing, lookup, a metadata update, and its
// status to deletion, ending with the region gone and its status reporting
// so. The download case covers observation, download state, and invalidation.
static void an_offline_region_lives_from_creation_to_deletion(void) {
  static const uint8_t metadata[] = {1, 2, 3};
  static const uint8_t updated_metadata[] = {9, 8};
  mln_runtime runtime = mln_test_create_runtime();

  const mln_offline_region_definition pyramid = tile_definition();
  region_probe* created = create_region(runtime, &pyramid, metadata, 3);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, created->status);
  TEST_ASSERT_EQUAL_size_t(1, created->count);
  const mln_offline_region_id id = created->regions[0].id;
  expect_tile_region(&created->regions[0], metadata, 3);
  free(created);

  const mln_offline_region_definition geometry = geometry_definition();
  region_probe* shaped = create_region(runtime, &geometry, NULL, 0);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, shaped->status);
  const region_copy shaped_region = shaped->regions[0];
  free(shaped);
  TEST_ASSERT_NOT_EQUAL_INT64(id, shaped_region.id);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_OFFLINE_REGION_DEFINITION_GEOMETRY, shaped_region.type
  );
  TEST_ASSERT_NOT_NULL(strstr(shaped_region.geometry, "LineString"));
  TEST_ASSERT_EQUAL_DOUBLE(5.0, shaped_region.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(6.0, shaped_region.max_zoom);
  TEST_ASSERT_EQUAL_size_t(0, shaped_region.metadata_size);

  region_probe* listed = list_regions(runtime);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, listed->status);
  TEST_ASSERT_EQUAL_size_t(2, listed->count);
  expect_tile_region(find_region(listed, id), metadata, 3);
  TEST_ASSERT_NOT_NULL(find_region(listed, shaped_region.id));
  free(listed);

  region_probe* fetched = get_region(runtime, id);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, fetched->status);
  TEST_ASSERT_EQUAL_size_t(1, fetched->count);
  expect_tile_region(&fetched->regions[0], metadata, 3);
  free(fetched);

  mln_completion update_completion;
  region_probe* updated = new_region_probe(&update_completion);
  updated = finish_region_probe(
    mln_runtime_offline_region_update_metadata(
      runtime, id, updated_metadata, sizeof(updated_metadata),
      &update_completion, MLN_TEST_DIAGNOSTIC
    ),
    updated
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, updated->status);
  expect_tile_region(&updated->regions[0], updated_metadata, 2);
  free(updated);
  fetched = get_region(runtime, id);
  expect_tile_region(&fetched->regions[0], updated_metadata, 2);
  free(fetched);

  mln_offline_region_status status = {0};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, get_region_status(runtime, id, &status));
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_offline_region_status), status.size);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE, status.download_state
  );
  TEST_ASSERT_FALSE(status.complete);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_runtime_offline_region_delete(runtime, id, &completion.descriptor, NULL)
  );

  fetched = get_region(runtime, id);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, fetched->status);
  TEST_ASSERT_EQUAL_size_t(0, fetched->count);
  free(fetched);
  listed = list_regions(runtime);
  TEST_ASSERT_EQUAL_size_t(1, listed->count);
  TEST_ASSERT_EQUAL_INT64(shaped_region.id, listed->regions[0].id);
  free(listed);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, get_region_status(runtime, id, &status)
  );
  mln_test_destroy_runtime(runtime);
}

// Submits one operation that names `region`, and returns its terminal status.
typedef mln_status (*region_operation)(
  mln_runtime runtime, mln_offline_region_id region,
  const mln_completion* completion
);

static mln_status update_missing(
  mln_runtime runtime, mln_offline_region_id region,
  const mln_completion* completion
) {
  static const uint8_t metadata[] = {4, 5, 6};
  return mln_runtime_offline_region_update_metadata(
    runtime, region, metadata, sizeof(metadata), completion, NULL
  );
}

static mln_status status_of_missing(
  mln_runtime runtime, mln_offline_region_id region,
  const mln_completion* completion
) {
  return mln_runtime_offline_region_get_status(
    runtime, region, completion, NULL
  );
}

static mln_status observe_missing(
  mln_runtime runtime, mln_offline_region_id region,
  const mln_completion* completion
) {
  return mln_runtime_offline_region_set_observed(
    runtime, region, true, completion, NULL
  );
}

static mln_status download_missing(
  mln_runtime runtime, mln_offline_region_id region,
  const mln_completion* completion
) {
  return mln_runtime_offline_region_set_download_state(
    runtime, region, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, completion, NULL
  );
}

static mln_status invalidate_missing(
  mln_runtime runtime, mln_offline_region_id region,
  const mln_completion* completion
) {
  return mln_runtime_offline_region_invalidate(
    runtime, region, completion, NULL
  );
}

static mln_status delete_missing(
  mln_runtime runtime, mln_offline_region_id region,
  const mln_completion* completion
) {
  return mln_runtime_offline_region_delete(runtime, region, completion, NULL);
}

static const struct {
  const char* label;
  region_operation operation;
} missing_region_operations[] = {
  {"update metadata", update_missing}, {"get status", status_of_missing},
  {"set observed", observe_missing},   {"set download state", download_missing},
  {"invalidate", invalidate_missing},  {"delete", delete_missing},
};

// Every operation that names a region reports one that does not exist as
// NOT_FOUND. A get reports it as an empty success instead.
static void offline_operations_report_a_missing_region(void) {
  static const mln_offline_region_id missing = 987654321;
  mln_runtime runtime = mln_test_create_runtime();

  region_probe* fetched = get_region(runtime, missing);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, fetched->status);
  TEST_ASSERT_EQUAL_size_t(0, fetched->count);
  free(fetched);

  for (size_t index = 0; index < sizeof(missing_region_operations) /
                                   sizeof(missing_region_operations[0]);
       index += 1) {
    mln_test_completion completion = mln_test_completion_default(0);
    const mln_status submission = missing_region_operations[index].operation(
      runtime, missing, &completion.descriptor
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, submission, missing_region_operations[index].label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_NOT_FOUND, mln_test_completion_finish(&completion),
      missing_region_operations[index].label
    );
    mln_test_completion_destroy(&completion);
  }

  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_offline_region_set_download_state(
                                   runtime, missing, 999, &completion, NULL
                                 )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_offline_region_update_metadata(
                                   runtime, missing, NULL, 3, &completion, NULL
                                 )
  );
  mln_test_destroy_runtime(runtime);
}

static void offline_database_merge_rejects_what_names_no_database_file(void) {
  static const char missing_name[] = "missing-side-database.db";
  char missing_path[1024];
  mln_test_temp_path(missing_name, missing_path, sizeof(missing_path));
  const char* const unreadable_paths[] = {
    missing_path, "", ":memory:", "file::memory:"
  };
  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_offline_regions_merge_database(runtime, NULL, &completion, NULL)
  );
  for (size_t index = 0;
       index < sizeof(unreadable_paths) / sizeof(unreadable_paths[0]);
       index += 1) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_runtime_offline_regions_merge_database(
        runtime, unreadable_paths[index], &completion, MLN_TEST_DIAGNOSTIC
      ),
      unreadable_paths[index]
    );
    // The rejection happens on the calling thread, so its diagnostic is
    // readable here rather than through the completion.
    TEST_ASSERT_NOT_NULL(
      strstr(mln_test_last_error(), "readable database file")
    );
  }
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

static uint8_t* read_file(const char* path, size_t* out_size) {
  *out_size = 0;
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    return NULL;
  }
  uint8_t* bytes = NULL;
  size_t size = 0;
  uint8_t chunk[4096];
  size_t read = 0;
  while ((read = fread(chunk, 1, sizeof(chunk), file)) > 0) {
    uint8_t* grown = realloc(bytes, size + read);
    if (grown == NULL) {
      free(bytes);
      fclose(file);
      return NULL;
    }
    bytes = grown;
    memcpy(bytes + size, chunk, read);
    size += read;
  }
  fclose(file);
  *out_size = size;
  return bytes;
}

static void make_read_only(const char* path) {
#if defined(_WIN32)
  TEST_ASSERT_EQUAL_INT(0, _chmod(path, _S_IREAD));
#else
  TEST_ASSERT_EQUAL_INT(0, chmod(path, S_IRUSR | S_IRGRP | S_IROTH));
#endif
}

static void remove_database(const char* path) {
#if defined(_WIN32)
  (void)_chmod(path, _S_IREAD | _S_IWRITE);
#else
  (void)chmod(path, S_IRUSR | S_IWUSR);
#endif
  (void)remove(path);
}

// Writes a side database holding one region with `metadata`, through a runtime
// whose cache lives at `path`. Returns the region's ID there.
static mln_offline_region_id write_side_database(
  const char* path, const uint8_t* metadata, size_t metadata_size
) {
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = path;
  mln_runtime side = mln_test_create_runtime_with_options(options);
  const mln_offline_region_definition definition = tile_definition();
  region_probe* created =
    create_region(side, &definition, metadata, metadata_size);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, created->status);
  const mln_offline_region_id id = created->regions[0].id;
  free(created);
  // The release completes once the runtime has closed its database.
  mln_test_destroy_runtime(side);
  return id;
}

// A merge copies the side database's regions into the runtime's database and
// reports them. The side database opens read-only, so a file the process
// cannot write merges too, and its bytes are unchanged afterwards.
static void offline_database_merge_copies_regions_from_a_read_only_file(void) {
  static const uint8_t metadata[] = {7, 7, 7};
  char side_path[1024];
  mln_test_temp_path("side.db", side_path, sizeof(side_path));
  (void)write_side_database(side_path, metadata, sizeof(metadata));
  make_read_only(side_path);
  size_t before_size = 0;
  uint8_t* before = read_file(side_path, &before_size);
  TEST_ASSERT_NOT_NULL(before);

  mln_runtime runtime = mln_test_create_runtime();
  region_probe* merged = merge_database(runtime, side_path);
  TEST_ASSERT_EQUAL_INT_MESSAGE(MLN_STATUS_OK, merged->status, side_path);
  TEST_ASSERT_EQUAL_size_t(1, merged->count);
  const mln_offline_region_id merged_id = merged->regions[0].id;
  expect_tile_region(&merged->regions[0], metadata, sizeof(metadata));
  free(merged);

  region_probe* listed = list_regions(runtime);
  TEST_ASSERT_EQUAL_size_t(1, listed->count);
  expect_tile_region(
    find_region(listed, merged_id), metadata, sizeof(metadata)
  );
  free(listed);
  mln_test_destroy_runtime(runtime);

  size_t after_size = 0;
  uint8_t* after = read_file(side_path, &after_size);
  TEST_ASSERT_NOT_NULL(after);
  TEST_ASSERT_EQUAL_size_t(before_size, after_size);
  TEST_ASSERT_EQUAL_MEMORY(before, after, before_size);
  free(before);
  free(after);
  remove_database(side_path);
}

typedef struct download_watch {
  mln_offline_region_id region;
  bool complete;
  mln_offline_region_status status;
} download_watch;

static bool download_completed(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  download_watch* watch = context;
  if (
    event->type != MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED ||
    event->payload.offline_region_status.region_id != watch->region
  ) {
    return false;
  }
  watch->status = event->payload.offline_region_status.status;
  watch->complete = watch->status.complete;
  return watch->complete;
}

// An observed download fetches the region's style and tiles through the
// provider, reports its progress until the region is complete, and leaves a
// status that says so. Invalidating the region then expires what it stored.
static void a_download_completes_from_provider_served_resources(void) {
  static const mln_test_provided_resource resources[] = {
    {.url = offline_style_url,
     .response =
       {
         .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
         .bytes = (const uint8_t*)offline_style_json,
         .byte_count = sizeof(offline_style_json) - 1,
       }},
    {.url = offline_tile_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)"tile",
       .byte_count = 4,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 2);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);

  const mln_offline_region_definition definition = tile_definition();
  region_probe* created = create_region(runtime, &definition, NULL, 0);
  const mln_offline_region_id id = created->regions[0].id;
  free(created);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_runtime_offline_region_set_observed(
                     runtime, id, true, &completion.descriptor, NULL
                   )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_runtime_offline_region_set_download_state(
                     runtime, id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
                     &completion.descriptor, NULL
                   )
  );
  download_watch watch = {.region = id};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_await_event_matching(
      runtime, download_completed, &watch, mln_test_deadline_default()
    ),
    "the download never reported completion"
  );
  TEST_ASSERT_EQUAL_UINT64(1, watch.status.required_tile_count);
  TEST_ASSERT_EQUAL_UINT64(1, watch.status.completed_tile_count);
  TEST_ASSERT_EQUAL_UINT64(4, watch.status.completed_tile_size);
  TEST_ASSERT_EQUAL_UINT64(
    watch.status.required_resource_count, watch.status.completed_resource_count
  );
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_provider_requests(provider, offline_tile_url)
  );
  const mln_test_provider_request* style_request =
    mln_test_provider_request_at(provider, offline_style_url, 0);
  TEST_ASSERT_NOT_NULL(style_request);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_USAGE_OFFLINE, style_request->usage);

  mln_offline_region_status status = {0};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, get_region_status(runtime, id, &status));
  TEST_ASSERT_TRUE(status.complete);
  TEST_ASSERT_EQUAL_UINT64(1, status.completed_tile_count);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_runtime_offline_region_set_download_state(
                     runtime, id, MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
                     &completion.descriptor, NULL
                   )
  );

  // A map reads the downloaded style as a usable cached copy, and only asks
  // the provider to revalidate it. Once the region is invalidated, the copy
  // has expired, so the map asks for the style again and offers that copy.
  mln_map before = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(before, offline_style_url)
  );
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, before));
  // The style loads from the cached copy before the revalidation reaches the
  // provider.
  TEST_ASSERT_TRUE(
    mln_test_provider_wait_for_requests(provider, offline_style_url, 2)
  );
  const mln_test_provider_request* revalidation =
    mln_test_provider_request_at(provider, offline_style_url, 1);
  TEST_ASSERT_NOT_NULL(revalidation);
  TEST_ASSERT_FALSE(revalidation->has_prior_expires);
  TEST_ASSERT_EQUAL_size_t(0, revalidation->prior_data_size);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_runtime_offline_region_invalidate(
                     runtime, id, &completion.descriptor, NULL
                   )
  );
  mln_map after = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(after, offline_style_url)
  );
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, after));
  const mln_test_provider_request* refetch =
    mln_test_provider_request_at(provider, offline_style_url, 2);
  TEST_ASSERT_NOT_NULL(refetch);
  TEST_ASSERT_TRUE(refetch->has_prior_expires);
  TEST_ASSERT_EQUAL_INT64(0, refetch->prior_expires_unix_ms);
  TEST_ASSERT_EQUAL_size_t(
    sizeof(offline_style_json) - 1, refetch->prior_data_size
  );

  mln_test_destroy_map(after);
  mln_test_destroy_map(before);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
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
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.finished));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe.reentrant_status));

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&list));
  mln_test_completion_destroy(&list);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(offline_region_creation_validates_its_definition);
  RUN_TEST(an_offline_region_lives_from_creation_to_deletion);
  RUN_TEST(offline_operations_report_a_missing_region);
  RUN_TEST(offline_database_merge_rejects_what_names_no_database_file);
  RUN_TEST(offline_database_merge_reports_a_corrupt_side_database);
  RUN_TEST(offline_database_merge_copies_regions_from_a_read_only_file);
  RUN_TEST(a_download_completes_from_provider_served_resources);
  RUN_TEST(offline_submission_never_waits_for_the_runtime_worker);
}
