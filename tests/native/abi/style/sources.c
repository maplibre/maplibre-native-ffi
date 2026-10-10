// Style sources: tile source options and the effective values a source reports,
// its URL, attribution, and inline tile URLs, source listing, volatility,
// removal, and image sources and their corners.
//
// URL sources start loading as soon as they are added, so every case that adds
// one serves the suite's resources through a provider that fails the rest.

#include "support/resources.h"
#include "support/style.h"
#include "support/test_support.h"

// A deep copy of one source's metadata, whose views die with the callback.
// Strings longer than their buffers are truncated, which the cases never reach.
typedef struct source_copy {
  bool found;
  mln_style_source_info info;
  char id[32];
  char attribution[64];
  char url[64];
  char tile_urls[2][64];
} source_copy;

#define SOURCE_PROBE_CAPACITY 4

// Copies of the sources one get or list query delivered.
typedef struct source_probe {
  atomic_bool done;
  mln_status status;
  size_t count;
  source_copy sources[SOURCE_PROBE_CAPACITY];
} source_probe;

static void copy_text(mln_buffer_view view, char* out, size_t capacity) {
  snprintf(out, capacity, "%.*s", (int)view.size, (const char*)view.data);
}

static void copy_sources(void* user_data, const mln_completion_result* result) {
  source_probe* probe = user_data;
  probe->status = result->status;
  probe->count = result->value_count;
  const mln_style_source_info* infos = result->value;
  for (size_t index = 0;
       index < result->value_count && index < SOURCE_PROBE_CAPACITY;
       index += 1) {
    const mln_style_source_info* info = &infos[index];
    source_copy* copy = &probe->sources[index];
    copy->found = true;
    copy->info = *info;
    copy_text(info->id, copy->id, sizeof(copy->id));
    copy_text(info->attribution, copy->attribution, sizeof(copy->attribution));
    copy_text(info->url, copy->url, sizeof(copy->url));
    for (size_t url = 0; url < info->tilejson.tile_url_count && url < 2;
         url += 1) {
      copy_text(
        info->tilejson.tile_urls[url], copy->tile_urls[url],
        sizeof(copy->tile_urls[url])
      );
    }
  }
  mln_test_flag_set(&probe->done);
}

static mln_completion source_completion(source_probe* probe) {
  atomic_init(&probe->done, false);
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = copy_sources,
    .user_data = probe,
  };
}

static void finish_source_probe(const source_probe* probe) {
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&probe->done), "the source query never completed"
  );
  MLN_TEST_OK(probe->status);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(SOURCE_PROBE_CAPACITY, probe->count);
}

// Reads a source's metadata. `found` reports whether the source exists.
static source_copy read_source(mln_map map, const char* id) {
  source_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  const mln_completion completion = source_completion(&probe);
  MLN_TEST_OK(
    mln_map_get_style_source(map, mln_test_view_of(id), &completion, NULL)
  );
  finish_source_probe(&probe);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(1, probe.count);
  return probe.sources[0];
}

// Lists every source's metadata into `probe`, which starts zeroed.
static void list_sources(mln_map map, source_probe* probe) {
  const mln_completion completion = source_completion(probe);
  MLN_TEST_OK(mln_map_list_style_sources(map, &completion, NULL));
  finish_source_probe(probe);
}

static bool source_exists(mln_map map, const char* id) {
  return read_source(map, id).found;
}

static void add_source_json(mln_map map, const char* id, const char* json) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, mln_test_view_of(id), mln_test_view_of(json), &completion.descriptor,
    NULL
  ));
}

enum tile_source_kind { VECTOR, RASTER, RASTER_DEM };

static const mln_buffer_view fixture_tiles[] = {
  MLN_BUFFER_LITERAL("fixture://tiles/{z}/{x}/{y}"),
};

typedef mln_status (*add_url_source)(
  mln_map, mln_buffer_view, mln_buffer_view,
  const mln_style_tile_source_options*, const mln_completion*, mln_diagnostic*
);
typedef mln_status (*add_tiles_source)(
  mln_map, mln_buffer_view, const mln_buffer_view*, size_t,
  const mln_style_tile_source_options*, const mln_completion*, mln_diagnostic*
);

// Indexed by tile_source_kind.
static const add_url_source url_adders[] = {
  mln_map_add_vector_source_url,
  mln_map_add_raster_source_url,
  mln_map_add_raster_dem_source_url,
};
static const add_tiles_source tiles_adders[] = {
  mln_map_add_vector_source_tiles,
  mln_map_add_raster_source_tiles,
  mln_map_add_raster_dem_source_tiles,
};

static mln_status add_tile_source(
  mln_map map, enum tile_source_kind kind, const char* id, bool from_url,
  const mln_style_tile_source_options* options,
  const mln_completion* completion, mln_diagnostic* diagnostic
) {
  const mln_buffer_view source_id = mln_test_view_of(id);
  if (from_url) {
    return url_adders[kind](
      map, source_id, MLN_BUFFER_LITERAL("fixture://tiles.json"), options,
      completion, diagnostic
    );
  }
  return tiles_adders[kind](
    map, source_id, fixture_tiles, 1, options, completion, diagnostic
  );
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
  options->bounds = (mln_lat_lng_bounds){{-10.0, -20.0}, {10.0, 20.0}};
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

// One source a row adds and the metadata it must then report: what the row's
// edit sets, or the defaults for null options. Every bit in `absent` must be
// missing from fields.
typedef struct effective_options_case {
  const char* label;
  enum tile_source_kind kind;
  bool from_url;
  void (*mutate)(mln_style_tile_source_options* options);
  uint32_t type;
  uint32_t present;
  uint32_t absent;
} effective_options_case;

enum {
  TILEJSON = MLN_STYLE_SOURCE_INFO_TILEJSON | MLN_STYLE_SOURCE_INFO_TILE_SIZE,
  URL_INFO = MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_TILE_SIZE,
  VECTOR_ENCODING = MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
  RASTER_ENCODING = MLN_STYLE_SOURCE_INFO_RASTER_ENCODING,
  URL_OR_ENCODING =
    MLN_STYLE_SOURCE_INFO_URL | VECTOR_ENCODING | RASTER_ENCODING,
};

static const effective_options_case effective_cases[] = {
  {"vector tiles, null options", VECTOR, false, NULL,
   MLN_STYLE_SOURCE_TYPE_VECTOR, TILEJSON | VECTOR_ENCODING,
   MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_BOUNDS | RASTER_ENCODING},
  {"vector tiles, explicit zoom, scheme, and bounds", VECTOR, false,
   explicit_zoom_scheme_and_bounds, MLN_STYLE_SOURCE_TYPE_VECTOR,
   TILEJSON | MLN_STYLE_SOURCE_INFO_BOUNDS, 0},
  {"vector tiles, MLT", VECTOR, false, mlt_encoding,
   MLN_STYLE_SOURCE_TYPE_VECTOR, TILEJSON | VECTOR_ENCODING, 0},
  {"raster tiles, null options", RASTER, false, NULL,
   MLN_STYLE_SOURCE_TYPE_RASTER, TILEJSON, URL_OR_ENCODING},
  {"raster tiles, 256 px", RASTER, false, small_tiles,
   MLN_STYLE_SOURCE_TYPE_RASTER, TILEJSON, 0},
  {"raster DEM tiles, null options", RASTER_DEM, false, NULL,
   MLN_STYLE_SOURCE_TYPE_RASTER_DEM, TILEJSON, URL_OR_ENCODING},
  {"raster DEM tiles, Terrarium", RASTER_DEM, false, terrarium_encoding,
   MLN_STYLE_SOURCE_TYPE_RASTER_DEM, TILEJSON | RASTER_ENCODING, 0},
  {"raster DEM tiles, Mapbox", RASTER_DEM, false, mapbox_encoding,
   MLN_STYLE_SOURCE_TYPE_RASTER_DEM, TILEJSON | RASTER_ENCODING, 0},
  {"raster tiles, explicit XYZ", RASTER, false, xyz_scheme,
   MLN_STYLE_SOURCE_TYPE_RASTER, TILEJSON, 0},
  {"vector URL, null options", VECTOR, true, NULL, MLN_STYLE_SOURCE_TYPE_VECTOR,
   URL_INFO | VECTOR_ENCODING, MLN_STYLE_SOURCE_INFO_TILEJSON},
  {"vector URL, zoom range and MLT", VECTOR, true, url_zoom_and_mlt,
   MLN_STYLE_SOURCE_TYPE_VECTOR, URL_INFO | VECTOR_ENCODING,
   MLN_STYLE_SOURCE_INFO_TILEJSON},
  {"raster URL, 256 px", RASTER, true, small_tiles,
   MLN_STYLE_SOURCE_TYPE_RASTER, URL_INFO, MLN_STYLE_SOURCE_INFO_TILEJSON},
  {"raster DEM URL, null options", RASTER_DEM, true, NULL,
   MLN_STYLE_SOURCE_TYPE_RASTER_DEM, URL_INFO, MLN_STYLE_SOURCE_INFO_TILEJSON},
};

// Each masked member whose bit is absent from fields reads as zero. The table
// lists members without padding, whose bytes are all zero; tilejson has
// trailing padding, so its members are compared instead.
static void expect_absent_members_zero(
  const mln_style_source_info* info, const char* label
) {
  static const struct {
    uint32_t bit;
    size_t offset;
    size_t size;
  } members[] = {
#define MEMBER(bit, name)                      \
  {bit, offsetof(mln_style_source_info, name), \
   sizeof(((mln_style_source_info*)0)->name)}
    MEMBER(MLN_STYLE_SOURCE_INFO_ATTRIBUTION, attribution),
    MEMBER(MLN_STYLE_SOURCE_INFO_URL, url),
    MEMBER(MLN_STYLE_SOURCE_INFO_BOUNDS, bounds),
    MEMBER(MLN_STYLE_SOURCE_INFO_TILE_SIZE, tile_size),
    MEMBER(MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING, vector_encoding),
    MEMBER(MLN_STYLE_SOURCE_INFO_RASTER_ENCODING, raster_encoding),
#undef MEMBER
  };
  for (size_t index = 0; index < sizeof(members) / sizeof(members[0]);
       index += 1) {
    if ((info->fields & members[index].bit) != 0) continue;
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_all_zero(
        (const unsigned char*)info + members[index].offset, members[index].size
      ),
      label
    );
  }
  if ((info->fields & MLN_STYLE_SOURCE_INFO_TILEJSON) == 0) {
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_tile_info_is_zero(&info->tilejson), label
    );
  }
}

// A source reports what its options set and the documented default for what
// they omit, including null options, and reports an encoding only for the
// source kinds that have one.
static void tile_sources_report_their_effective_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);

  for (size_t index = 0;
       index < sizeof(effective_cases) / sizeof(effective_cases[0]);
       index += 1) {
    const effective_options_case* row = &effective_cases[index];
    const char* label = row->label;
    char id[16];
    snprintf(id, sizeof(id), "source-%zu", index);
    mln_style_tile_source_options expected =
      mln_style_tile_source_options_default();
    if (row->mutate != NULL) {
      row->mutate(&expected);
    }
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_OK_MESSAGE(
      add_tile_source(
        map, row->kind, id, row->from_url,
        row->mutate == NULL ? NULL : &expected, &completion.descriptor, NULL
      ),
      label
    );
    MLN_TEST_OK_MESSAGE(mln_test_completion_settle(&completion), label);

    const source_copy probe = read_source(map, id);
    TEST_ASSERT_TRUE_MESSAGE(probe.found, label);
    const mln_style_source_info* info = &probe.info;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(row->type, info->type, label);
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(
      row->present, info->fields & row->present, label
    );
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(0, info->fields & row->absent, label);
    expect_absent_members_zero(info, label);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      expected.tile_size, info->tile_size, label
    );
    if ((info->fields & MLN_STYLE_SOURCE_INFO_TILEJSON) != 0) {
      TEST_ASSERT_EQUAL_size_t_MESSAGE(1, info->tilejson.tile_url_count, label);
      TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
        expected.min_zoom, info->tilejson.min_zoom, label
      );
      TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
        expected.max_zoom, info->tilejson.max_zoom, label
      );
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        expected.scheme, info->tilejson.scheme, label
      );
    }
    if ((info->fields & MLN_STYLE_SOURCE_INFO_BOUNDS) != 0) {
      TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
        &expected.bounds, &info->bounds, sizeof(info->bounds), label
      );
    }
    if ((info->fields & VECTOR_ENCODING) != 0) {
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        expected.vector_encoding, info->vector_encoding, label
      );
    }
    if ((info->fields & RASTER_ENCODING) != 0) {
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        expected.raster_encoding, info->raster_encoding, label
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

// Sets `field` on a validation row's options, and returns them for the row to
// set the field's value.
static mln_style_tile_source_options* edit(void* descriptor, uint32_t field) {
  tile_source_call* call = descriptor;
  call->options.fields |= field;
  return &call->options;
}

static void undersized_options(void* descriptor) {
  edit(descriptor, 0)->size -= 1;
}
static void unknown_option_bit(void* descriptor) {
  edit(descriptor, UINT32_C(1) << 31);
}
static void negative_min_zoom(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM)->min_zoom = -1.0;
}
static void min_zoom_above_max_zoom(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM)->min_zoom = 10.0;
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM)->max_zoom = 4.0;
}
static void unknown_scheme(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_SCHEME)->scheme = 7;
}
static void zero_tile_size(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE)->tile_size = 0;
}
static void oversized_tile_size(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE)->tile_size = 65536;
}
static void null_attribution_bytes(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION)->attribution =
    (mln_buffer_view){.data = NULL, .size = 3};
}
static void out_of_range_bounds(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS)->bounds =
    (mln_lat_lng_bounds){{-100.0, 0.0}, {10.0, 10.0}};
}
static void vector_encoding_on_raster_dem(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING)
    ->vector_encoding = MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
}
static void unknown_vector_encoding(void* descriptor) {
  ((tile_source_call*)descriptor)->kind = VECTOR;
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING)
    ->vector_encoding = 9;
}
static void raster_encoding_on_vector(void* descriptor) {
  ((tile_source_call*)descriptor)->kind = VECTOR;
  terrarium_encoding(edit(descriptor, 0));
}
static void unknown_raster_encoding(void* descriptor) {
  edit(descriptor, MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING)
    ->raster_encoding = 9;
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
    .kind = RASTER_DEM,
    .options = mln_style_tile_source_options_default(),
  };
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    submit_tile_source_call, &map
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A URL source reports the URL it was added with, or last set to. An inline
// tile source reports the attribution its options carried, or an empty one when
// they carried none, and its tile URLs. A source reports only the members it
// has. A missing source completes with no value.
static void a_source_reports_its_url_attribution_and_tilejson(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);

  MLN_TEST_AWAIT_OK(add_tile_source(
    map, RASTER, "remote", true, NULL, &completion.descriptor, NULL
  ));
  mln_style_tile_source_options attributed =
    mln_style_tile_source_options_default();
  attributed.fields = MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
  char attribution[] = "Fixture tiles";
  attributed.attribution = mln_test_view_of(attribution);
  const mln_buffer_view tiles[] = {
    MLN_BUFFER_LITERAL("fixture://a/{z}/{x}/{y}.mvt"),
    MLN_BUFFER_LITERAL("fixture://b/{z}/{x}/{y}.mvt"),
  };
  mln_test_completion add = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_add_vector_source_tiles(
    map, MLN_BUFFER_LITERAL("inline"), tiles, 2, &attributed, &add.descriptor,
    NULL
  ));
  // The command copied the attribution before it returned.
  memset(attribution, 'x', strlen(attribution));
  MLN_TEST_OK(mln_test_completion_settle(&add));
  MLN_TEST_AWAIT_OK(mln_map_add_vector_source_tiles(
    map, MLN_BUFFER_LITERAL("unattributed"), tiles, 1, NULL,
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_url(
    map, MLN_BUFFER_LITERAL("geojson"),
    MLN_BUFFER_LITERAL("fixture://first.geojson"), NULL, &completion.descriptor,
    NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_geojson_source_url(
    map, MLN_BUFFER_LITERAL("geojson"),
    MLN_BUFFER_LITERAL("fixture://second.geojson"), &completion.descriptor, NULL
  ));

  source_copy probe = read_source(map, "remote");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_STYLE_SOURCE_INFO_URL,
    probe.info.fields &
      (MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_ATTRIBUTION |
       MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("fixture://tiles.json", probe.url);

  probe = read_source(map, "geojson");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_STRING("fixture://second.geojson", probe.url);

  probe = read_source(map, "inline");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON,
    probe.info.fields &
      (MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_ATTRIBUTION |
       MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("Fixture tiles", probe.attribution);
  TEST_ASSERT_EQUAL_size_t(2, probe.info.tilejson.tile_url_count);
  TEST_ASSERT_EQUAL_STRING("fixture://a/{z}/{x}/{y}.mvt", probe.tile_urls[0]);
  TEST_ASSERT_EQUAL_STRING("fixture://b/{z}/{x}/{y}.mvt", probe.tile_urls[1]);

  probe = read_source(map, "unattributed");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_TRUE(probe.info.fields & MLN_STYLE_SOURCE_INFO_ATTRIBUTION);
  TEST_ASSERT_EQUAL_STRING("", probe.attribution);

  TEST_ASSERT_FALSE(read_source(map, "missing").found);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void assert_same_source(
  const source_copy* expected, const source_copy* actual
) {
  const mln_style_source_info* want = &expected->info;
  const mln_style_source_info* got = &actual->info;
  TEST_ASSERT_EQUAL_STRING(expected->id, actual->id);
  TEST_ASSERT_EQUAL_UINT32(want->type, got->type);
  TEST_ASSERT_EQUAL_HEX32(want->fields, got->fields);
  TEST_ASSERT_EQUAL_INT(want->is_volatile, got->is_volatile);
  TEST_ASSERT_EQUAL_STRING(expected->attribution, actual->attribution);
  TEST_ASSERT_EQUAL_STRING(expected->url, actual->url);
  TEST_ASSERT_EQUAL_size_t(
    want->tilejson.tile_url_count, got->tilejson.tile_url_count
  );
  TEST_ASSERT_EQUAL_STRING(expected->tile_urls[0], actual->tile_urls[0]);
  TEST_ASSERT_EQUAL_DOUBLE(want->tilejson.min_zoom, got->tilejson.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(want->tilejson.max_zoom, got->tilejson.max_zoom);
  TEST_ASSERT_EQUAL_UINT32(want->tilejson.scheme, got->tilejson.scheme);
  TEST_ASSERT_EQUAL_UINT32(want->tile_size, got->tile_size);
  TEST_ASSERT_EQUAL_UINT32(want->vector_encoding, got->vector_encoding);
}

// Sources list in style order with the same metadata a get reports: the
// style's own sources as the document declares them, then added ones, less
// removed ones.
static void sources_list_in_style_order_with_their_info(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);
  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{\"first\":" MLN_TEST_EMPTY_GEOJSON_SOURCE
      ",\"second\":" MLN_TEST_EMPTY_GEOJSON_SOURCE "},\"layers\":[]}"
    )
  );
  mln_style_tile_source_options attributed =
    mln_style_tile_source_options_default();
  attributed.fields = MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
  attributed.attribution = MLN_BUFFER_LITERAL("Third tiles");
  MLN_TEST_AWAIT_OK(mln_map_add_vector_source_tiles(
    map, MLN_BUFFER_LITERAL("third"), fixture_tiles, 1, &attributed,
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("first"), &completion.descriptor, NULL
  ));

  source_probe list = {.status = MLN_STATUS_INVALID_STATE};
  list_sources(map, &list);
  TEST_ASSERT_EQUAL_size_t(2, list.count);
  const source_copy* second = &list.sources[0];
  const source_copy* third = &list.sources[1];
  TEST_ASSERT_EQUAL_STRING("second", second->id);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_GEOJSON, second->info.type);
  TEST_ASSERT_EQUAL_HEX32(
    0, second->info.fields &
         (MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("third", third->id);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_VECTOR, third->info.type);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON,
    third->info.fields &
      (MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("Third tiles", third->attribution);
  TEST_ASSERT_EQUAL_size_t(1, third->info.tilejson.tile_url_count);
  TEST_ASSERT_EQUAL_STRING("fixture://tiles/{z}/{x}/{y}", third->tile_urls[0]);

  const source_copy got = read_source(map, "third");
  TEST_ASSERT_TRUE(got.found);
  assert_same_source(&got, third);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static bool source_is_volatile(mln_map map) {
  const source_copy probe = read_source(map, "volatile-vector");
  TEST_ASSERT_TRUE(probe.found);
  return probe.info.is_volatile;
}

static void style_source_volatility_round_trips(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view source_id = MLN_BUFFER_LITERAL("volatile-vector");
  MLN_TEST_AWAIT_OK(mln_map_add_vector_source_tiles(
    map, source_id, fixture_tiles, 1, NULL, &completion.descriptor, NULL
  ));
  TEST_ASSERT_FALSE(source_is_volatile(map));

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
  TEST_ASSERT_TRUE(source_is_volatile(map));

  MLN_TEST_AWAIT_OK(mln_map_set_style_source_volatile(
    map, source_id, false, &completion.descriptor, NULL
  ));
  TEST_ASSERT_FALSE(source_is_volatile(map));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void an_in_use_source_removal_fails_and_leaves_the_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view in_use = MLN_BUFFER_LITERAL("in-use");
  add_source_json(map, "in-use", MLN_TEST_EMPTY_GEOJSON_SOURCE);
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"id\":\"user\",\"type\":\"circle\",\"source\":\"in-use\"}"
    ),
    MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
  ));

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "used by a layer",
    mln_map_remove_style_source(map, in_use, &completion.descriptor, NULL)
  );
  TEST_ASSERT_TRUE(source_exists(map, "in-use"));

  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("user"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(
    mln_map_remove_style_source(map, in_use, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(source_exists(map, "in-use"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads an image source's four corners into `out`.
static void read_corners(mln_map map, const char* id, mln_lat_lng out[4]) {
  mln_test_completion completion =
    mln_test_completion_default(4 * sizeof(mln_lat_lng));
  MLN_TEST_OK(mln_map_get_image_source_coordinates(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  TEST_ASSERT_EQUAL_size_t(4, mln_test_completion_value_count(&completion));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&completion, out, 4 * sizeof(*out))
  );
  mln_test_completion_destroy(&completion);
}

static const mln_lat_lng corners[4] = {{1, 2}, {1, 3}, {0, 3}, {0, 2}};

// Image sources hold four corner coordinates and an image, from a URL or from
// inline pixels, and the typed updates reject every other source kind.
static void image_sources_hold_corners_and_pixels(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);
  const uint8_t pixels[16] = {
    255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
  };
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);
  const mln_buffer_view inline_image = MLN_BUFFER_LITERAL("inline-image");
  const mln_buffer_view remote_image = MLN_BUFFER_LITERAL("remote-image");
  const mln_buffer_view geojson = MLN_BUFFER_LITERAL("geojson");
  const mln_buffer_view png = MLN_BUFFER_LITERAL("fixture://image.png");

  MLN_TEST_AWAIT_OK(mln_map_add_image_source_image(
    map, inline_image, corners, 4, &image, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_image_source_url(
    map, remote_image, corners, 4, png, &completion.descriptor, NULL
  ));
  const source_copy probe = read_source(map, "inline-image");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_IMAGE, probe.info.type);

  mln_lat_lng read[4];
  read_corners(map, "remote-image", read);
  TEST_ASSERT_EQUAL_MEMORY(corners, read, sizeof(corners));

  const mln_lat_lng moved[4] = {{5, 6}, {5, 7}, {4, 7}, {4, 6}};
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_coordinates(
    map, inline_image, moved, 4, &completion.descriptor, NULL
  ));
  read_corners(map, "inline-image", read);
  TEST_ASSERT_EQUAL_MEMORY(moved, read, sizeof(moved));
  // A URL source takes inline pixels, and an inline one takes a URL.
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_image(
    map, remote_image, &image, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_url(
    map, inline_image, png, &completion.descriptor, NULL
  ));

  // Coordinates come in fours, and pixels must cover the image, before the
  // call returns.
  mln_completion discard = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_set_image_source_coordinates(
    map, inline_image, moved, 3, &discard, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "must be 4", mln_map_add_image_source_url(
                   map, MLN_BUFFER_LITERAL("one-corner"), moved, 1, png,
                   &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  mln_premultiplied_rgba8_image short_image = image;
  short_image.byte_length = 15;
  MLN_TEST_INVALID(mln_map_set_image_source_image(
    map, inline_image, &short_image, &discard, NULL
  ));

  add_source_json(map, "geojson", MLN_TEST_EMPTY_GEOJSON_SOURCE);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_coordinates(
      map, geojson, moved, 4, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_image(
      map, geojson, &image, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "source does not exist",
    mln_map_set_image_source_image(
      map, MLN_BUFFER_LITERAL("missing"), &image, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_image_source_coordinates(
                                   map, geojson, &completion.descriptor, NULL
                                 )
  );
  // Corners are a member of the source, so reading a missing one fails.
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NOT_FOUND,
    mln_map_get_image_source_coordinates(
      map, MLN_BUFFER_LITERAL("missing"), &completion.descriptor, NULL
    )
  );

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
  RUN_TEST(a_source_reports_its_url_attribution_and_tilejson);
  RUN_TEST(sources_list_in_style_order_with_their_info);
  RUN_TEST(style_source_volatility_round_trips);
  RUN_TEST(an_in_use_source_removal_fails_and_leaves_the_source);
  RUN_TEST(image_sources_hold_corners_and_pixels);
  RUN_TEST(an_image_source_requests_its_url_as_an_image);
}
