// Custom geometry and custom vector sources.
//
// The two kinds share their contracts, so every shared case runs once per kind.
// The release callback is the only report that the host's state is no longer
// referenced, and it runs whatever the map's event mask.

#include "support/delivery.h"
#include "support/style.h"
#include "support/test_support.h"

// Written from MapLibre threads and read from the test thread, so atomic. Each
// callback pulses, so the waits below wake on it. A case sets map and
// source_id before it adds the source, and every call that names another map
// or source counts in misnamed.
typedef struct custom_probe {
  mln_map map;
  const char* source_id;
  atomic_size_t release_count;
  atomic_size_t fetch_count;
  atomic_size_t root_fetches;
  atomic_size_t root_cancels;
  // The tiles at zoom 1, indexed by y * 2 + x.
  atomic_size_t zoom_one_fetches[4];
  atomic_size_t zoom_one_cancels[4];
  atomic_size_t misnamed;
} custom_probe;

static bool view_equals(mln_buffer_view view, const char* text) {
  return view.size == strlen(text) && memcmp(view.data, text, view.size) == 0;
}

static void probe_check_names(
  custom_probe* probe, mln_map map, mln_buffer_view source_id
) {
  if (map != probe->map || !view_equals(source_id, probe->source_id)) {
    atomic_fetch_add(&probe->misnamed, 1);
  }
}

static void probe_fetch_tile(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  custom_probe* probe = user_data;
  probe_check_names(probe, map, source_id);
  atomic_fetch_add(&probe->fetch_count, 1);
  if (tile_id.z == 0) {
    atomic_fetch_add(&probe->root_fetches, 1);
  } else if (tile_id.z == 1) {
    atomic_fetch_add(&probe->zoom_one_fetches[tile_id.y * 2 + tile_id.x], 1);
  }
  mln_test_pulse();
}

static void probe_cancel_tile(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  custom_probe* probe = user_data;
  probe_check_names(probe, map, source_id);
  if (tile_id.z == 0) {
    atomic_fetch_add(&probe->root_cancels, 1);
  } else if (tile_id.z == 1) {
    atomic_fetch_add(&probe->zoom_one_cancels[tile_id.y * 2 + tile_id.x], 1);
  }
  mln_test_pulse();
}

static void probe_release(void* user_data) {
  atomic_fetch_add(&((custom_probe*)user_data)->release_count, 1);
  mln_test_pulse();
}

enum add_variant {
  ADD_VALID,
  // Rejected before the call returns, so nothing references the probe.
  ADD_NEGATIVE_MIN_ZOOM,
  ADD_WITHOUT_FETCH,
  ADD_WITHOUT_ID,
};

static const mln_canonical_tile_id root_tile = {.z = 0, .x = 0, .y = 0};

// Northeast of the origin, so of the tiles at zoom 1 it intersects only 1/1/0.
static const mln_lat_lng_bounds northeast_region = {{10.0, 10.0}, {20.0, 20.0}};
static const size_t northeast_tile = 1;

static mln_status add_geometry_source(
  mln_map map, custom_probe* probe, enum add_variant variant,
  const mln_completion* completion
) {
  mln_custom_geometry_source_options options = {
    .size = sizeof(mln_custom_geometry_source_options)
  };
  options.fetch_tile = variant == ADD_WITHOUT_FETCH ? NULL : probe_fetch_tile;
  options.cancel_tile = probe_cancel_tile;
  options.user_data = probe;
  options.release_user_data = probe_release;
  if (variant == ADD_NEGATIVE_MIN_ZOOM) {
    options.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
    options.min_zoom = -1;
  }
  return mln_map_add_custom_geometry_source(
    map,
    variant == ADD_WITHOUT_ID ? MLN_BUFFER_LITERAL("")
                              : MLN_BUFFER_LITERAL("custom-geometry"),
    &options, completion, NULL
  );
}

static mln_status add_vector_source(
  mln_map map, custom_probe* probe, enum add_variant variant,
  const mln_completion* completion
) {
  mln_custom_vector_source_options options = {
    .size = sizeof(mln_custom_vector_source_options)
  };
  options.fetch_tile = variant == ADD_WITHOUT_FETCH ? NULL : probe_fetch_tile;
  options.cancel_tile = probe_cancel_tile;
  options.user_data = probe;
  options.release_user_data = probe_release;
  if (variant == ADD_NEGATIVE_MIN_ZOOM) {
    options.fields = MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM;
    options.min_zoom = -1;
  }
  return mln_map_add_custom_vector_source(
    map,
    variant == ADD_WITHOUT_ID ? MLN_BUFFER_LITERAL("")
                              : MLN_BUFFER_LITERAL("custom-vector"),
    &options, completion, NULL
  );
}

static mln_status deliver_geometry_point(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_custom_geometry_source_tile_data(
    map, MLN_BUFFER_LITERAL("custom-geometry"), root_tile,
    MLN_BUFFER_LITERAL(
      "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
      "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"
      "\"properties\":{\"kind\":\"delivered\"}}]}"
    ),
    completion, NULL
  );
}

// One MVT tile holding layer "points" with one point feature, id 1, at the
// tile's center: a layer (version 2, extent 4096) whose feature is a MoveTo
// of (2048, 2048), zigzag-encoded.
static const uint8_t mvt_point_tile[] = {
  0x1a, 0x1a, 0x78, 0x02, 0x0a, 0x06, 'p',  'o',  'i',  'n',
  't',  's',  0x12, 0x0b, 0x08, 0x01, 0x18, 0x01, 0x22, 0x05,
  0x09, 0x80, 0x20, 0x80, 0x20, 0x28, 0x80, 0x20,
};

static mln_status deliver_mvt_point(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_custom_vector_source_tile_data(
    map, MLN_BUFFER_LITERAL("custom-vector"), root_tile,
    MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
    (mln_buffer_view){.data = mvt_point_tile, .size = sizeof(mvt_point_tile)},
    completion, NULL
  );
}

static mln_status fail_geometry_tile(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_custom_geometry_source_tile_error(
    map, MLN_BUFFER_LITERAL("custom-geometry"), root_tile,
    MLN_BUFFER_LITERAL("provider failed"), completion, NULL
  );
}

static mln_status invalidate_geometry_tile(
  mln_map map, const mln_completion* completion
) {
  return mln_map_invalidate_custom_geometry_source_tile(
    map, MLN_BUFFER_LITERAL("custom-geometry"), root_tile, completion, NULL
  );
}

static mln_status invalidate_geometry_region(
  mln_map map, const mln_completion* completion
) {
  return mln_map_invalidate_custom_geometry_source_region(
    map, MLN_BUFFER_LITERAL("custom-geometry"), northeast_region, completion,
    NULL
  );
}

static mln_status fail_vector_tile(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_custom_vector_source_tile_error(
    map, MLN_BUFFER_LITERAL("custom-vector"), root_tile,
    MLN_BUFFER_LITERAL("provider failed"), completion, NULL
  );
}

static mln_status invalidate_vector_tile(
  mln_map map, const mln_completion* completion
) {
  return mln_map_invalidate_custom_vector_source_tile(
    map, MLN_BUFFER_LITERAL("custom-vector"), root_tile, completion, NULL
  );
}

static mln_status invalidate_vector_region(
  mln_map map, const mln_completion* completion
) {
  return mln_map_invalidate_custom_vector_source_region(
    map, MLN_BUFFER_LITERAL("custom-vector"), northeast_region, completion, NULL
  );
}

typedef struct custom_kind {
  const char* name;
  const char* source_id;
  // The source layer a feature query names, or null when there is none.
  const char* source_layer;
  // A layer that draws the source, so its tiles become required.
  const char* layer_json;
  mln_status (*add)(
    mln_map map, custom_probe* probe, enum add_variant variant,
    const mln_completion* completion
  );
  mln_status (*deliver_point)(mln_map map, const mln_completion* completion);
  // Fails the root tile with the message "provider failed".
  mln_status (*fail_root)(mln_map map, const mln_completion* completion);
  mln_status (*invalidate_root)(mln_map map, const mln_completion* completion);
  // Invalidates northeast_region.
  mln_status (*invalidate_region)(
    mln_map map, const mln_completion* completion
  );
} custom_kind;

static const custom_kind kinds[] = {
  {
    .name = "custom geometry",
    .source_id = "custom-geometry",
    .source_layer = NULL,
    .layer_json = "{\"id\":\"custom-dots\",\"type\":\"circle\",\"source\":"
                  "\"custom-geometry\"}",
    .add = add_geometry_source,
    .deliver_point = deliver_geometry_point,
    .fail_root = fail_geometry_tile,
    .invalidate_root = invalidate_geometry_tile,
    .invalidate_region = invalidate_geometry_region,
  },
  {
    .name = "custom vector",
    .source_id = "custom-vector",
    .source_layer = "points",
    .layer_json = "{\"id\":\"custom-dots\",\"type\":\"circle\",\"source\":"
                  "\"custom-vector\",\"source-layer\":\"points\"}",
    .add = add_vector_source,
    .deliver_point = deliver_mvt_point,
    .fail_root = fail_vector_tile,
    .invalidate_root = invalidate_vector_tile,
    .invalidate_region = invalidate_vector_region,
  },
};

#define FOR_EACH_KIND(kind)             \
  for (const custom_kind* kind = kinds; \
       kind < kinds + sizeof(kinds) / sizeof(kinds[0]); kind += 1)

static void init_probe(custom_probe* probe) {
  probe->map = MLN_HANDLE_NULL;
  probe->source_id = "";
  atomic_init(&probe->release_count, 0);
  atomic_init(&probe->fetch_count, 0);
  atomic_init(&probe->root_fetches, 0);
  atomic_init(&probe->root_cancels, 0);
  for (size_t index = 0; index < 4; index += 1) {
    atomic_init(&probe->zoom_one_fetches[index], 0);
    atomic_init(&probe->zoom_one_cancels[index], 0);
  }
  atomic_init(&probe->misnamed, 0);
}

static mln_status submit_and_settle(
  const custom_kind* kind, mln_map map,
  mln_status (*submit)(mln_map, const mln_completion*)
) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(submit(map, &completion.descriptor), kind->name);
  return mln_test_completion_settle(&completion);
}

static void add_source(
  const custom_kind* kind, mln_map map, custom_probe* probe
) {
  probe->map = map;
  probe->source_id = kind->source_id;
  mln_test_completion add = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    kind->add(map, probe, ADD_VALID, &add.descriptor), kind->name
  );
  MLN_TEST_OK_MESSAGE(mln_test_completion_settle(&add), kind->name);
}

static void draw_source(const custom_kind* kind, mln_map map) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_buffer_view_from_cstr(kind->layer_json), MLN_BUFFER_LITERAL(""),
    &completion.descriptor, NULL
  ));
}

static mln_map create_map_without_style_events(mln_runtime runtime) {
  mln_map_options options = mln_test_map_options();
  options.initial_extent.width = 256;
  options.initial_extent.height = 256;
  options.fields |= MLN_MAP_OPTION_EVENT_MASK;
  options.event_mask = MLN_RUNTIME_EVENT_FLAG_ALL &
                       ~(uint64_t)MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED;
  return mln_test_create_map_with_options(runtime, &options);
}

static void a_style_replacement_releases_a_dropped_source_unsubscribed(void) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = create_map_without_style_events(runtime);
    custom_probe probe;
    init_probe(&probe);

    mln_test_load_style_and_wait(map, mln_test_background_style_json);
    add_source(kind, map, &probe);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      0, atomic_load(&probe.release_count), kind->name
    );

    mln_test_load_style_and_wait(map, mln_test_empty_style_json);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );
    TEST_ASSERT_EQUAL_size_t(
      0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
    );

    mln_test_destroy_map(map);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );
    mln_test_destroy_runtime(runtime);
  }
}

static void an_explicit_removal_releases_once(void) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    init_probe(&probe);

    mln_test_load_style_and_wait(map, mln_test_background_style_json);
    add_source(kind, map, &probe);

    mln_test_completion removal = mln_test_completion_default(0);
    MLN_TEST_OK(mln_map_remove_style_source(
      map, mln_buffer_view_from_cstr(kind->source_id), &removal.descriptor, NULL
    ));
    MLN_TEST_OK(mln_test_completion_finish(&removal));
    mln_test_completion_destroy(&removal);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );

    // A style load after the removal has nothing left to reconcile.
    mln_test_load_style_and_wait(map, mln_test_empty_style_json);
    mln_test_destroy_map(map);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );
    mln_test_destroy_runtime(runtime);
  }
}

static void accepted_adds_release_their_callback_state(void) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    init_probe(&probe);
    mln_test_load_style_and_wait(map, mln_test_background_style_json);

    static const enum add_variant rejected_variants[] = {
      ADD_NEGATIVE_MIN_ZOOM,
      ADD_WITHOUT_FETCH,
      ADD_WITHOUT_ID,
    };
    for (size_t index = 0;
         index < sizeof(rejected_variants) / sizeof(rejected_variants[0]);
         index += 1) {
      mln_test_completion rejected = mln_test_completion_default(0);
      TEST_ASSERT_EQUAL_INT_MESSAGE(
        MLN_STATUS_INVALID_ARGUMENT,
        kind->add(map, &probe, rejected_variants[index], &rejected.descriptor),
        kind->name
      );
      mln_test_completion_reject(&rejected);
      mln_test_completion_destroy(&rejected);
    }

    add_source(kind, map, &probe);
    // The duplicate command is accepted, then fails application because the
    // ID already exists.
    mln_test_completion duplicate = mln_test_completion_default(0);
    MLN_TEST_OK(kind->add(map, &probe, ADD_VALID, &duplicate.descriptor));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_ALREADY_EXISTS, mln_test_completion_settle(&duplicate),
      kind->name
    );
    MLN_TEST_OK(mln_test_map_fence(map));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );

    MLN_TEST_OK(mln_test_map_release(map));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      2, atomic_load(&probe.release_count), kind->name
    );
    mln_test_destroy_runtime(runtime);
  }
}

typedef struct probe_target {
  const custom_probe* probe;
  size_t root_fetches;
  size_t root_cancels;
} probe_target;

static bool probe_reached(void* context) {
  const probe_target* target = context;
  return atomic_load(&target->probe->root_fetches) >= target->root_fetches &&
         atomic_load(&target->probe->root_cancels) >= target->root_cancels;
}

typedef struct feature_target {
  const mln_test_render_fixture* fixture;
  const custom_kind* kind;
} feature_target;

static bool delivered_feature_found(void* context) {
  const feature_target* target = context;
  const mln_test_feature_list list = mln_test_style_query_source(
    target->fixture, target->kind->source_id, target->kind->source_layer
  );
  return list.status == MLN_STATUS_OK && list.count == 1;
}

// Renders until the root tile is fetched.
static void start_rendering(
  const custom_kind* kind, mln_runtime runtime, mln_map map,
  custom_probe* probe, mln_test_render_fixture* fixture
) {
  init_probe(probe);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  add_source(kind, map, probe);
  draw_source(kind, map);
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_offscreen(map, fixture), kind->name
  );
  probe_target fetched = {.probe = probe, .root_fetches = 1};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_style_render_until(
      fixture, probe_reached, &fetched, "the root tile's fetch"
    ),
    kind->name
  );
}

static void a_delivered_tile_becomes_features_and_invalidation_refetches_it(
  void
) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    mln_test_render_fixture fixture = {0};
    start_rendering(kind, runtime, map, &probe, &fixture);

    const mln_test_feature_list before = mln_test_style_query_source(
      &fixture, kind->source_id, kind->source_layer
    );
    MLN_TEST_OK_MESSAGE(before.status, kind->name);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(0, before.count, kind->name);

    MLN_TEST_OK_MESSAGE(
      submit_and_settle(kind, map, kind->deliver_point), kind->name
    );
    feature_target features = {.fixture = &fixture, .kind = kind};
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_style_render_until(
        &fixture, delivered_feature_found, &features, "the delivered feature"
      ),
      kind->name
    );

    const size_t fetches = atomic_load(&probe.root_fetches);
    const size_t cancels = atomic_load(&probe.root_cancels);
    MLN_TEST_OK_MESSAGE(
      submit_and_settle(kind, map, kind->invalidate_root), kind->name
    );
    probe_target refetched = {
      .probe = &probe,
      .root_fetches = fetches + 1,
      .root_cancels = cancels + 1,
    };
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_style_render_until(
        &fixture, probe_reached, &refetched, "the invalidated tile's fetch"
      ),
      kind->name
    );
    // The tile loader runs the refetch after the invalidation, so the one
    // cancel the invalidation made has already arrived.
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      cancels + 1, atomic_load(&probe.root_cancels), kind->name
    );

    // Every fetch and cancel named this map and the source's ID.
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      0, atomic_load(&probe.misnamed), kind->name
    );

    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
  }
}

typedef struct zoom_one_target {
  const custom_probe* probe;
  size_t fetches[4];
} zoom_one_target;

static bool zoom_one_fetches_reached(void* context) {
  const zoom_one_target* target = context;
  for (size_t index = 0; index < 4; index += 1) {
    if (
      atomic_load(&target->probe->zoom_one_fetches[index]) <
      target->fetches[index]
    ) {
      return false;
    }
  }
  return true;
}

// Centers the map on the origin at zoom 1, where it shows all four tiles of
// that zoom.
static void center_at_zoom_one(mln_map map) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 1.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

static void a_region_invalidation_refetches_the_tiles_it_intersects(void) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    init_probe(&probe);
    mln_test_load_style_and_wait(map, mln_test_background_style_json);
    add_source(kind, map, &probe);
    draw_source(kind, map);
    center_at_zoom_one(map);
    mln_test_render_fixture fixture = {0};
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_render_fixture_create_offscreen(map, &fixture), kind->name
    );
    zoom_one_target target = {.probe = &probe, .fetches = {1, 1, 1, 1}};
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_style_render_until(
        &fixture, zoom_one_fetches_reached, &target, "the zoom 1 tiles' fetches"
      ),
      kind->name
    );

    size_t cancels[4];
    for (size_t index = 0; index < 4; index += 1) {
      target.fetches[index] = atomic_load(&probe.zoom_one_fetches[index]);
      cancels[index] = atomic_load(&probe.zoom_one_cancels[index]);
    }
    MLN_TEST_OK_MESSAGE(
      submit_and_settle(kind, map, kind->invalidate_region), kind->name
    );
    target.fetches[northeast_tile] += 1;
    cancels[northeast_tile] += 1;
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_style_render_until(
        &fixture, zoom_one_fetches_reached, &target,
        "the invalidated tile's fetch"
      ),
      kind->name
    );
    // The tile loader cancels every tile the invalidation reaches before it
    // runs the refetch, so only the tile in the region was invalidated.
    for (size_t index = 0; index < 4; index += 1) {
      TEST_ASSERT_EQUAL_size_t_MESSAGE(
        target.fetches[index], atomic_load(&probe.zoom_one_fetches[index]),
        kind->name
      );
      TEST_ASSERT_EQUAL_size_t_MESSAGE(
        cancels[index], atomic_load(&probe.zoom_one_cancels[index]), kind->name
      );
    }
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      0, atomic_load(&probe.misnamed), kind->name
    );

    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
  }
}

// Answers each fetch through the map and source ID that the call names, with a
// point whose "source" property is that ID. The null completion discards the
// command's outcome.
static void answer_with_source_id(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  (void)user_data;
  char data[256];
  snprintf(
    data, sizeof(data),
    "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
    "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"
    "\"properties\":{\"source\":\"%.*s\"}}]}",
    (int)source_id.size, (const char*)source_id.data
  );
  (void)mln_map_set_custom_geometry_source_tile_data(
    map, source_id, tile_id, mln_buffer_view_from_cstr(data), NULL, NULL
  );
}

typedef struct shared_target {
  const mln_test_render_fixture* fixture;
  const char* source_ids[2];
} shared_target;

static bool source_answered(
  const mln_test_render_fixture* fixture, const char* source_id
) {
  const mln_test_feature_list list =
    mln_test_style_query_source(fixture, source_id, NULL);
  char property[64];
  snprintf(property, sizeof(property), "\"source\":\"%s\"", source_id);
  return list.status == MLN_STATUS_OK && list.count == 1 &&
         strstr(list.features[0].feature, property) != NULL;
}

static bool both_sources_answered(void* context) {
  const shared_target* target = context;
  return source_answered(target->fixture, target->source_ids[0]) &&
         source_answered(target->fixture, target->source_ids[1]);
}

static void sources_that_share_a_callback_answer_their_own_tiles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  shared_target target = {.source_ids = {"first", "second"}};
  int shared = 0;
  for (size_t index = 0; index < 2; index += 1) {
    const mln_custom_geometry_source_options options = {
      .size = sizeof(mln_custom_geometry_source_options),
      .fetch_tile = answer_with_source_id,
      .user_data = &shared,
    };
    MLN_TEST_AWAIT_OK(mln_map_add_custom_geometry_source(
      map, mln_buffer_view_from_cstr(target.source_ids[index]), &options,
      &completion.descriptor, NULL
    ));
    char layer[96];
    snprintf(
      layer, sizeof(layer),
      "{\"id\":\"%s-dots\",\"type\":\"circle\",\"source\":\"%s\"}",
      target.source_ids[index], target.source_ids[index]
    );
    MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
      map, mln_buffer_view_from_cstr(layer), MLN_BUFFER_LITERAL(""),
      &completion.descriptor, NULL
    ));
  }
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  target.fixture = &fixture;
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, both_sources_answered, &target, "each source's own answer"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Set from MapLibre's threads, so it lives in static storage that outlives any
// registration a failing case leaves behind. The expected fragment is written
// before the callback is installed and only read after.
static atomic_bool tile_error_logged;
static char tile_error_fragment[64];

static void record_tile_error(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  mln_buffer_view message_view
) {
  const char* message = message_view.data;
  (void)user_data;
  (void)event;
  (void)code;
  if (
    severity == MLN_LOG_SEVERITY_ERROR &&
    strstr(message, tile_error_fragment) != NULL
  ) {
    mln_test_flag_set(&tile_error_logged);
  }
}

static bool tile_error_reported(void* context) {
  (void)context;
  return atomic_load(&tile_error_logged);
}

static void fail_and_recover_tile(const custom_kind* kind) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  custom_probe probe;
  mln_test_render_fixture fixture = {0};
  start_rendering(kind, runtime, map, &probe, &fixture);

  MLN_TEST_OK_MESSAGE(
    submit_and_settle(kind, map, kind->fail_root), kind->name
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_style_render_until(
      &fixture, tile_error_reported, NULL, "the tile's error"
    ),
    kind->name
  );

  const size_t fetches = atomic_load(&probe.root_fetches);
  MLN_TEST_OK_MESSAGE(
    submit_and_settle(kind, map, kind->invalidate_root), kind->name
  );
  probe_target refetched = {.probe = &probe, .root_fetches = fetches + 1};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_style_render_until(
      &fixture, probe_reached, &refetched, "the failed tile's fetch"
    ),
    kind->name
  );

  MLN_TEST_OK_MESSAGE(
    submit_and_settle(kind, map, kind->deliver_point), kind->name
  );
  feature_target features = {.fixture = &fixture, .kind = kind};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_style_render_until(
      &fixture, delivered_feature_found, &features, "the delivered feature"
    ),
    kind->name
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The log callback belongs to the process, so the case clears it even when an
// assertion fails, and Unity still reports the failure.
static void a_failed_tile_is_reported_and_invalidation_refetches_it(void) {
  FOR_EACH_KIND(kind) {
    atomic_store(&tile_error_logged, false);
    snprintf(
      tile_error_fragment, sizeof(tile_error_fragment),
      "for source %s: provider failed", kind->source_id
    );
    const mln_log_handler handler = {
      .size = sizeof(mln_log_handler),
      .callback = record_tile_error,
    };
    MLN_TEST_OK(mln_log_set_callback(&handler, NULL));
    if (TEST_PROTECT()) {
      fail_and_recover_tile(kind);
    }
    (void)mln_log_clear_callback(NULL);
  }
}

static uint32_t source_type(mln_map map, const char* id) {
  mln_test_completion info =
    mln_test_completion_default(sizeof(mln_style_source_info));
  MLN_TEST_OK(mln_map_get_style_source(
    map, mln_buffer_view_from_cstr(id), &info.descriptor, NULL
  ));
  mln_style_source_info result = {0};
  MLN_TEST_OK(mln_test_completion_finish_value(&info, &result, sizeof(result)));
  return result.type;
}

static void tile_operations_check_their_tile_and_source_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  custom_probe geometry_probe;
  custom_probe vector_probe;
  init_probe(&geometry_probe);
  init_probe(&vector_probe);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  add_source(&kinds[0], map, &geometry_probe);
  add_source(&kinds[1], map, &vector_probe);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY, source_type(map, "custom-geometry")
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR, source_type(map, "custom-vector")
  );

  const mln_buffer_view geometry = MLN_BUFFER_LITERAL("custom-geometry");
  const mln_buffer_view vector = MLN_BUFFER_LITERAL("custom-vector");
  const mln_buffer_view empty = {.data = NULL, .size = 0};
  const mln_buffer_view no_features =
    MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}");
  const mln_buffer_view missing = MLN_BUFFER_LITERAL("missing");
  MLN_TEST_AWAIT_OK(mln_map_set_custom_vector_source_tile_data(
    map, vector, root_tile, MLN_STYLE_VECTOR_TILE_ENCODING_MVT, empty,
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_custom_vector_source_tile_error(
    map, vector, root_tile, missing, &completion.descriptor, NULL
  ));
  MLN_TEST_OK(submit_and_settle(&kinds[1], map, kinds[1].invalidate_root));
  MLN_TEST_OK(submit_and_settle(&kinds[1], map, kinds[1].invalidate_region));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "southwest must be less than or equal",
    mln_map_invalidate_custom_vector_source_region(
      map, vector, (mln_lat_lng_bounds){{1, 1}, {-1, -1}},
      &completion.descriptor, NULL
    )
  );

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "within zoom bounds",
    mln_map_set_custom_geometry_source_tile_data(
      map, geometry, (mln_canonical_tile_id){.z = 1, .x = 2, .y = 0},
      no_features, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "encoding is invalid",
    mln_map_set_custom_vector_source_tile_data(
      map, vector, root_tile, (mln_style_vector_tile_encoding)2, empty,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "does not exist",
    mln_map_set_custom_vector_source_tile_data(
      map, missing, root_tile, MLN_STYLE_VECTOR_TILE_ENCODING_MVT, empty,
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_vector_source_tile_data(
      map, geometry, root_tile, MLN_STYLE_VECTOR_TILE_ENCODING_MVT, empty,
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_vector_source_tile_error(
      map, geometry, root_tile, missing, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_vector_source_tile(
      map, geometry, root_tile, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_vector_source_region(
      map, geometry, (mln_lat_lng_bounds){{-1, -1}, {1, 1}},
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_geometry_source_tile_data(
      map, vector, root_tile, no_features, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_geometry_source_tile_error(
      map, vector, root_tile, missing, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_geometry_source_tile(
      map, vector, root_tile, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_geometry_source_region(
      map, vector, (mln_lat_lng_bounds){{-1, -1}, {1, 1}},
      &completion.descriptor, NULL
    )
  );

  MLN_TEST_OK(mln_test_map_release(map));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&geometry_probe.release_count));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&vector_probe.release_count));
  mln_test_destroy_runtime(runtime);
}

// Every custom geometry option field, each set to a value in range.
static mln_custom_geometry_source_options every_geometry_option(
  custom_probe* probe
) {
  mln_custom_geometry_source_options options = {
    .size = sizeof(mln_custom_geometry_source_options)
  };
  options.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  options.min_zoom = 1;
  options.max_zoom = 12;
  options.tolerance = 0.5;
  options.tile_size = 256;
  options.buffer = 64;
  options.clip = true;
  options.wrap = true;
  options.fetch_tile = probe_fetch_tile;
  options.user_data = probe;
  options.release_user_data = probe_release;
  return options;
}

static void fractional_max_zoom(mln_custom_geometry_source_options* options) {
  options->max_zoom = 4.5;
}
static void inverted_zoom(mln_custom_geometry_source_options* options) {
  options->min_zoom = 10.0;
  options->max_zoom = 4.0;
}
static void negative_tolerance(mln_custom_geometry_source_options* options) {
  options->tolerance = -1.0;
}
static void empty_tiles(mln_custom_geometry_source_options* options) {
  options->tile_size = 0;
}
static void uneven_tiles(mln_custom_geometry_source_options* options) {
  options->tile_size = 300;
}
static void oversized_tiles(mln_custom_geometry_source_options* options) {
  options->tile_size = 16384;
}
static void oversized_buffer(mln_custom_geometry_source_options* options) {
  options->buffer = 70000;
}

static const struct {
  const char* label;
  void (*mutate)(mln_custom_geometry_source_options* options);
  const char* fragment;
} geometry_option_cases[] = {
  {"fractional max zoom", fractional_max_zoom,
   "max_zoom must be an integer within [0, 32]"},
  {"inverted zoom range", inverted_zoom,
   "min_zoom must be less than or equal to max_zoom"},
  {"negative tolerance", negative_tolerance, "tolerance"},
  {"empty tiles", empty_tiles,
   "tile_size must be a power of two within [1, 8192]"},
  {"uneven tiles", uneven_tiles,
   "tile_size must be a power of two within [1, 8192]"},
  {"oversized tiles", oversized_tiles,
   "tile_size must be a power of two within [1, 8192]"},
  {"oversized buffer", oversized_buffer,
   "buffer must be at most 2047 for a tile_size of 256"},
};

// Tile sizes with the widest buffer each takes: the widest and narrowest
// tiles, and a small tile whose limit is below the default buffer of 128.
static const struct {
  uint32_t tile_size;
  uint32_t buffer;
} geometry_buffer_limits[] = {{8192, 65535}, {16, 127}, {1, 7}};

static void custom_sources_take_and_validate_every_option(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  custom_probe geometry_probe;
  init_probe(&geometry_probe);
  custom_probe vector_probe;
  init_probe(&vector_probe);

  mln_custom_geometry_source_options geometry =
    every_geometry_option(&geometry_probe);
  MLN_TEST_AWAIT_OK(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("every-geometry-option"), &geometry,
    &completion.descriptor, NULL
  ));
  const size_t limit_count =
    sizeof(geometry_buffer_limits) / sizeof(geometry_buffer_limits[0]);
  for (size_t index = 0; index < limit_count; index += 1) {
    geometry = every_geometry_option(&geometry_probe);
    geometry.tile_size = geometry_buffer_limits[index].tile_size;
    geometry.buffer = geometry_buffer_limits[index].buffer;
    char id[24];
    snprintf(id, sizeof(id), "accepted-%zu", index);
    MLN_TEST_AWAIT_OK(mln_map_add_custom_geometry_source(
      map, mln_buffer_view_from_cstr(id), &geometry, &completion.descriptor,
      NULL
    ));

    // One pixel more is too wide.
    geometry.buffer += 1;
    snprintf(id, sizeof(id), "too-wide-%zu", index);
    char fragment[64];
    snprintf(
      fragment, sizeof(fragment),
      "buffer must be at most %u for a tile_size of %u",
      (unsigned)geometry_buffer_limits[index].buffer,
      (unsigned)geometry_buffer_limits[index].tile_size
    );
    MLN_TEST_EXPECT_COMMAND_REJECTED(
      fragment, mln_map_add_custom_geometry_source(
                  map, mln_buffer_view_from_cstr(id), &geometry,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
    );
  }
  for (size_t index = 0;
       index < sizeof(geometry_option_cases) / sizeof(geometry_option_cases[0]);
       index += 1) {
    geometry = every_geometry_option(&geometry_probe);
    geometry_option_cases[index].mutate(&geometry);
    char id[24];
    snprintf(id, sizeof(id), "geometry-%zu", index);
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_map_add_custom_geometry_source(
        map, mln_buffer_view_from_cstr(id), &geometry, &completion.descriptor,
        MLN_TEST_DIAGNOSTIC
      ),
      geometry_option_cases[index].label
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(mln_test_last_error(), geometry_option_cases[index].fragment),
      geometry_option_cases[index].label
    );
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
  }

  mln_custom_vector_source_options vector = {
    .size = sizeof(mln_custom_vector_source_options)
  };
  vector.fields = MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM |
                  MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  vector.min_zoom = 2;
  vector.max_zoom = 10;
  vector.fetch_tile = probe_fetch_tile;
  vector.user_data = &vector_probe;
  vector.release_user_data = probe_release;
  MLN_TEST_AWAIT_OK(mln_map_add_custom_vector_source(
    map, MLN_BUFFER_LITERAL("every-vector-option"), &vector,
    &completion.descriptor, NULL
  ));
  mln_custom_vector_source_options rejected = vector;
  rejected.max_zoom = 4.5;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "max_zoom must be an integer within [0, 32]",
    mln_map_add_custom_vector_source(
      map, MLN_BUFFER_LITERAL("vector-fractional"), &rejected,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  rejected.min_zoom = 10.0;
  rejected.max_zoom = 4.0;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "min_zoom must be less than or equal to max_zoom",
    mln_map_add_custom_vector_source(
      map, MLN_BUFFER_LITERAL("vector-inverted"), &rejected,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );

  MLN_TEST_OK(mln_test_map_release(map));
  TEST_ASSERT_EQUAL_size_t(
    1 + limit_count, atomic_load(&geometry_probe.release_count)
  );
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&vector_probe.release_count));
  mln_test_destroy_runtime(runtime);
}

static mln_status deliver_geometry_feature(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_custom_geometry_source_tile_data(
    map, MLN_BUFFER_LITERAL("custom-geometry"), root_tile,
    MLN_BUFFER_LITERAL(
      "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
      "\"coordinates\":[0,0]},\"properties\":{}}"
    ),
    completion, NULL
  );
}

static mln_status deliver_bare_geometry(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_custom_geometry_source_tile_data(
    map, MLN_BUFFER_LITERAL("custom-geometry"), root_tile,
    MLN_BUFFER_LITERAL("{\"type\":\"Point\",\"coordinates\":[0,0]}"),
    completion, NULL
  );
}

static void a_geometry_tile_takes_one_feature_or_a_bare_geometry(void) {
  const custom_kind* kind = &kinds[0];
  mln_status (*const deliveries[])(mln_map, const mln_completion*) = {
    deliver_geometry_feature,
    deliver_bare_geometry,
  };
  for (size_t index = 0; index < sizeof(deliveries) / sizeof(deliveries[0]);
       index += 1) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    mln_test_render_fixture fixture = {0};
    start_rendering(kind, runtime, map, &probe, &fixture);

    MLN_TEST_OK(submit_and_settle(kind, map, deliveries[index]));
    feature_target features = {.fixture = &fixture, .kind = kind};
    TEST_ASSERT_TRUE(mln_test_style_render_until(
      &fixture, delivered_feature_found, &features, "the delivered feature"
    ));

    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
  }
}

MLN_TEST_GROUP {
  RUN_TEST(a_style_replacement_releases_a_dropped_source_unsubscribed);
  RUN_TEST(an_explicit_removal_releases_once);
  RUN_TEST(accepted_adds_release_their_callback_state);
  RUN_TEST(a_delivered_tile_becomes_features_and_invalidation_refetches_it);
  RUN_TEST(a_region_invalidation_refetches_the_tiles_it_intersects);
  RUN_TEST(sources_that_share_a_callback_answer_their_own_tiles);
  RUN_TEST(a_failed_tile_is_reported_and_invalidation_refetches_it);
  RUN_TEST(tile_operations_check_their_tile_and_source_kind);
  RUN_TEST(custom_sources_take_and_validate_every_option);
  RUN_TEST(a_geometry_tile_takes_one_feature_or_a_bare_geometry);
}
