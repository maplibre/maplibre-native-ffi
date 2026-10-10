// Which map changes reach the next frame. A mutation must publish a render
// update, and a host that renders only on those updates must then see the
// change in its pixels. A frame result reports whether the map wants another
// frame.

#include "support/style.h"
#include "support/test_support.h"

typedef struct idle_probe {
  mln_runtime runtime;
  const mln_test_render_fixture* fixture;
  bool dirty;
  bool idle;
  // The statistics of the latest finished frame, if any.
  bool has_stats;
  mln_rendering_stats stats;
} idle_probe;

// An update clears an earlier idle, and an idle after it ends the wait. Each
// finished frame's statistics replace the ones before.
static bool record_update_or_idle(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  idle_probe* probe = context;
  if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) {
    probe->dirty = true;
    probe->idle = false;
  } else if (event->type == MLN_RUNTIME_EVENT_MAP_IDLE) {
    probe->idle = true;
  } else if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) {
    probe->has_stats = true;
    probe->stats = event->payload.render_frame.stats;
  }
  return false;
}

static bool update_or_idle_arrived(void* context) {
  idle_probe* probe = context;
  (void)mln_test_render_fixture_service(probe->fixture);
  (void)mln_test_drain_counting_matching(
    probe->runtime, record_update_or_idle, probe
  );
  return probe->dirty || probe->idle;
}

// Renders the way a host driven by render updates does: one frame per batch
// of updates, until the map reports idle. The barrier orders every earlier
// command before the first drain. Returns the probe, which holds the
// statistics of the latest frame that finished on the way.
static idle_probe render_to_idle(
  mln_runtime runtime, const mln_test_render_fixture* fixture
) {
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  idle_probe probe = {.runtime = runtime, .fixture = fixture};
  const mln_test_deadline deadline = mln_test_deadline_default();
  for (;;) {
    probe.dirty = false;
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_await(
        update_or_idle_arrived, &probe, deadline, "a render update or idle"
      ),
      "the map never reported idle"
    );
    if (probe.idle) {
      return probe;
    }
    mln_frame_demand demand = mln_frame_demand_default();
    MLN_TEST_OK(
      mln_render_session_request_frame(fixture->session, &demand, NULL)
    );
    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, fixture,
      mln_render_session_barrier(fixture->session, &completion.descriptor, NULL)
    );
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    MLN_TEST_OK(
      mln_render_session_drain_frame_results(fixture->session, &batch, NULL)
    );
    mln_render_frame_batch_release(batch);
  }
}

// The premultiplied RGBA of the pixel at the center of the latest frame.
static void read_center_pixel(
  const mln_test_render_fixture* fixture, uint8_t out[4]
) {
  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_render_session_read_texture(
    fixture->session, &readback.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &readback));
  mln_texture_readback_result result = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &result, sizeof(result))
  );
  mln_texture_image_info expected = mln_texture_image_info_default();
  expected.width = 64;
  expected.height = 64;
  TEST_ASSERT_EQUAL_UINT32(expected.size, result.info.size);
  TEST_ASSERT_EQUAL_UINT32(expected.width, result.info.width);
  TEST_ASSERT_EQUAL_UINT32(expected.height, result.info.height);
  TEST_ASSERT_EQUAL_size_t(result.info.byte_length, result.data.size);
  const size_t offset = (size_t)(result.info.height / 2) * result.info.stride +
                        (size_t)(result.info.width / 2) * 4;
  memcpy(out, (const uint8_t*)result.data.data + offset, 4);
  mln_test_completion_destroy(&readback);
}

static void expect_center_pixel(
  const mln_test_render_fixture* fixture, const uint8_t expected[4],
  const char* label
) {
  uint8_t pixel[4];
  read_center_pixel(fixture, pixel);
  TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(expected, pixel, 4, label);
}

static void set_property(mln_map map, const char* property, const char* value) {
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("background"), mln_test_view_of(property),
    mln_test_view_of(value), &completion.descriptor, NULL
  ));
}

static const mln_feature_state_selector point_one = {
  .size = sizeof(mln_feature_state_selector),
  .fields = MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
  .source_id = {.data = "point", .size = 5},
  .feature_id = {.data = "1", .size = 1},
};

static void set_selected(mln_map map, const char* state) {
  MLN_TEST_AWAIT_OK(mln_map_set_feature_state(
    map, &point_one, mln_test_view_of(state), &completion.descriptor, NULL
  ));
}

static const uint8_t red[4] = {255, 0, 0, 255};
static const uint8_t green[4] = {0, 255, 0, 255};
static const uint8_t blue[4] = {0, 0, 255, 255};
static const uint8_t clear[4] = {0, 0, 0, 0};

static const uint8_t red_swatch[16] = {
  255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,
};

static void add_swatch(mln_map map) {
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = red_swatch;
  image.byte_length = sizeof(red_swatch);
  MLN_TEST_AWAIT_OK(mln_map_set_style_image(
    map, MLN_BUFFER_LITERAL("swatch"), &image, NULL, &completion.descriptor,
    NULL
  ));
}

// A value written under a long delay, whose delay is cleared before any frame
// renders: a frame that reused the layer's cached transition would still hold
// the old color.
static void recolor_under_a_cleared_delay(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
) {
  (void)runtime;
  (void)fixture;
  set_property(
    map, "background-color-transition", "{\"duration\":0,\"delay\":60000}"
  );
  set_property(map, "background-color", "\"#0000ff\"");
  set_property(
    map, "background-color-transition", "{\"duration\":0,\"delay\":0}"
  );
}

// Clearing renderer data drops the state the renderer was given, so the next
// frame must be given the map's feature state again.
static void select_then_clear_renderer_data(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
) {
  set_selected(map, "{\"selected\":true}");
  render_to_idle(runtime, fixture);
  expect_center_pixel(fixture, green, "the selected point");
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_clear_data(
      fixture->session, &completion.descriptor, NULL
    )
  );
}

static void add_the_late_source(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
) {
  (void)runtime;
  (void)fixture;
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL(
      "{\"type\":\"Feature\",\"properties\":{},\"geometry\":"
      "{\"type\":\"Point\",\"coordinates\":[0,0]}}"
    ),
    NULL, &data, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("late"), data, &completion.descriptor, NULL
  ));
  mln_geojson_source_data_destroy(data);
}

static void remove_the_swatch(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
) {
  (void)runtime;
  (void)fixture;
  MLN_TEST_AWAIT_OK(mln_map_remove_style_image(
    map, MLN_BUFFER_LITERAL("swatch"), &completion.descriptor, NULL
  ));
}

// A layer paints from global state, so a new value repaints it.
static void set_the_global_color(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
) {
  (void)runtime;
  (void)fixture;
  MLN_TEST_AWAIT_OK(mln_map_set_global_state_property(
    map, MLN_BUFFER_LITERAL("color"), MLN_BUFFER_LITERAL("\"#0000ff\""),
    &completion.descriptor, NULL
  ));
}

typedef struct pixel_row {
  const char* label;
  const char* style;
  // Runs before the first frame, or null.
  void (*prepare)(mln_map map);
  const uint8_t* before;
  void (*mutate)(
    mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
  );
  const uint8_t* after;
} pixel_row;

static const pixel_row pixel_rows[] = {
  {"a transition change reaches the pending frame",
   "{\"version\":8,\"sources\":{},\"transition\":{\"duration\":0},"
   "\"layers\":[{\"id\":\"background\",\"type\":\"background\","
   "\"paint\":{\"background-color\":\"#ff0000\"}}]}",
   NULL, red, recolor_under_a_cleared_delay, blue},
  {"cleared renderer data gets the feature state again",
   "{\"version\":8,\"transition\":{\"duration\":0},\"sources\":{\"point\":"
   "{\"type\":\"geojson\",\"data\":{\"type\":\"Feature\",\"id\":1,"
   "\"properties\":{},\"geometry\":{\"type\":\"Point\",\"coordinates\":"
   "[0,0]}}}},\"layers\":[{\"id\":\"point\",\"type\":\"circle\",\"source\":"
   "\"point\",\"paint\":{\"circle-radius\":20,\"circle-color\":[\"case\","
   "[\"boolean\",[\"feature-state\",\"selected\"],false],\"#00ff00\","
   "\"#ff0000\"]}}]}",
   NULL, red, select_then_clear_renderer_data, green},
  {"a source added after its layer renders",
   "{\"version\":8,\"transition\":{\"duration\":0},\"sources\":{},"
   "\"layers\":[{\"id\":\"point\",\"type\":\"circle\",\"source\":\"late\","
   "\"paint\":{\"circle-radius\":20,\"circle-color\":\"#00ff00\"}}]}",
   NULL, clear, add_the_late_source, green},
  // The opaque base makes the removal visible where a shared context keeps
  // its color buffer between frames.
  {"a removed pattern image stops painting",
   "{\"version\":8,\"transition\":{\"duration\":0},\"sources\":{},"
   "\"layers\":[{\"id\":\"base\",\"type\":\"background\",\"paint\":"
   "{\"background-color\":\"#0000ff\"}},{\"id\":\"pattern\",\"type\":"
   "\"background\",\"paint\":{\"background-pattern\":\"swatch\"}}]}",
   add_swatch, red, remove_the_swatch, blue},
  {"a global state change repaints what reads it",
   "{\"version\":8,\"transition\":{\"duration\":0},\"state\":{\"color\":"
   "{\"default\":\"#ff0000\"}},\"sources\":{},\"layers\":[{\"id\":"
   "\"background\",\"type\":\"background\",\"paint\":{\"background-color\":"
   "[\"global-state\",\"color\"]}}]}",
   NULL, red, set_the_global_color, blue},
};

static void each_mutation_reaches_the_pixels_of_an_update_driven_host(void) {
  for (size_t index = 0; index < sizeof(pixel_rows) / sizeof(pixel_rows[0]);
       index += 1) {
    const pixel_row* row = &pixel_rows[index];
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    mln_test_render_fixture fixture = {0};
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_render_fixture_create(map, &fixture), row->label
    );
    MLN_TEST_OK_MESSAGE(
      mln_test_map_set_style_json(map, mln_test_view_of(row->style)), row->label
    );
    if (row->prepare != NULL) {
      row->prepare(map);
    }
    render_to_idle(runtime, &fixture);
    expect_center_pixel(&fixture, row->before, row->label);
    row->mutate(runtime, map, &fixture);
    render_to_idle(runtime, &fixture);
    expect_center_pixel(&fixture, row->after, row->label);
    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
  }
}

// How many render updates the map published for the commands before the
// barrier.
static size_t take_updates(mln_runtime runtime) {
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  return mln_test_drain_counting(
    runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
  );
}

typedef struct update_state {
  mln_geojson_source_data data;
  mln_map_tile_options initial_tile;
} update_state;

static void add_empty_geojson(mln_map map, update_state* state) {
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("geo"), state->data, &completion.descriptor, NULL
  ));
}
static void remove_geojson(mln_map map, update_state* state) {
  (void)state;
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("geo"), &completion.descriptor, NULL
  ));
}
static const uint8_t white_pixel[4] = {255, 255, 255, 255};
static mln_premultiplied_rgba8_image white_image(void) {
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = white_pixel;
  image.byte_length = sizeof(white_pixel);
  return image;
}
static void add_image_source(mln_map map, update_state* state) {
  (void)state;
  const mln_lat_lng corners[] = {
    {.latitude = 1, .longitude = -1},
    {.latitude = 1, .longitude = 1},
    {.latitude = -1, .longitude = 1},
    {.latitude = -1, .longitude = -1},
  };
  const mln_premultiplied_rgba8_image image = white_image();
  MLN_TEST_AWAIT_OK(mln_map_add_image_source_image(
    map, MLN_BUFFER_LITERAL("image"), corners, 4, &image,
    &completion.descriptor, NULL
  ));
}
static void add_icon(mln_map map, update_state* state) {
  (void)state;
  const mln_premultiplied_rgba8_image image = white_image();
  MLN_TEST_AWAIT_OK(mln_map_set_style_image(
    map, MLN_BUFFER_LITERAL("icon"), &image, NULL, &completion.descriptor, NULL
  ));
}
static void remove_icon(mln_map map, update_state* state) {
  (void)state;
  MLN_TEST_AWAIT_OK(mln_map_remove_style_image(
    map, MLN_BUFFER_LITERAL("icon"), &completion.descriptor, NULL
  ));
}
static void set_tile_options(mln_map map, const mln_map_tile_options* options) {
  MLN_TEST_AWAIT_OK(
    mln_map_set_tile_options(map, options, &completion.descriptor, NULL)
  );
}
static void raise_prefetch_delta(mln_map map, update_state* state) {
  mln_map_tile_options options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
  options.prefetch_zoom_delta = state->initial_tile.prefetch_zoom_delta + 1;
  set_tile_options(map, &options);
}
static void raise_lod_min_radius(mln_map map, update_state* state) {
  mln_map_tile_options options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
  options.lod_min_radius = state->initial_tile.lod_min_radius + 1.0;
  set_tile_options(map, &options);
}
static void raise_lod_scale(mln_map map, update_state* state) {
  mln_map_tile_options options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_LOD_SCALE;
  options.lod_scale = state->initial_tile.lod_scale + 1.0;
  set_tile_options(map, &options);
}
static void set_lod_pitch_threshold(mln_map map, update_state* state) {
  (void)state;
  mln_map_tile_options options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
  options.lod_pitch_threshold = 30.0;
  set_tile_options(map, &options);
}
static void raise_lod_zoom_shift(mln_map map, update_state* state) {
  mln_map_tile_options options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
  options.lod_zoom_shift = state->initial_tile.lod_zoom_shift + 1.0;
  set_tile_options(map, &options);
}
static void use_distance_lod(mln_map map, update_state* state) {
  (void)state;
  mln_map_tile_options options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_LOD_MODE;
  options.lod_mode = MLN_TILE_LOD_MODE_DISTANCE;
  set_tile_options(map, &options);
}
static void set_layer_transition(mln_map map, update_state* state) {
  (void)state;
  set_property(map, "background-color-transition", "{\"duration\":123}");
}
static void set_style_transition(mln_map map, update_state* state) {
  (void)state;
  mln_style_transition_options options = mln_style_transition_options_default();
  options.fields = MLN_STYLE_TRANSITION_OPTION_DURATION;
  options.duration_ms = 123.0;
  MLN_TEST_AWAIT_OK(mln_map_set_style_transition_options(
    map, &options, &completion.descriptor, NULL
  ));
}
static void select_point(mln_map map, update_state* state) {
  (void)state;
  set_selected(map, "{\"selected\":true}");
}
static void merge_no_state(mln_map map, update_state* state) {
  (void)state;
  set_selected(map, "{}");
}
static void remove_point_state(mln_map map, update_state* state) {
  (void)state;
  MLN_TEST_AWAIT_OK(
    mln_map_remove_feature_state(map, &point_one, &completion.descriptor, NULL)
  );
}

typedef struct update_row {
  const char* label;
  void (*mutate)(mln_map map, update_state* state);
  // Whether the command must publish a render update. A command that changes
  // nothing the renderer draws must publish none.
  bool publishes;
} update_row;

// Each row runs after the ones before it on one map, so a repeated row
// measures a command that changes nothing.
//
// The runtime barrier after each row fences the updates its command publishes,
// which is what lets a row assert none. That holds only while no row starts
// work that finishes on a worker: the style's GeoJSON is inline, the added
// source takes data parsed before the call, and the image source takes pixels,
// so nothing loads and publishes an update into a later row. A row that
// fetched or parsed data asynchronously would need its own fence.
static const update_row update_rows[] = {
  {"inline GeoJSON added", add_empty_geojson, true},
  {"a source removed", remove_geojson, true},
  {"an image source added", add_image_source, true},
  {"a style image added", add_icon, true},
  {"a style image removed", remove_icon, true},
  {"the prefetch delta raised", raise_prefetch_delta, true},
  {"the prefetch delta unchanged", raise_prefetch_delta, false},
  {"the LOD minimum radius raised", raise_lod_min_radius, true},
  {"the LOD minimum radius unchanged", raise_lod_min_radius, false},
  {"the LOD scale raised", raise_lod_scale, true},
  {"the LOD scale unchanged", raise_lod_scale, false},
  {"the LOD pitch threshold set", set_lod_pitch_threshold, true},
  {"the LOD pitch threshold unchanged", set_lod_pitch_threshold, false},
  {"the LOD zoom shift raised", raise_lod_zoom_shift, true},
  {"the LOD zoom shift unchanged", raise_lod_zoom_shift, false},
  {"the LOD mode changed", use_distance_lod, true},
  {"the LOD mode unchanged", use_distance_lod, false},
  {"a layer transition set", set_layer_transition, true},
  {"the style transition set", set_style_transition, true},
  {"feature state set", select_point, true},
  {"the same feature state set again", select_point, false},
  {"an empty feature state merged", merge_no_state, false},
  {"feature state removed", remove_point_state, true},
  {"absent feature state removed", remove_point_state, false},
};

static void each_mutation_publishes_a_render_update_only_when_it_changes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(
    map, MLN_BUFFER_LITERAL(
           "{\"version\":8,\"sources\":{\"point\":{\"type\":\"geojson\","
           "\"data\":{\"type\":\"Feature\",\"id\":1,\"properties\":{},"
           "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]}}}},"
           "\"layers\":[{\"id\":\"background\",\"type\":\"background\"}]}"
         )
  ));
  (void)take_updates(runtime);
  update_state state = {0};
  MLN_TEST_OK(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}"),
    NULL, &state.data, NULL
  ));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  state.initial_tile = snapshot.tile;

  for (size_t index = 0; index < sizeof(update_rows) / sizeof(update_rows[0]);
       index += 1) {
    const update_row* row = &update_rows[index];
    row->mutate(map, &state);
    const size_t updates = take_updates(runtime);
    if (row->publishes) {
      TEST_ASSERT_GREATER_THAN_size_t_MESSAGE(0, updates, row->label);
    } else {
      TEST_ASSERT_EQUAL_size_t_MESSAGE(0, updates, row->label);
    }
  }

  mln_geojson_source_data_destroy(state.data);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct frame_seek {
  const mln_test_render_fixture* fixture;
  uint32_t flags;
  uint32_t disposition;
  // For a rendered frame: the needs_repaint value to find.
  bool needs_repaint;
} frame_seek;

// Demands one frame and reports whether its result matched.
static bool frame_matches(void* context) {
  const frame_seek* seek = context;
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = seek->flags;
  MLN_TEST_OK(
    mln_render_session_request_frame(seek->fixture->session, &demand, NULL)
  );
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, seek->fixture,
    mln_render_session_barrier(
      seek->fixture->session, &completion.descriptor, NULL
    )
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(seek->fixture->session, &batch, NULL)
  );
  size_t count = 0;
  MLN_TEST_OK(mln_render_frame_batch_count(batch, &count, NULL));
  bool matched = false;
  for (size_t index = 0; index < count; index += 1) {
    mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
    MLN_TEST_OK(mln_render_frame_batch_get(batch, index, &result, NULL));
    matched |= result.disposition == seek->disposition &&
               (result.disposition != MLN_RENDER_RESULT_RENDERED ||
                result.needs_repaint == seek->needs_repaint);
  }
  mln_render_frame_batch_release(batch);
  return matched;
}

// Demands frames until one result matches `seek`. Each demand waits on its
// own result, so the loop never decides by elapsed time.
static void demand_until(frame_seek seek, const char* what) {
  const mln_test_deadline deadline = mln_test_deadline_default();
  while (!frame_matches(&seek)) {
    TEST_ASSERT_FALSE_MESSAGE(mln_test_deadline_passed(deadline), what);
  }
}

// A settled static map reports no repaint, and a running paint transition
// asks for another frame from every rendered one.
static void frame_results_report_whether_the_map_needs_another_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(
    mln_test_map_set_style_json(map, mln_test_red_background_style_json)
  );

  demand_until(
    (frame_seek){
      .fixture = &fixture,
      .flags = 0,
      .disposition = MLN_RENDER_RESULT_RENDERED,
      .needs_repaint = false,
    },
    "the map never settled"
  );
  // Once settled, an if-needed demand finds no update. A rendered result here
  // means only that a fresh update slipped in, so demand again.
  demand_until(
    (frame_seek){
      .fixture = &fixture,
      .flags = MLN_FRAME_DEMAND_IF_NEEDED,
      .disposition = MLN_RENDER_RESULT_NO_UPDATE,
    },
    "an if-needed demand never found the map settled"
  );

  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("bg"),
    MLN_BUFFER_LITERAL("background-color-transition"),
    MLN_BUFFER_LITERAL("{\"duration\":60000}"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("bg"), MLN_BUFFER_LITERAL("background-color"),
    MLN_BUFFER_LITERAL("\"#0000ff\""), &completion.descriptor, NULL
  ));
  demand_until(
    (frame_seek){
      .fixture = &fixture,
      .flags = 0,
      .disposition = MLN_RENDER_RESULT_RENDERED,
      .needs_repaint = true,
    },
    "a running transition never asked for another frame"
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The backends whose renderer counts frames and draws per render pass, which
// the 0032-webgpu-frame-stats patch carries for WebGPU, Metal, and Vulkan.
#if defined(MLN_FFI_TEST_BACKEND_WEBGPU) || \
  defined(MLN_FFI_TEST_BACKEND_METAL) || defined(MLN_FFI_TEST_BACKEND_VULKAN)
#define COUNTS_DRAWS_PER_FRAME 1
#endif

#ifdef COUNTS_DRAWS_PER_FRAME
static mln_rendering_stats render_stats_to_idle(
  mln_runtime runtime, const mln_test_render_fixture* fixture
) {
  const idle_probe probe = render_to_idle(runtime, fixture);
  TEST_ASSERT_TRUE_MESSAGE(
    probe.has_stats, "rendering to idle finished no frame"
  );
  return probe.stats;
}

// Each frame advances the frame count and adds its draws to the total. A frame
// with nothing visible draws nothing and leaves the total as it was.
static void frame_statistics_count_each_frame_and_its_draws(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_map_set_style_json(
    map, MLN_BUFFER_LITERAL(
           "{\"version\":8,\"sources\":{\"point\":{\"type\":\"geojson\","
           "\"data\":{\"type\":\"Feature\",\"properties\":{},\"geometry\":"
           "{\"type\":\"Point\",\"coordinates\":[0,0]}}}},\"layers\":[{\"id\":"
           "\"circle\",\"type\":\"circle\",\"source\":\"point\",\"paint\":"
           "{\"circle-radius\":5}}]}"
         )
  ));

  const mln_rendering_stats first = render_stats_to_idle(runtime, &fixture);
  TEST_ASSERT_GREATER_THAN_INT64(0, first.frame_count);
  TEST_ASSERT_GREATER_THAN_INT64(0, first.draw_call_count);
  TEST_ASSERT_GREATER_OR_EQUAL_INT64(
    first.draw_call_count, first.total_draw_call_count
  );

  MLN_TEST_OK(mln_test_map_request_repaint(map));
  const mln_rendering_stats second = render_stats_to_idle(runtime, &fixture);
  TEST_ASSERT_GREATER_THAN_INT64(0, second.draw_call_count);
  TEST_ASSERT_EQUAL_INT64(first.frame_count + 1, second.frame_count);
  TEST_ASSERT_EQUAL_INT64(
    first.total_draw_call_count + second.draw_call_count,
    second.total_draw_call_count
  );

  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("circle"), MLN_BUFFER_LITERAL("visibility"),
    MLN_BUFFER_LITERAL("\"none\""), &completion.descriptor, NULL
  ));
  const mln_rendering_stats empty = render_stats_to_idle(runtime, &fixture);
  // Source fading can request several empty frames after the layer hides.
  TEST_ASSERT_GREATER_THAN_INT64(second.frame_count, empty.frame_count);
  TEST_ASSERT_EQUAL_INT64(0, empty.draw_call_count);
  TEST_ASSERT_EQUAL_INT64(
    second.total_draw_call_count, empty.total_draw_call_count
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(each_mutation_reaches_the_pixels_of_an_update_driven_host);
  RUN_TEST(each_mutation_publishes_a_render_update_only_when_it_changes);
  RUN_TEST(frame_results_report_whether_the_map_needs_another_frame);
#ifdef COUNTS_DRAWS_PER_FRAME
  RUN_TEST(frame_statistics_count_each_frame_and_its_draws);
#endif
}
