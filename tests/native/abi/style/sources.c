// Style sources: tile source options and the effective values a source reports,
// the URL and attribution copies, source listing, volatility, removal, and
// image sources.
//
// URL sources start loading as soon as they are added, so every case that adds
// one serves the suite's resources through a provider that fails the rest.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "support/harness.h"
#include "support/resources.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

// Fails every request, for cases whose URL sources must never load.
static void serve_nothing(mln_runtime runtime) {
  mln_test_style_serve(runtime, NULL, 0);
}

// Reads a source's metadata, and reports whether the source exists.
static bool read_source(
  mln_map map, const char* id, mln_style_source_result* out
) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_source_result));
  MLN_TEST_OK(mln_map_get_style_source_info(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  *out = (mln_style_source_result){0};
  if (found) {
    TEST_ASSERT_TRUE(
      mln_test_completion_copy_value(&completion, out, sizeof(*out))
    );
  }
  mln_test_completion_destroy(&completion);
  return found;
}

static bool source_exists(mln_map map, const char* id) {
  mln_style_source_result result;
  return read_source(map, id, &result);
}

enum tile_source_kind {
  TILE_SOURCE_VECTOR,
  TILE_SOURCE_RASTER,
  TILE_SOURCE_RASTER_DEM,
};

static const mln_buffer_view fixture_tiles[] = {
  MLN_BUFFER_LITERAL("fixture://tiles/{z}/{x}/{y}"),
};

static mln_status add_tile_source(
  mln_map map, enum tile_source_kind kind, const char* id, bool from_url,
  const mln_style_tile_source_options* options,
  const mln_completion* completion, mln_diagnostic* diagnostic
) {
  const mln_buffer_view source_id = mln_test_view_of(id);
  const mln_buffer_view url = MLN_BUFFER_LITERAL("fixture://tiles.json");
  switch (kind) {
    case TILE_SOURCE_VECTOR:
      return from_url ? mln_map_add_vector_source_url(
                          map, source_id, url, options, completion, diagnostic
                        )
                      : mln_map_add_vector_source_tiles(
                          map, source_id, fixture_tiles, 1, options, completion,
                          diagnostic
                        );
    case TILE_SOURCE_RASTER:
      return from_url ? mln_map_add_raster_source_url(
                          map, source_id, url, options, completion, diagnostic
                        )
                      : mln_map_add_raster_source_tiles(
                          map, source_id, fixture_tiles, 1, options, completion,
                          diagnostic
                        );
    case TILE_SOURCE_RASTER_DEM:
      return from_url ? mln_map_add_raster_dem_source_url(
                          map, source_id, url, options, completion, diagnostic
                        )
                      : mln_map_add_raster_dem_source_tiles(
                          map, source_id, fixture_tiles, 1, options, completion,
                          diagnostic
                        );
  }
  return MLN_STATUS_NATIVE_ERROR;
}

static void explicit_zoom_scheme_and_bounds(
  mln_style_tile_source_options* options
) {
  options->fields |= MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM |
                     MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM |
                     MLN_STYLE_TILE_SOURCE_OPTION_SCHEME |
                     MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
  options->min_zoom = 2.0;
  options->max_zoom = 12.0;
  options->scheme = MLN_STYLE_TILE_SCHEME_TMS;
  options->bounds = (mln_lat_lng_bounds){
    .southwest = {.latitude = -10.0, .longitude = -20.0},
    .northeast = {.latitude = 10.0, .longitude = 20.0},
  };
}
static void mlt_encoding(mln_style_tile_source_options* options) {
  options->fields |= MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
  options->vector_encoding = MLN_STYLE_VECTOR_TILE_ENCODING_MLT;
}
static void small_tiles(mln_style_tile_source_options* options) {
  options->fields |= MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
  options->tile_size = 256;
}
// A URL source takes its zoom range and encoding from the options, since its
// TileJSON has not loaded when the source is added.
static void url_zoom_and_mlt(mln_style_tile_source_options* options) {
  options->fields |= MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM |
                     MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM |
                     MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
  options->min_zoom = 2.0;
  options->max_zoom = 12.0;
  options->scheme = MLN_STYLE_TILE_SCHEME_XYZ;
  mlt_encoding(options);
}
static void terrarium_encoding(mln_style_tile_source_options* options) {
  options->fields |= MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  options->raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM;
}
static void mapbox_encoding(mln_style_tile_source_options* options) {
  options->fields |= MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  options->raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
}
static void xyz_scheme(mln_style_tile_source_options* options) {
  options->fields |= MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
  options->scheme = MLN_STYLE_TILE_SCHEME_XYZ;
}

// One source a row adds and the metadata it must then report. A zero field
// in `absent` is not checked; every bit in it must be missing from fields.
typedef struct effective_options_case {
  const char* label;
  enum tile_source_kind kind;
  bool from_url;
  // Edits the defaults; null passes null options.
  void (*mutate)(mln_style_tile_source_options* options);
  uint32_t type;
  uint32_t present;
  uint32_t absent;
  mln_style_tile_source_options expected;
} effective_options_case;

// A source reports what its options set and the documented default for what
// they omit, including null options, and reports an encoding only for the
// source kinds that have one.
static void tile_sources_report_their_effective_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  serve_nothing(runtime);
  const mln_style_tile_source_options defaults =
    mln_style_tile_source_options_default();
  mln_style_tile_source_options explicit_options = defaults;
  explicit_zoom_scheme_and_bounds(&explicit_options);
  mln_style_tile_source_options small = defaults;
  small_tiles(&small);
  mln_style_tile_source_options mlt = defaults;
  mlt_encoding(&mlt);
  mln_style_tile_source_options terrarium = defaults;
  terrarium_encoding(&terrarium);
  mln_style_tile_source_options url_mlt = defaults;
  url_zoom_and_mlt(&url_mlt);
  mln_style_tile_source_options mapbox = defaults;
  mapbox_encoding(&mapbox);
  mln_style_tile_source_options xyz = defaults;
  xyz_scheme(&xyz);

  const uint32_t tilejson =
    MLN_STYLE_SOURCE_INFO_TILEJSON | MLN_STYLE_SOURCE_INFO_TILE_SIZE;
  const effective_options_case cases[] = {
    {"vector tiles, null options", TILE_SOURCE_VECTOR, false, NULL,
     MLN_STYLE_SOURCE_TYPE_VECTOR,
     tilejson | MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
     MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_BOUNDS |
       MLN_STYLE_SOURCE_INFO_RASTER_ENCODING,
     defaults},
    {"vector tiles, explicit zoom, scheme, and bounds", TILE_SOURCE_VECTOR,
     false, explicit_zoom_scheme_and_bounds, MLN_STYLE_SOURCE_TYPE_VECTOR,
     tilejson | MLN_STYLE_SOURCE_INFO_BOUNDS, 0, explicit_options},
    {"vector tiles, MLT", TILE_SOURCE_VECTOR, false, mlt_encoding,
     MLN_STYLE_SOURCE_TYPE_VECTOR,
     tilejson | MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING, 0, mlt},
    {"raster tiles, null options", TILE_SOURCE_RASTER, false, NULL,
     MLN_STYLE_SOURCE_TYPE_RASTER, tilejson,
     MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING |
       MLN_STYLE_SOURCE_INFO_RASTER_ENCODING,
     defaults},
    {"raster tiles, 256 px", TILE_SOURCE_RASTER, false, small_tiles,
     MLN_STYLE_SOURCE_TYPE_RASTER, tilejson, 0, small},
    {"raster DEM tiles, null options", TILE_SOURCE_RASTER_DEM, false, NULL,
     MLN_STYLE_SOURCE_TYPE_RASTER_DEM, tilejson,
     MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING |
       MLN_STYLE_SOURCE_INFO_RASTER_ENCODING,
     defaults},
    {"raster DEM tiles, Terrarium", TILE_SOURCE_RASTER_DEM, false,
     terrarium_encoding, MLN_STYLE_SOURCE_TYPE_RASTER_DEM,
     tilejson | MLN_STYLE_SOURCE_INFO_RASTER_ENCODING, 0, terrarium},
    {"raster DEM tiles, Mapbox", TILE_SOURCE_RASTER_DEM, false, mapbox_encoding,
     MLN_STYLE_SOURCE_TYPE_RASTER_DEM,
     tilejson | MLN_STYLE_SOURCE_INFO_RASTER_ENCODING, 0, mapbox},
    {"raster tiles, explicit XYZ", TILE_SOURCE_RASTER, false, xyz_scheme,
     MLN_STYLE_SOURCE_TYPE_RASTER, tilejson, 0, xyz},
    {"vector URL, null options", TILE_SOURCE_VECTOR, true, NULL,
     MLN_STYLE_SOURCE_TYPE_VECTOR,
     MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_TILE_SIZE |
       MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
     MLN_STYLE_SOURCE_INFO_TILEJSON, defaults},
    {"vector URL, zoom range and MLT", TILE_SOURCE_VECTOR, true,
     url_zoom_and_mlt, MLN_STYLE_SOURCE_TYPE_VECTOR,
     MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_TILE_SIZE |
       MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
     MLN_STYLE_SOURCE_INFO_TILEJSON, url_mlt},
    {"raster URL, 256 px", TILE_SOURCE_RASTER, true, small_tiles,
     MLN_STYLE_SOURCE_TYPE_RASTER,
     MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_TILE_SIZE,
     MLN_STYLE_SOURCE_INFO_TILEJSON, small},
    {"raster DEM URL, null options", TILE_SOURCE_RASTER_DEM, true, NULL,
     MLN_STYLE_SOURCE_TYPE_RASTER_DEM,
     MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_TILE_SIZE,
     MLN_STYLE_SOURCE_INFO_TILEJSON, defaults},
  };

  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index += 1) {
    const effective_options_case* row = &cases[index];
    char id[16];
    snprintf(id, sizeof(id), "source-%zu", index);
    mln_style_tile_source_options options = defaults;
    if (row->mutate != NULL) {
      row->mutate(&options);
    }
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_OK_MESSAGE(
      add_tile_source(
        map, row->kind, id, row->from_url,
        row->mutate == NULL ? NULL : &options, &completion.descriptor, NULL
      ),
      row->label
    );
    MLN_TEST_OK_MESSAGE(mln_test_completion_settle(&completion), row->label);

    mln_style_source_result result;
    TEST_ASSERT_TRUE_MESSAGE(read_source(map, id, &result), row->label);
    const mln_style_source_info* info = &result.info;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(row->type, info->type, row->label);
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(
      row->present, info->fields & row->present, row->label
    );
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(0, info->fields & row->absent, row->label);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      row->expected.tile_size, info->tile_size, row->label
    );
    if ((info->fields & MLN_STYLE_SOURCE_INFO_TILEJSON) != 0) {
      TEST_ASSERT_EQUAL_size_t_MESSAGE(1, info->tile_count, row->label);
      TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
        row->expected.min_zoom, info->min_zoom, row->label
      );
      TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
        row->expected.max_zoom, info->max_zoom, row->label
      );
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        row->expected.scheme, info->scheme, row->label
      );
    }
    if ((info->fields & MLN_STYLE_SOURCE_INFO_BOUNDS) != 0) {
      TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
        &row->expected.bounds, &info->bounds, sizeof(info->bounds), row->label
      );
    }
    if ((info->fields & MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) != 0) {
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        row->expected.vector_encoding, info->vector_encoding, row->label
      );
    }
    if ((info->fields & MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0) {
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        row->expected.raster_encoding, info->raster_encoding, row->label
      );
    }
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The call a validation row submits: one source kind and its options.
typedef struct tile_source_call {
  enum tile_source_kind kind;
  mln_style_tile_source_options options;
} tile_source_call;

static mln_status submit_tile_source_call(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const tile_source_call* call = descriptor;
  const mln_completion completion = mln_test_discard_completion();
  return add_tile_source(
    *(const mln_map*)context, call->kind, "validated", false, &call->options,
    &completion, diagnostic
  );
}

static void as_vector(void* descriptor) {
  ((tile_source_call*)descriptor)->kind = TILE_SOURCE_VECTOR;
}
static void undersized_options(void* descriptor) {
  ((tile_source_call*)descriptor)->options.size -= 1;
}
static void unknown_option_bit(void* descriptor) {
  ((tile_source_call*)descriptor)->options.fields |= UINT32_C(1) << 31;
}
static void negative_min_zoom(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
  call->options.min_zoom = -1.0;
}
static void min_zoom_above_max_zoom(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM |
                          MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
  call->options.min_zoom = 10.0;
  call->options.max_zoom = 4.0;
}
static void unknown_scheme(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
  call->options.scheme = 7;
}
static void zero_tile_size(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
  call->options.tile_size = 0;
}
static void oversized_tile_size(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
  call->options.tile_size = 65536;
}
static void null_attribution_bytes(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
  call->options.attribution = (mln_buffer_view){.data = NULL, .size = 3};
}
static void out_of_range_bounds(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
  call->options.bounds = (mln_lat_lng_bounds){
    .southwest = {.latitude = -100.0, .longitude = 0.0},
    .northeast = {.latitude = 10.0, .longitude = 10.0},
  };
}
static void vector_encoding_on_raster_dem(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
  call->options.vector_encoding = MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
}
static void unknown_vector_encoding(void* descriptor) {
  tile_source_call* call = descriptor;
  as_vector(descriptor);
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
  call->options.vector_encoding = 9;
}
static void raster_encoding_on_vector(void* descriptor) {
  as_vector(descriptor);
  terrarium_encoding(&((tile_source_call*)descriptor)->options);
}
static void unknown_raster_encoding(void* descriptor) {
  tile_source_call* call = descriptor;
  call->options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  call->options.raster_encoding = 9;
}

// Tile source options are checked before the call returns, so a malformed
// set never reaches the map worker.
static void tile_source_options_are_validated_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  static const mln_test_validation_case cases[] = {
    {"defaults", NULL, MLN_STATUS_OK, NULL},
    {"undersized", undersized_options, MLN_STATUS_INVALID_ARGUMENT,
     "size is too small"},
    {"unknown field bit", unknown_option_bit, MLN_STATUS_INVALID_ARGUMENT,
     "unknown bits"},
    {"negative min_zoom", negative_min_zoom, MLN_STATUS_INVALID_ARGUMENT,
     "min_zoom must be finite"},
    {"min_zoom above max_zoom", min_zoom_above_max_zoom,
     MLN_STATUS_INVALID_ARGUMENT, "less than or equal to max_zoom"},
    {"unknown scheme", unknown_scheme, MLN_STATUS_INVALID_ARGUMENT,
     "scheme is invalid"},
    {"zero tile_size", zero_tile_size, MLN_STATUS_INVALID_ARGUMENT,
     "tile_size must be within"},
    {"oversized tile_size", oversized_tile_size, MLN_STATUS_INVALID_ARGUMENT,
     "tile_size must be within"},
    {"null attribution bytes", null_attribution_bytes,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"latitude out of range", out_of_range_bounds, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"vector_encoding on a raster DEM source", vector_encoding_on_raster_dem,
     MLN_STATUS_INVALID_ARGUMENT, "only valid for vector sources"},
    {"unknown vector_encoding", unknown_vector_encoding,
     MLN_STATUS_INVALID_ARGUMENT, "vector_encoding is invalid"},
    {"raster_encoding on a vector source", raster_encoding_on_vector,
     MLN_STATUS_INVALID_ARGUMENT, "only valid for raster DEM sources"},
    {"unknown raster_encoding", unknown_raster_encoding,
     MLN_STATUS_INVALID_ARGUMENT, "raster_encoding is invalid"},
  };
  const tile_source_call defaults = {
    .kind = TILE_SOURCE_RASTER_DEM,
    .options = mln_style_tile_source_options_default(),
  };
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    submit_tile_source_call, &map
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads a source's URL or attribution copy, or "" when there is none.
static void read_source_text(
  mln_map map, const char* id, bool attribution, char* out, bool* found
) {
  const mln_buffer_view view = mln_test_view_of(id);
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(
    attribution
      ? mln_map_copy_style_source_attribution(
          map, view, &completion.descriptor, NULL
        )
      : mln_map_copy_style_source_url(map, view, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_style_finish_text(&completion, out, 64, found));
}

// A URL source reports the URL it was added with, or last set to, and an
// inline tile source reports the attribution its options carried. A copy of
// what a source lacks, or of a missing source, completes with no value.
static void sources_copy_their_url_and_attribution(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  serve_nothing(runtime);

  MLN_TEST_AWAIT_OK(add_tile_source(
    map, TILE_SOURCE_RASTER, "remote", true, NULL, &completion.descriptor, NULL
  ));
  mln_style_tile_source_options attributed =
    mln_style_tile_source_options_default();
  attributed.fields = MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
  char attribution[] = "Fixture tiles";
  attributed.attribution = mln_test_view_of(attribution);
  mln_test_completion add = mln_test_completion_default(0);
  MLN_TEST_OK(add_tile_source(
    map, TILE_SOURCE_VECTOR, "inline", false, &attributed, &add.descriptor, NULL
  ));
  // The command copied the attribution before it returned.
  memset(attribution, 'x', strlen(attribution));
  MLN_TEST_OK(mln_test_completion_settle(&add));
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_url(
    map, MLN_BUFFER_LITERAL("geojson"),
    MLN_BUFFER_LITERAL("fixture://first.geojson"), NULL, &completion.descriptor,
    NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_geojson_source_url(
    map, MLN_BUFFER_LITERAL("geojson"),
    MLN_BUFFER_LITERAL("fixture://second.geojson"), &completion.descriptor, NULL
  ));

  char text[64];
  bool found = false;
  read_source_text(map, "remote", false, text, &found);
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_STRING("fixture://tiles.json", text);
  read_source_text(map, "geojson", false, text, &found);
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_STRING("fixture://second.geojson", text);
  read_source_text(map, "inline", true, text, &found);
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_STRING("Fixture tiles", text);
  mln_style_source_result result;
  TEST_ASSERT_TRUE(read_source(map, "inline", &result));
  TEST_ASSERT_TRUE(result.info.has_attribution);
  TEST_ASSERT_EQUAL_size_t(
    strlen("Fixture tiles"), result.info.attribution_size
  );

  read_source_text(map, "inline", false, text, &found);
  TEST_ASSERT_FALSE(found);
  read_source_text(map, "remote", true, text, &found);
  TEST_ASSERT_FALSE(found);
  read_source_text(map, "missing", false, text, &found);
  TEST_ASSERT_FALSE(found);
  read_source_text(map, "missing", true, text, &found);
  TEST_ASSERT_FALSE(found);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Source IDs list in style order: the style's own sources as the document
// declares them, then added ones, less removed ones.
static void source_ids_list_in_style_order(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{\"first\":{\"type\":\"geojson\",\"data\":"
      "{\"type\":\"FeatureCollection\",\"features\":[]}},\"second\":{"
      "\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\","
      "\"features\":[]}}},\"layers\":[]}"
    )
  );
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("third"),
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
    NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("first"), &completion.descriptor, NULL
  ));
  const mln_test_style_list list = mln_test_style_list_source_ids(map);
  MLN_TEST_OK(list.status);
  TEST_ASSERT_EQUAL_size_t(2, list.count);
  TEST_ASSERT_EQUAL_STRING("second", list.entries[0].id);
  TEST_ASSERT_EQUAL_STRING("third", list.entries[1].id);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct tile_urls_probe {
  atomic_bool done;
  mln_status status;
  size_t value_count;
  size_t tile_url_count;
  char first[64];
  char second[64];
} tile_urls_probe;

static void copy_tile_urls(
  void* user_data, const mln_completion_result* result
) {
  tile_urls_probe* probe = user_data;
  probe->status = result->status;
  probe->value_count = result->value_count;
  if (result->value_count == 1) {
    const mln_style_source_tile_urls_result* urls = result->value;
    probe->tile_url_count = urls->tile_url_count;
    if (urls->tile_url_count > 0) {
      snprintf(
        probe->first, sizeof(probe->first), "%.*s",
        (int)urls->tile_urls[0].size, (const char*)urls->tile_urls[0].data
      );
    }
    if (urls->tile_url_count > 1) {
      snprintf(
        probe->second, sizeof(probe->second), "%.*s",
        (int)urls->tile_urls[1].size, (const char*)urls->tile_urls[1].data
      );
    }
  }
  mln_test_flag_set(&probe->done);
}

static void read_tile_urls(
  mln_runtime runtime, mln_map map, mln_buffer_view source_id,
  tile_urls_probe* probe
) {
  *probe = (tile_urls_probe){.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe->done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_tile_urls,
    .user_data = probe,
  };
  MLN_TEST_OK(
    mln_map_get_style_source_tile_urls(map, source_id, &completion, NULL)
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_TRUE(atomic_load(&probe->done));
  MLN_TEST_OK(probe->status);
}

// A found source completes with one result even when it holds no inline tile
// URLs, so a host can tell a URL-backed source from a missing one.
static void style_source_tile_urls_distinguish_empty_from_missing(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  serve_nothing(runtime);
  const mln_buffer_view tiles[] = {
    MLN_BUFFER_LITERAL("fixture://a/{z}/{x}/{y}.mvt"),
    MLN_BUFFER_LITERAL("fixture://b/{z}/{x}/{y}.mvt"),
  };
  MLN_TEST_AWAIT_OK(mln_map_add_vector_source_tiles(
    map, MLN_BUFFER_LITERAL("inline"), tiles, 2, NULL, &completion.descriptor,
    NULL
  ));
  MLN_TEST_AWAIT_OK(add_tile_source(
    map, TILE_SOURCE_VECTOR, "remote", true, NULL, &completion.descriptor, NULL
  ));

  tile_urls_probe probe;
  read_tile_urls(runtime, map, MLN_BUFFER_LITERAL("inline"), &probe);
  TEST_ASSERT_EQUAL_size_t(1, probe.value_count);
  TEST_ASSERT_EQUAL_size_t(2, probe.tile_url_count);
  TEST_ASSERT_EQUAL_STRING("fixture://a/{z}/{x}/{y}.mvt", probe.first);
  TEST_ASSERT_EQUAL_STRING("fixture://b/{z}/{x}/{y}.mvt", probe.second);

  read_tile_urls(runtime, map, MLN_BUFFER_LITERAL("remote"), &probe);
  TEST_ASSERT_EQUAL_size_t(1, probe.value_count);
  TEST_ASSERT_EQUAL_size_t(0, probe.tile_url_count);

  read_tile_urls(runtime, map, MLN_BUFFER_LITERAL("missing"), &probe);
  TEST_ASSERT_EQUAL_size_t(0, probe.value_count);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void style_source_volatility_round_trips(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view source_id = MLN_BUFFER_LITERAL("volatile-vector");
  MLN_TEST_AWAIT_OK(mln_map_add_vector_source_tiles(
    map, source_id, fixture_tiles, 1, NULL, &completion.descriptor, NULL
  ));
  mln_style_source_result result;
  TEST_ASSERT_TRUE(read_source(map, "volatile-vector", &result));
  TEST_ASSERT_FALSE(result.info.is_volatile);

  // The committed toggle publishes a snapshot generation, so volatility is an
  // ordered command rather than a synchronous write.
  mln_test_completion enable = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_set_style_source_volatile(
    map, source_id, true, &enable.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&enable));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, mln_test_completion_disposition(&enable)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(0, mln_test_completion_generation(&enable));
  mln_test_completion_destroy(&enable);
  TEST_ASSERT_TRUE(read_source(map, "volatile-vector", &result));
  TEST_ASSERT_TRUE(result.info.is_volatile);

  MLN_TEST_AWAIT_OK(mln_map_set_style_source_volatile(
    map, source_id, false, &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE(read_source(map, "volatile-vector", &result));
  TEST_ASSERT_FALSE(result.info.is_volatile);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void an_in_use_source_removal_fails_and_leaves_the_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("in-use"),
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
    NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"id\":\"user\",\"type\":\"circle\",\"source\":\"in-use\"}"
    ),
    MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
  ));

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "used by a layer",
    mln_map_remove_style_source(
      map, MLN_BUFFER_LITERAL("in-use"), &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_TRUE(source_exists(map, "in-use"));

  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("user"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("in-use"), &completion.descriptor, NULL
  ));
  TEST_ASSERT_FALSE(source_exists(map, "in-use"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void read_image_source_coordinates(
  mln_map map, const char* id, mln_lat_lng out[4]
) {
  mln_test_completion completion =
    mln_test_completion_default(4 * sizeof(mln_lat_lng));
  MLN_TEST_OK(mln_map_get_image_source_coordinates(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  TEST_ASSERT_EQUAL_size_t(4, mln_test_completion_value_count(&completion));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&completion, out, 4 * sizeof(mln_lat_lng))
  );
  mln_test_completion_destroy(&completion);
}

// Image sources hold four corner coordinates and an image, from a URL or from
// inline pixels, and the typed updates reject every other source kind.
static void image_sources_hold_corners_and_pixels(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  serve_nothing(runtime);
  const mln_lat_lng corners[4] = {
    {.latitude = 1.0, .longitude = 2.0},
    {.latitude = 1.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 2.0},
  };
  const uint8_t pixels[16] = {
    255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
  };
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);

  MLN_TEST_AWAIT_OK(mln_map_add_image_source_image(
    map, MLN_BUFFER_LITERAL("inline-image"), corners, 4, &image,
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_image_source_url(
    map, MLN_BUFFER_LITERAL("remote-image"), corners, 4,
    MLN_BUFFER_LITERAL("fixture://image.png"), &completion.descriptor, NULL
  ));
  mln_style_source_result result;
  TEST_ASSERT_TRUE(read_source(map, "inline-image", &result));
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_IMAGE, result.info.type);

  mln_lat_lng read[4];
  read_image_source_coordinates(map, "remote-image", read);
  TEST_ASSERT_EQUAL_MEMORY(corners, read, sizeof(corners));

  const mln_lat_lng moved[4] = {
    {.latitude = 5.0, .longitude = 6.0},
    {.latitude = 5.0, .longitude = 7.0},
    {.latitude = 4.0, .longitude = 7.0},
    {.latitude = 4.0, .longitude = 6.0},
  };
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_coordinates(
    map, MLN_BUFFER_LITERAL("inline-image"), moved, 4, &completion.descriptor,
    NULL
  ));
  read_image_source_coordinates(map, "inline-image", read);
  TEST_ASSERT_EQUAL_MEMORY(moved, read, sizeof(moved));
  // A URL source takes inline pixels, and an inline one takes a URL.
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_image(
    map, MLN_BUFFER_LITERAL("remote-image"), &image, &completion.descriptor,
    NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_url(
    map, MLN_BUFFER_LITERAL("inline-image"),
    MLN_BUFFER_LITERAL("fixture://image.png"), &completion.descriptor, NULL
  ));

  // Coordinates come in fours, and pixels must cover the image, before the
  // call returns.
  mln_completion discard = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_set_image_source_coordinates(
    map, MLN_BUFFER_LITERAL("inline-image"), moved, 3, &discard, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "must be 4", mln_map_add_image_source_url(
                   map, MLN_BUFFER_LITERAL("one-corner"), moved, 1,
                   MLN_BUFFER_LITERAL("fixture://image.png"),
                   &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  mln_premultiplied_rgba8_image short_image = image;
  short_image.byte_length = 15;
  MLN_TEST_INVALID(mln_map_set_image_source_image(
    map, MLN_BUFFER_LITERAL("inline-image"), &short_image, &discard, NULL
  ));

  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("geojson"),
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
    NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_coordinates(
      map, MLN_BUFFER_LITERAL("geojson"), moved, 4, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_image(
      map, MLN_BUFFER_LITERAL("geojson"), &image, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "source does not exist",
    mln_map_set_image_source_image(
      map, MLN_BUFFER_LITERAL("missing"), &image, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_image_source_coordinates(
      map, MLN_BUFFER_LITERAL("geojson"), &completion.descriptor, NULL
    )
  );
  // A missing source has no coordinates to report, which is not a failure.
  mln_test_completion missing =
    mln_test_completion_default(4 * sizeof(mln_lat_lng));
  MLN_TEST_OK(mln_map_get_image_source_coordinates(
    map, MLN_BUFFER_LITERAL("missing"), &missing.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&missing));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&missing));
  mln_test_completion_destroy(&missing);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An image source asks the provider for its URL as an image.
static void an_image_source_requests_its_url_as_an_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_provider* provider = mln_test_provider_create(NULL, 0);
  mln_test_provider_install(runtime, provider);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  const mln_lat_lng corners[4] = {
    {.latitude = 1.0, .longitude = 2.0},
    {.latitude = 1.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 2.0},
  };
  MLN_TEST_AWAIT_OK(mln_map_add_image_source_url(
    map, MLN_BUFFER_LITERAL("remote-image"), corners, 4,
    MLN_BUFFER_LITERAL("fixture://image.png"), &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE(
    mln_test_provider_wait_for_requests(provider, "fixture://image.png", 1)
  );
  const mln_test_provider_request* request =
    mln_test_provider_request_at(provider, "fixture://image.png", 0);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_IMAGE, request->kind);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

MLN_TEST_GROUP {
  RUN_TEST(tile_sources_report_their_effective_options);
  RUN_TEST(tile_source_options_are_validated_at_submission);
  RUN_TEST(sources_copy_their_url_and_attribution);
  RUN_TEST(source_ids_list_in_style_order);
  RUN_TEST(style_source_tile_urls_distinguish_empty_from_missing);
  RUN_TEST(style_source_volatility_round_trips);
  RUN_TEST(an_in_use_source_removal_fails_and_leaves_the_source);
  RUN_TEST(image_sources_hold_corners_and_pixels);
  RUN_TEST(an_image_source_requests_its_url_as_an_image);
}
