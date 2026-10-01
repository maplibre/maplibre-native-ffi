// Custom geometry and custom MVT vector sources: when the host's callback state
// is released, which tiles the fetch and cancel callbacks receive during a
// stepped render, delivered tiles becoming features, and which tiles an
// invalidation refetches.
//
// The two kinds share their contracts, so every shared case runs once per kind.
// The release callback is the only report that the host's state is no longer
// referenced, and it runs whatever the map's event mask.

#include "support/style.h"
#include "support/test_support.h"

// Written from MapLibre threads and read from the test thread, so atomic. Each
// callback pulses, so the waits below wake on it.
typedef struct custom_probe {
  atomic_size_t release_count;
  atomic_size_t fetch_count;
  atomic_size_t root_fetches;
  atomic_size_t root_cancels;
  atomic_uint deepest_fetch;
  // Fetches and cancels of each zoom-1 tile, indexed by x + 2 * y.
  atomic_size_t quadrant_fetches[4];
  atomic_size_t quadrant_cancels[4];
} custom_probe;

static void probe_fetch_tile(void* user_data, mln_canonical_tile_id tile_id) {
  custom_probe* probe = user_data;
  atomic_fetch_add(&probe->fetch_count, 1);
  if (tile_id.z == 0) {
    atomic_fetch_add(&probe->root_fetches, 1);
  }
  if (tile_id.z == 1) {
    atomic_fetch_add(&probe->quadrant_fetches[tile_id.x + 2 * tile_id.y], 1);
  }
  unsigned int deepest = atomic_load(&probe->deepest_fetch);
  while (
    tile_id.z > deepest &&
    !atomic_compare_exchange_weak(&probe->deepest_fetch, &deepest, tile_id.z)) {
  }
  mln_test_pulse();
}

static void probe_cancel_tile(void* user_data, mln_canonical_tile_id tile_id) {
  custom_probe* probe = user_data;
  if (tile_id.z == 0) {
    atomic_fetch_add(&probe->root_cancels, 1);
  }
  if (tile_id.z == 1) {
    atomic_fetch_add(&probe->quadrant_cancels[tile_id.x + 2 * tile_id.y], 1);
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
};

static const mln_canonical_tile_id root_tile = {.z = 0, .x = 0, .y = 0};

static mln_status add_geometry_source(
  mln_map map, custom_probe* probe, enum add_variant variant,
  const mln_completion* completion
) {
  mln_custom_geometry_source_options options =
    mln_custom_geometry_source_options_default();
  options.fetch_tile = variant == ADD_WITHOUT_FETCH ? NULL : probe_fetch_tile;
  options.cancel_tile = probe_cancel_tile;
  options.user_data = probe;
  options.release_user_data = probe_release;
  if (variant == ADD_NEGATIVE_MIN_ZOOM) {
    options.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
    options.min_zoom = -1;
  }
  return mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("custom-geometry"), &options, completion, NULL
  );
}

static mln_status add_mvt_source(
  mln_map map, custom_probe* probe, enum add_variant variant,
  const mln_completion* completion
) {
  mln_custom_mvt_vector_source_options options =
    mln_custom_mvt_vector_source_options_default();
  options.fetch_tile = variant == ADD_WITHOUT_FETCH ? NULL : probe_fetch_tile;
  options.cancel_tile = probe_cancel_tile;
  options.user_data = probe;
  options.release_user_data = probe_release;
  if (variant == ADD_NEGATIVE_MIN_ZOOM) {
    options.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
    options.min_zoom = -1;
  }
  return mln_map_add_custom_mvt_vector_source(
    map, MLN_BUFFER_LITERAL("custom-mvt-vector"), &options, completion, NULL
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
  return mln_map_set_custom_mvt_vector_source_tile_data(
    map, MLN_BUFFER_LITERAL("custom-mvt-vector"), root_tile,
    (mln_buffer_view){.data = mvt_point_tile, .size = sizeof(mvt_point_tile)},
    completion, NULL
  );
}

static mln_status invalidate_geometry_tile(
  mln_map map, const mln_completion* completion
) {
  return mln_map_invalidate_custom_geometry_source_tile(
    map, MLN_BUFFER_LITERAL("custom-geometry"), root_tile, completion, NULL
  );
}

static mln_status invalidate_mvt_tile(
  mln_map map, const mln_completion* completion
) {
  return mln_map_invalidate_custom_mvt_vector_source_tile(
    map, MLN_BUFFER_LITERAL("custom-mvt-vector"), root_tile, completion, NULL
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
  mln_status (*invalidate_root)(mln_map map, const mln_completion* completion);
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
    .invalidate_root = invalidate_geometry_tile,
  },
  {
    .name = "custom MVT vector",
    .source_id = "custom-mvt-vector",
    .source_layer = "points",
    .layer_json = "{\"id\":\"custom-dots\",\"type\":\"circle\",\"source\":"
                  "\"custom-mvt-vector\",\"source-layer\":\"points\"}",
    .add = add_mvt_source,
    .deliver_point = deliver_mvt_point,
    .invalidate_root = invalidate_mvt_tile,
  },
};

#define FOR_EACH_KIND(kind)             \
  for (const custom_kind* kind = kinds; \
       kind < kinds + sizeof(kinds) / sizeof(kinds[0]); kind += 1)

static void init_probe(custom_probe* probe) {
  atomic_init(&probe->release_count, 0);
  atomic_init(&probe->fetch_count, 0);
  atomic_init(&probe->root_fetches, 0);
  atomic_init(&probe->root_cancels, 0);
  atomic_init(&probe->deepest_fetch, 0);
  for (size_t index = 0; index < 4; index += 1) {
    atomic_init(&probe->quadrant_fetches[index], 0);
    atomic_init(&probe->quadrant_cancels[index], 0);
  }
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
  mln_test_completion add = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    kind->add(map, probe, ADD_VALID, &add.descriptor), kind->name
  );
  MLN_TEST_OK_MESSAGE(mln_test_completion_settle(&add), kind->name);
}

static void draw_source(const custom_kind* kind, mln_map map) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_test_view_of(kind->layer_json), MLN_BUFFER_LITERAL(""),
    &completion.descriptor, NULL
  ));
}

static mln_map create_map_without_style_events(mln_runtime runtime) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 256;
  options.initial_extent.height = 256;
  options.event_mask = MLN_RUNTIME_EVENT_MASK_ALL &
                       ~(uint64_t)MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED;
  return mln_test_create_map_with_options(runtime, &options);
}

// A host that never subscribes to style-loaded events still learns that a style
// replacement dropped its source.
static void a_style_replacement_releases_a_dropped_source_unsubscribed(void) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = create_map_without_style_events(runtime);
    custom_probe probe;
    init_probe(&probe);

    mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
    add_source(kind, map, &probe);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      0, atomic_load(&probe.release_count), kind->name
    );

    mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );
    TEST_ASSERT_EQUAL_size_t(
      0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
    );

    // The map no longer references the state, so retiring it releases nothing.
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

    mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
    add_source(kind, map, &probe);

    mln_test_completion removal = mln_test_completion_default(0);
    MLN_TEST_OK(mln_map_remove_style_source(
      map, mln_test_view_of(kind->source_id), &removal.descriptor, NULL
    ));
    MLN_TEST_OK(mln_test_completion_finish(&removal));
    TEST_ASSERT_EQUAL_UINT32(
      MLN_COMMAND_DISPOSITION_COMMITTED,
      mln_test_completion_disposition(&removal)
    );
    mln_test_completion_destroy(&removal);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );

    // A style load after the removal has nothing left to reconcile.
    mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
    mln_test_destroy_map(map);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );
    mln_test_destroy_runtime(runtime);
  }
}

// A synchronous rejection never references the host's state, so it releases
// nothing. An accepted command that then fails does reference it and releases
// it. Closing the map releases each live source's state exactly once.
static void accepted_adds_release_their_callback_state(void) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    init_probe(&probe);
    mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);

    static const enum add_variant rejected_variants[] = {
      ADD_NEGATIVE_MIN_ZOOM,
      ADD_WITHOUT_FETCH,
    };
    for (size_t index = 0; index < 2; index += 1) {
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
    // ID already exists. Its callback state is released independently.
    mln_test_completion duplicate = mln_test_completion_default(0);
    MLN_TEST_OK(kind->add(map, &probe, ADD_VALID, &duplicate.descriptor));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_settle(&duplicate),
      kind->name
    );
    MLN_TEST_OK(mln_test_runtime_barrier(runtime));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );

    mln_test_destroy_map(map);
    MLN_TEST_OK(mln_test_runtime_barrier(runtime));
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
  unsigned int deepest_fetch;
} probe_target;

static bool probe_reached(void* context) {
  const probe_target* target = context;
  return atomic_load(&target->probe->root_fetches) >= target->root_fetches &&
         atomic_load(&target->probe->root_cancels) >= target->root_cancels &&
         atomic_load(&target->probe->deepest_fetch) >= target->deepest_fetch;
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

// Loads a background style with the kind's source and a layer that draws it,
// attaches a render session, and renders until the root tile is fetched.
static void start_rendering(
  const custom_kind* kind, mln_runtime runtime, mln_map map,
  custom_probe* probe, mln_test_render_fixture* fixture
) {
  init_probe(probe);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  add_source(kind, map, probe);
  draw_source(kind, map);
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create(map, fixture), kind->name
  );
  probe_target fetched = {.probe = probe, .root_fetches = 1};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_style_render_until(
      fixture, probe_reached, &fetched, "the root tile's fetch"
    ),
    kind->name
  );
}

// Only tiles a frame requires are fetched. A tile the camera leaves before its
// data arrives is cancelled, and the camera's new tiles are fetched instead.
static void fetches_follow_the_rendered_tiles(void) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    mln_test_render_fixture fixture = {0};
    start_rendering(kind, runtime, map, &probe, &fixture);

    mln_camera_update update = mln_camera_update_default();
    update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
    update.camera.latitude = 10.0;
    update.camera.longitude = 10.0;
    // Deep enough that the root tile is neither ideal nor one of the pan
    // tiles MapLibre Native prefetches four zooms up.
    update.camera.zoom = 6.0;
    MLN_TEST_AWAIT_OK(
      mln_map_update_camera(map, &update, &completion.descriptor, NULL)
    );
    probe_target moved = {
      .probe = &probe,
      .root_fetches = 1,
      .root_cancels = 1,
      .deepest_fetch = 6,
    };
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_style_render_until(
        &fixture, probe_reached, &moved, "the root tile's cancel"
      ),
      kind->name
    );

    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    MLN_TEST_OK(mln_test_runtime_barrier(runtime));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1, atomic_load(&probe.release_count), kind->name
    );
    mln_test_destroy_runtime(runtime);
  }
}

// Data delivered for a fetched tile becomes that tile's features, and
// invalidating the tile cancels it and fetches it again.
static void a_delivered_tile_becomes_features_and_invalidation_refetches_it(
  void
) {
  FOR_EACH_KIND(kind) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    custom_probe probe;
    mln_test_render_fixture fixture = {0};
    start_rendering(kind, runtime, map, &probe, &fixture);

    // A fetched tile holds no features until the host delivers its data.
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

    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
  }
}

static bool every_quadrant_fetched(void* context) {
  const custom_probe* probe = context;
  for (size_t index = 0; index < 4; index += 1) {
    if (atomic_load(&probe->quadrant_fetches[index]) == 0) {
      return false;
    }
  }
  return true;
}

typedef struct quadrant_target {
  const custom_probe* probe;
  size_t index;
  size_t fetches;
} quadrant_target;

static bool quadrant_refetched(void* context) {
  const quadrant_target* target = context;
  return atomic_load(&target->probe->quadrant_fetches[target->index]) >
         target->fetches;
}

// A region invalidation cancels and refetches the fetched tiles inside it, and
// leaves the tiles outside it alone.
static void a_region_invalidation_refetches_only_the_tiles_inside_it(void) {
  const custom_kind* kind = &kinds[0];
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  custom_probe probe;
  mln_test_render_fixture fixture = {0};
  start_rendering(kind, runtime, map, &probe, &fixture);

  // At zoom 1 the viewport's center is the corner all four tiles share, so
  // every one of them is fetched.
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = 0.0;
  update.camera.longitude = 0.0;
  update.camera.zoom = 1.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, every_quadrant_fetched, &probe, "every zoom-1 tile's fetch"
  ));
  size_t fetches[4];
  size_t cancels[4];
  for (size_t index = 0; index < 4; index += 1) {
    fetches[index] = atomic_load(&probe.quadrant_fetches[index]);
    cancels[index] = atomic_load(&probe.quadrant_cancels[index]);
  }

  // The region lies inside tile 1/1/0, the northeast quadrant.
  const size_t inside = 1;
  const mln_lat_lng_bounds northeast = {
    .southwest = {.latitude = 10.0, .longitude = 10.0},
    .northeast = {.latitude = 20.0, .longitude = 20.0},
  };
  MLN_TEST_AWAIT_OK(mln_map_invalidate_custom_geometry_source_region(
    map, MLN_BUFFER_LITERAL("custom-geometry"), northeast,
    &completion.descriptor, NULL
  ));
  quadrant_target refetched = {
    .probe = &probe,
    .index = inside,
    .fetches = fetches[inside],
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, quadrant_refetched, &refetched, "the invalidated tile's fetch"
  ));

  // The tile loader runs the refetch after the whole invalidation, so every
  // cancel the invalidation made has already arrived.
  for (size_t index = 0; index < 4; index += 1) {
    const size_t changed = index == inside ? 1 : 0;
    TEST_ASSERT_EQUAL_size_t(
      cancels[index] + changed, atomic_load(&probe.quadrant_cancels[index])
    );
    TEST_ASSERT_EQUAL_size_t(
      fetches[index] + changed, atomic_load(&probe.quadrant_fetches[index])
    );
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void tile_delivery_and_invalidate_accept_an_empty_tile(void) {
  const custom_kind* kind = &kinds[1];
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  custom_probe probe;
  init_probe(&probe);

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  add_source(kind, map, &probe);

  mln_test_completion info =
    mln_test_completion_default(sizeof(mln_style_source_result));
  MLN_TEST_OK(mln_map_get_style_source_info(
    map, MLN_BUFFER_LITERAL("custom-mvt-vector"), &info.descriptor, NULL
  ));
  mln_style_source_result source_result = {0};
  MLN_TEST_OK(mln_test_completion_finish_value(
    &info, &source_result, sizeof(source_result)
  ));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR, source_result.info.type
  );

  const mln_buffer_view empty = {.data = NULL, .size = 0};
  MLN_TEST_AWAIT_OK(mln_map_set_custom_mvt_vector_source_tile_data(
    map, MLN_BUFFER_LITERAL("custom-mvt-vector"), root_tile, empty,
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_custom_mvt_vector_source_tile_error(
    map, MLN_BUFFER_LITERAL("custom-mvt-vector"), root_tile,
    MLN_BUFFER_LITERAL("missing"), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(submit_and_settle(kind, map, kind->invalidate_root));

  mln_test_destroy_map(map);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

static uint32_t source_type(mln_map map, const char* id) {
  mln_test_completion info =
    mln_test_completion_default(sizeof(mln_style_source_result));
  MLN_TEST_OK(mln_map_get_style_source_info(
    map, mln_test_view_of(id), &info.descriptor, NULL
  ));
  mln_style_source_result result = {0};
  MLN_TEST_OK(mln_test_completion_finish_value(&info, &result, sizeof(result)));
  return result.info.type;
}

// Each kind takes every option it declares and reports its own source type,
// and rejects a maximum zoom that is not a tile zoom before the call returns.
static void custom_sources_take_every_option_they_declare(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  custom_probe geometry_probe;
  init_probe(&geometry_probe);
  custom_probe mvt_probe;
  init_probe(&mvt_probe);

  mln_custom_geometry_source_options geometry =
    mln_custom_geometry_source_options_default();
  geometry.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  geometry.min_zoom = 1;
  geometry.max_zoom = 12;
  geometry.tolerance = 0.5;
  geometry.tile_size = 256;
  geometry.buffer = 64;
  geometry.clip = true;
  geometry.wrap = true;
  geometry.fetch_tile = probe_fetch_tile;
  geometry.user_data = &geometry_probe;
  geometry.release_user_data = probe_release;
  MLN_TEST_AWAIT_OK(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("every-geometry-option"), &geometry,
    &completion.descriptor, NULL
  ));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR,
    source_type(map, "every-geometry-option")
  );

  mln_custom_mvt_vector_source_options mvt =
    mln_custom_mvt_vector_source_options_default();
  mvt.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM |
               MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  mvt.min_zoom = 2;
  mvt.max_zoom = 10;
  mvt.fetch_tile = probe_fetch_tile;
  mvt.user_data = &mvt_probe;
  mvt.release_user_data = probe_release;
  MLN_TEST_AWAIT_OK(mln_map_add_custom_mvt_vector_source(
    map, MLN_BUFFER_LITERAL("every-mvt-option"), &mvt, &completion.descriptor,
    NULL
  ));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR,
    source_type(map, "every-mvt-option")
  );

  mln_custom_mvt_vector_source_options past_the_last_zoom = mvt;
  past_the_last_zoom.max_zoom = 33;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "max_zoom",
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL("past-the-last-zoom"), &past_the_last_zoom,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );

  mln_test_destroy_map(map);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&geometry_probe.release_count));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&mvt_probe.release_count));
  mln_test_destroy_runtime(runtime);
}

static void tile_operations_reject_the_other_custom_source_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  custom_probe geometry_probe;
  custom_probe mvt_probe;
  init_probe(&geometry_probe);
  init_probe(&mvt_probe);

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  add_source(&kinds[0], map, &geometry_probe);
  add_source(&kinds[1], map, &mvt_probe);

  // A tile operation against the other custom source kind is accepted, then
  // fails application on the worker.
  const mln_buffer_view geometry = MLN_BUFFER_LITERAL("custom-geometry");
  const mln_buffer_view mvt = MLN_BUFFER_LITERAL("custom-mvt-vector");
  const mln_buffer_view empty = {.data = NULL, .size = 0};
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_mvt_vector_source_tile_data(
      map, geometry, root_tile, empty, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_mvt_vector_source_tile_error(
      map, geometry, root_tile, MLN_BUFFER_LITERAL("missing"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_mvt_vector_source_tile(
      map, geometry, root_tile, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_geometry_source_tile_data(
      map, mvt, root_tile,
      MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_geometry_source_tile(
      map, mvt, root_tile, &completion.descriptor, NULL
    )
  );
  const mln_lat_lng_bounds bounds = {
    .southwest = {.latitude = -1.0, .longitude = -1.0},
    .northeast = {.latitude = 1.0, .longitude = 1.0},
  };
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_geometry_source_region(
      map, mvt, bounds, &completion.descriptor, NULL
    )
  );

  mln_test_destroy_map(map);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&geometry_probe.release_count));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&mvt_probe.release_count));
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_style_replacement_releases_a_dropped_source_unsubscribed);
  RUN_TEST(an_explicit_removal_releases_once);
  RUN_TEST(accepted_adds_release_their_callback_state);
  RUN_TEST(fetches_follow_the_rendered_tiles);
  RUN_TEST(a_delivered_tile_becomes_features_and_invalidation_refetches_it);
  RUN_TEST(a_region_invalidation_refetches_only_the_tiles_inside_it);
  RUN_TEST(tile_delivery_and_invalidate_accept_an_empty_tile);
  RUN_TEST(tile_operations_reject_the_other_custom_source_kind);
  RUN_TEST(custom_sources_take_every_option_they_declare);
}
