// Drives the maplibre-native-next build of maplibre_native_c through the header:
// the struct layouts of the Rust mirrors, then the flows of the ffi's
// "Render images" guide (tests/native/docsnippets/images.c) and of its
// offscreen ABI tests (tests/native/abi/render/offscreen.c), rendered on the
// Rust port and read back.

// nanosleep under -std=c23.
#define _POSIX_C_SOURCE 200809L

#include <maplibre_native_c.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int failures = 0;

#define CHECK(condition)                                                   \
  do {                                                                     \
    if (!(condition)) {                                                    \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__,     \
              #condition);                                                 \
      failures += 1;                                                       \
    }                                                                      \
  } while (0)

#define CHECK_STATUS(expected, call)                                         \
  do {                                                                       \
    mln_diagnostic diagnostic_ = {.size = sizeof(mln_diagnostic)};           \
    const mln_status status_ = (call_with_diagnostic(call, &diagnostic_));   \
    if (status_ != (expected)) {                                             \
      fprintf(stderr, "%s:%d: %s returned %d, expected %d: %s\n", __FILE__,  \
              __LINE__, #call, (int)status_, (int)(expected),                \
              diagnostic_.message);                                          \
      failures += 1;                                                         \
    }                                                                        \
  } while (0)

// Lets CHECK_STATUS pass its diagnostic as the call's last argument.
#define call_with_diagnostic(call, diagnostic) call
#define DIAG (&diagnostic_)

// MARK: - Layouts

extern bool mln_next_ffi_layout(
  const char* name, size_t* out_size, size_t* out_align
);

static void check_layout(const char* name, size_t size, size_t align) {
  size_t rust_size = 0;
  size_t rust_align = 0;
  if (!mln_next_ffi_layout(name, &rust_size, &rust_align)) {
    fprintf(stderr, "layout: Rust doesn't mirror %s\n", name);
    failures += 1;
    return;
  }
  if (rust_size != size || rust_align != align) {
    fprintf(stderr, "layout: %s is %zu/%zu in C, %zu/%zu in Rust\n", name, size,
            align, rust_size, rust_align);
    failures += 1;
  }
}

#define TYPE(t) check_layout(#t, sizeof(t), _Alignof(t))
#define FIELD(t, f) check_layout(#t "." #f, offsetof(t, f), 0)

static void test_layouts(void) {
  TYPE(mln_diagnostic);
  FIELD(mln_diagnostic, message);
  TYPE(mln_buffer_view);
  FIELD(mln_buffer_view, size);
  TYPE(mln_logical_extent);
  FIELD(mln_logical_extent, scale_factor);
  TYPE(mln_completion_result);
  FIELD(mln_completion_result, generation);
  FIELD(mln_completion_result, diagnostic);
  FIELD(mln_completion_result, value);
  FIELD(mln_completion_result, value_count);
  TYPE(mln_completion);
  FIELD(mln_completion, callback);
  FIELD(mln_completion, queue);
  TYPE(mln_wake);
  FIELD(mln_wake, queue);
  TYPE(mln_delivery_queue_options);
  FIELD(mln_delivery_queue_options, wake);
  FIELD(mln_delivery_queue_options, release_abandoned_user_data);
  TYPE(mln_runtime_options);
  FIELD(mln_runtime_options, asset_path);
  FIELD(mln_runtime_options, cache_path);
  TYPE(mln_runtime_event);
  FIELD(mln_runtime_event, generation);
  FIELD(mln_runtime_event, code);
  FIELD(mln_runtime_event, payload_type);
  FIELD(mln_runtime_event, message);
  FIELD(mln_runtime_event, payload);
  TYPE(mln_runtime_event_payload);
  TYPE(mln_runtime_event_render_frame);
  FIELD(mln_runtime_event_render_frame, stats);
  TYPE(mln_event_batch_view);
  FIELD(mln_event_batch_view, events);
  FIELD(mln_event_batch_view, event_count);
  TYPE(mln_runtime_event_handler);
  FIELD(mln_runtime_event_handler, callback);
  FIELD(mln_runtime_event_handler, queue);
  TYPE(mln_map_options);
  FIELD(mln_map_options, initial_extent);
  FIELD(mln_map_options, map_mode);
  FIELD(mln_map_options, fast_pfor_enabled);
  FIELD(mln_map_options, event_mask);
  FIELD(mln_map_options, tile_cache_enabled);
  TYPE(mln_camera_options);
  FIELD(mln_camera_options, center);
  FIELD(mln_camera_options, center_altitude);
  FIELD(mln_camera_options, padding);
  FIELD(mln_camera_options, anchor);
  FIELD(mln_camera_options, zoom);
  FIELD(mln_camera_options, field_of_view);
  TYPE(mln_camera);
  FIELD(mln_camera, zoom);
  FIELD(mln_camera, field_of_view);
  TYPE(mln_animation_options);
  FIELD(mln_animation_options, duration_ms);
  FIELD(mln_animation_options, easing);
  FIELD(mln_animation_options, transition_id);
  FIELD(mln_animation_options, end_handler);
  TYPE(mln_camera_update);
  FIELD(mln_camera_update, camera);
  FIELD(mln_camera_update, animation);
  FIELD(mln_camera_update, gesture_phase);
  TYPE(mln_map_view);
  FIELD(mln_map_view, camera);
  FIELD(mln_map_view, extent);
  FIELD(mln_map_view, projection_mode);
  FIELD(mln_map_view, viewport);
  TYPE(mln_map_snapshot);
  FIELD(mln_map_snapshot, generation);
  FIELD(mln_map_snapshot, event_mask);
  FIELD(mln_map_snapshot, latest_render_update_generation);
  FIELD(mln_map_snapshot, tile);
  FIELD(mln_map_snapshot, bounds);
  FIELD(mln_map_snapshot, free_camera);
  FIELD(mln_map_snapshot, view);
  TYPE(mln_camera_query_result);
  FIELD(mln_camera_query_result, camera);
  TYPE(mln_render_session_attach_options);
  FIELD(mln_render_session_attach_options, frame_wake);
  FIELD(mln_render_session_attach_options, driver_work_wake);
  FIELD(mln_render_session_attach_options, queue_lock);
  TYPE(mln_render_session_capabilities);
  FIELD(mln_render_session_capabilities, consumer_sync_kinds);
  TYPE(mln_offscreen_descriptor);
  FIELD(mln_offscreen_descriptor, logical_size);
  TYPE(mln_web_gpu_context_descriptor);
  FIELD(mln_web_gpu_context_descriptor, device);
  FIELD(mln_web_gpu_context_descriptor, queue);
  TYPE(mln_web_gpu_surface_descriptor);
  FIELD(mln_web_gpu_surface_descriptor, logical_size);
  FIELD(mln_web_gpu_surface_descriptor, context);
  FIELD(mln_web_gpu_surface_descriptor, surface);
  FIELD(mln_web_gpu_surface_descriptor, format);
  TYPE(mln_offscreen_device_options);
  FIELD(mln_offscreen_device_options, client_api);
  TYPE(mln_texture_readback_result);
  FIELD(mln_texture_readback_result, info);
  TYPE(mln_texture_image_info);
  FIELD(mln_texture_image_info, byte_length);
  TYPE(mln_frame_demand);
  FIELD(mln_frame_demand, token);
  TYPE(mln_render_frame_result);
  FIELD(mln_render_frame_result, token);
  FIELD(mln_render_frame_result, frame_generation);
  FIELD(mln_render_frame_result, needs_repaint);
  FIELD(mln_render_frame_result, view);
  FIELD(mln_render_frame_result, mode);
  TYPE(mln_render_frame_batch_view);
  FIELD(mln_render_frame_batch_view, results);
  FIELD(mln_render_frame_batch_view, result_count);
  TYPE(mln_render_session_snapshot);
  FIELD(mln_render_session_snapshot, extent);
  FIELD(mln_render_session_snapshot, generation);
  FIELD(mln_render_session_snapshot, failure_status);
  FIELD(mln_render_session_snapshot, has_target);
  TYPE(mln_render_abandon_result);
  FIELD(mln_render_abandon_result, reserved);
}

// MARK: - Waiting

static void sleep_ms(long ms) {
  struct timespec delay = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
  nanosleep(&delay, NULL);
}

// Waits up to 60 s (lavapipe is slow) for `flag` to become nonzero.
static bool wait_flag(atomic_int* flag) {
  for (int i = 0; i < 60000; i++) {
    if (atomic_load(flag) != 0) return true;
    sleep_ms(1);
  }
  return false;
}

typedef struct probe {
  atomic_int done;
  atomic_int released;
  mln_status status;
  uint64_t generation;
  char diagnostic[256];
  // A copy of a readback result or a buffer view.
  mln_texture_image_info info;
  uint8_t* bytes;
  size_t byte_count;
  bool readback;
} probe;

static void on_complete(void* user_data, const mln_completion_result* result) {
  probe* p = user_data;
  p->status = result->status;
  p->generation = result->generation;
  snprintf(p->diagnostic, sizeof(p->diagnostic), "%.*s",
           (int)result->diagnostic.size, (const char*)result->diagnostic.data);
  if (result->status == MLN_STATUS_OK && result->value != NULL) {
    if (p->readback) {
      const mln_texture_readback_result* readback = result->value;
      p->info = readback->info;
      p->byte_count = readback->data.size;
      p->bytes = malloc(p->byte_count);
      memcpy(p->bytes, readback->data.data, p->byte_count);
    } else {
      const mln_buffer_view* view = result->value;
      p->byte_count = view->size;
      p->bytes = malloc(view->size + 1);
      memcpy(p->bytes, view->data, view->size);
      p->bytes[view->size] = 0;
    }
  }
  atomic_store(&p->done, 1);
}

static void on_release(void* user_data) {
  probe* p = user_data;
  atomic_store(&p->released, 1);
}

static mln_completion completion_for(probe* p) {
  memset(p, 0, sizeof(*p));
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = on_complete,
    .user_data = p,
    .release_user_data = on_release,
  };
}

// Waits for a completion and its release; returns its status.
static mln_status settle(probe* p) {
  if (!wait_flag(&p->done) || !wait_flag(&p->released)) {
    fprintf(stderr, "a completion never arrived\n");
    failures += 1;
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (p->status != MLN_STATUS_OK) {
    fprintf(stderr, "completion failed: %d %s\n", (int)p->status, p->diagnostic);
  }
  return p->status;
}

// MARK: - Events through a delivery queue

static atomic_int wakes;
static atomic_int style_loaded_events;
static atomic_int render_frame_events;

static void on_wake(void* user_data) {
  (void)user_data;
  atomic_fetch_add(&wakes, 1);
}

static void on_events(void* user_data, const mln_event_batch_view* batch) {
  (void)user_data;
  for (size_t i = 0; i < batch->event_count; i++) {
    const mln_runtime_event* event =
      MLN_STRIDED_AT(mln_runtime_event, batch->events, batch->event_size, i);
    if (event->type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
      atomic_fetch_add(&style_loaded_events, 1);
    } else if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) {
      CHECK(event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME);
      if (getenv("FFI_TEST_DEBUG")) {
        printf("frame finished: mode %u needs_repaint %d placement_changed %d\n",
               event->payload.render_frame.mode, event->payload.render_frame.needs_repaint,
               event->payload.render_frame.placement_changed);
      }
      atomic_fetch_add(&render_frame_events, 1);
    } else if (event->type == MLN_RUNTIME_EVENT_MAP_IDLE && getenv("FFI_TEST_DEBUG")) {
      printf("idle\n");
    }
  }
}

static mln_delivery_queue event_queue;

// Dispatches the event queue until `counter` is nonzero.
static bool dispatch_until(atomic_int* counter) {
  for (int i = 0; i < 60000; i++) {
    mln_delivery_queue_dispatch(event_queue, NULL);
    if (atomic_load(counter) != 0) return true;
    sleep_ms(1);
  }
  return false;
}

// MARK: - Helpers

// No transition: each style replaces the color at once (both have a layer
// "bg", so the color would otherwise animate, as in C++).
static const char* background_style(const char* color) {
  static char style[320];
  snprintf(style, sizeof(style),
           "{\"version\":8,\"transition\":{\"duration\":0,\"delay\":0},"
           "\"sources\":{},\"layers\":[{\"id\":\"bg\","
           "\"type\":\"background\",\"paint\":{\"background-color\":\"%s\"}}]}",
           color);
  return style;
}

// A white background and an inline GeoJSON polygon around (0, 0), filled
// red: the tile pipeline (GeoJSON source, worker, fill bucket) end to end.
static const char* geojson_fill_style =
  "{\"version\":8,"
  "\"sources\":{\"square\":{\"type\":\"geojson\",\"data\":{\"type\":\"Feature\","
  "\"properties\":{},\"geometry\":{\"type\":\"Polygon\",\"coordinates\":"
  "[[[-10,-10],[10,-10],[10,10],[-10,10],[-10,-10]]]}}}},"
  "\"layers\":["
  "{\"id\":\"bg\",\"type\":\"background\",\"paint\":{\"background-color\":\"white\"}},"
  "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"square\","
  "\"paint\":{\"fill-color\":\"red\"}}]}";

static mln_runtime create_runtime(void) {
  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  CHECK_STATUS(MLN_STATUS_OK, mln_runtime_create(&options, &runtime, DIAG));
  CHECK(runtime != MLN_HANDLE_NULL);

  const mln_delivery_queue_options queue_options = {
    .size = sizeof(queue_options), .wake = {.callback = on_wake}
  };
  CHECK_STATUS(MLN_STATUS_OK, mln_delivery_queue_create(&queue_options, &event_queue, DIAG));
  const mln_runtime_event_handler handler = {
    .size = sizeof(handler), .callback = on_events, .queue = event_queue
  };
  CHECK_STATUS(MLN_STATUS_OK, mln_runtime_set_event_handler(runtime, &handler, NULL, DIAG));
  return runtime;
}

static void destroy_runtime(mln_runtime runtime) {
  probe p;
  const mln_completion done = completion_for(&p);
  CHECK_STATUS(MLN_STATUS_OK, mln_runtime_release(runtime, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
  mln_delivery_queue_release(event_queue);
  event_queue = MLN_HANDLE_NULL;
}

static mln_map create_map(mln_runtime runtime, mln_map_mode mode, uint32_t size) {
  probe p;
  const mln_completion done = completion_for(&p);
  const mln_map_options options = {
    .size = sizeof(mln_map_options),
    .initial_extent = {.width = size, .height = size, .scale_factor = 1.0},
    .map_mode = mode,
  };
  mln_map map = MLN_HANDLE_NULL;
  CHECK_STATUS(MLN_STATUS_OK, mln_runtime_create_map(runtime, &options, &map, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
  return map;
}

static void set_style(mln_map map, const char* json) {
  probe p;
  const mln_completion done = completion_for(&p);
  CHECK_STATUS(MLN_STATUS_OK,
               mln_map_set_style_json(map, mln_buffer_view_from_cstr(json), &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
}

static mln_render_session attach_offscreen(mln_map map, uint32_t size, mln_wake frame_wake) {
  probe p;
  const mln_completion done = completion_for(&p);
  const mln_offscreen_descriptor descriptor = {
    .size = sizeof(descriptor), .logical_size = {.width = size, .height = size}
  };
  const mln_render_session_attach_options options = {
    .size = sizeof(options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
    .frame_wake = frame_wake,
  };
  mln_render_session session = MLN_HANDLE_NULL;
  CHECK_STATUS(MLN_STATUS_OK, mln_map_attach_offscreen(map, &descriptor, &options,
                                                       MLN_HANDLE_NULL, &session, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
  return session;
}

// Reads the session's last frame; returns its pixels (caller frees).
static uint8_t* read_back(mln_render_session session, mln_texture_image_info* info) {
  probe p;
  const mln_completion done = completion_for(&p);
  p.readback = true;
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_read_texture(session, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
  *info = p.info;
  return p.bytes;
}

static void expect_pixel(const uint8_t* pixels, const mln_texture_image_info* info,
                         uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b,
                         uint8_t a) {
  if (pixels == NULL || x >= info->width || y >= info->height) {
    fprintf(stderr, "no pixel (%u, %u)\n", x, y);
    failures += 1;
    return;
  }
  const uint8_t* px = pixels + (size_t)y * info->stride + (size_t)x * 4;
  if (px[0] != r || px[1] != g || px[2] != b || px[3] != a) {
    fprintf(stderr, "pixel (%u, %u) is %u,%u,%u,%u, expected %u,%u,%u,%u\n", x, y,
            px[0], px[1], px[2], px[3], r, g, b, a);
    failures += 1;
  }
}

static void dispose_session(mln_render_session session) {
  probe p;
  const mln_completion done = completion_for(&p);
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_dispose(session, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
}

static void release_map(mln_map map) {
  probe p;
  const mln_completion done = completion_for(&p);
  CHECK_STATUS(MLN_STATUS_OK, mln_map_release(map, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
}

static void render_still(mln_map map) {
  probe p;
  const mln_completion done = completion_for(&p);
  CHECK_STATUS(MLN_STATUS_OK, mln_map_request_still_image(map, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
}

// MARK: - Cases

static void test_base(void) {
  CHECK(mln_c_version() == MLN_C_VERSION);
  CHECK(mln_supported_render_backend_mask() == MLN_RENDER_BACKEND_FLAG_WEB_GPU);

  // A stubbed function reports UNSUPPORTED with a message.
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  mln_style_expression expression = MLN_HANDLE_NULL;
  CHECK(mln_style_expression_create(mln_buffer_view_from_cstr("1"), NULL,
                                    &expression, &diagnostic) == MLN_STATUS_UNSUPPORTED);
  CHECK(strstr(diagnostic.message, "not implemented") != NULL);

  // Argument checks happen on the caller's thread.
  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_runtime_options short_options = {.size = sizeof(mln_runtime_options) - 1};
  CHECK_STATUS(MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(&short_options, &runtime, DIAG));
  CHECK_STATUS(MLN_STATUS_INVALID_ARGUMENT, mln_map_release(12345, NULL, DIAG));

  uint32_t width = 0;
  uint32_t height = 0;
  CHECK_STATUS(MLN_STATUS_OK, mln_logical_extent_physical_size(
                                (mln_logical_extent){.width = 100, .height = 50, .scale_factor = 2.0},
                                &width, &height, DIAG));
  CHECK(width == 200 && height == 100);
}

// The ffi's "Render images" guide: a static map, a style, an offscreen
// session, a camera, a still image, its pixels.
static void test_static_still_image(void) {
  mln_runtime runtime = create_runtime();
  atomic_store(&style_loaded_events, 0);
  mln_map map = create_map(runtime, MLN_MAP_MODE_STATIC, 64);
  set_style(map, background_style("#ff0000"));
  CHECK(dispatch_until(&style_loaded_events));

  mln_render_session session = attach_offscreen(map, 64, (mln_wake){0});

  const mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {
      .size = sizeof(mln_camera_options),
      .fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM,
      .center = {.latitude = 48.8566, .longitude = 2.3522},
      .zoom = 4,
    },
  };
  CHECK_STATUS(MLN_STATUS_OK, mln_map_update_camera(map, &update, NULL, DIAG));
  render_still(map);

  mln_texture_image_info info = {0};
  uint8_t* pixels = read_back(session, &info);
  CHECK(info.width == 64 && info.height == 64 && info.stride == 256);
  CHECK(info.byte_length == 64 * 64 * 4);
  expect_pixel(pixels, &info, 0, 0, 255, 0, 0, 255);
  expect_pixel(pixels, &info, 63, 63, 255, 0, 0, 255);
  free(pixels);

  // The snapshot sees the camera.
  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  CHECK_STATUS(MLN_STATUS_OK, mln_map_get_snapshot(map, &snapshot, DIAG));
  CHECK(snapshot.view.camera.zoom == 4.0);
  CHECK(snapshot.view.camera.center.latitude > 48.85 && snapshot.view.camera.center.latitude < 48.86);

  // The style URL of a JSON style is empty (a query with a buffer view).
  probe p;
  const mln_completion done = completion_for(&p);
  CHECK_STATUS(MLN_STATUS_OK, mln_map_get_style_url(map, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
  CHECK(p.byte_count == 0);
  free(p.bytes);

  // A GeoJSON fill through the tile pipeline.
  set_style(map, geojson_fill_style);
  const mln_camera_update origin = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {
      .size = sizeof(mln_camera_options),
      .fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM,
      .center = {.latitude = 0, .longitude = 0},
      .zoom = 1,
    },
  };
  CHECK_STATUS(MLN_STATUS_OK, mln_map_update_camera(map, &origin, NULL, DIAG));
  render_still(map);
  pixels = read_back(session, &info);
  expect_pixel(pixels, &info, 32, 32, 255, 0, 0, 255);
  expect_pixel(pixels, &info, 2, 2, 255, 255, 255, 255);
  free(pixels);

  // A map with a session can't be released.
  CHECK_STATUS(MLN_STATUS_INVALID_STATE, mln_map_release(map, NULL, DIAG));
  dispose_session(session);
  release_map(map);
  destroy_runtime(runtime);
}

static atomic_int frame_wakes;

static void on_frame_wake(void* user_data) {
  (void)user_data;
  atomic_fetch_add(&frame_wakes, 1);
}

// Demands one frame and returns its result.
static mln_render_frame_result demand(mln_render_session session, uint32_t flags, uint64_t token) {
  atomic_store(&frame_wakes, 0);
  const mln_frame_demand frame_demand = {.size = sizeof(frame_demand), .flags = flags, .token = token};
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_demand_frame(session, &frame_demand, DIAG));
  mln_render_frame_result result = {0};
  for (int i = 0; i < 60000; i++) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    mln_render_frame_batch_view view = {.size = sizeof(view)};
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_drain_frame_results(session, &batch, &view, DIAG));
    if (view.result_count > 0) {
      CHECK(view.result_count == 1);
      result = *MLN_STRIDED_AT(mln_render_frame_result, view.results, view.result_size, 0);
      mln_render_frame_batch_release(batch);
      CHECK(atomic_load(&frame_wakes) > 0);
      return result;
    }
    sleep_ms(1);
  }
  fprintf(stderr, "no frame result for token %llu\n", (unsigned long long)token);
  failures += 1;
  return result;
}

// The ffi's offscreen ABI test "renders frames in turn": a continuous map,
// frame demands, results, readback.
static void test_continuous_frames(void) {
  mln_runtime runtime = create_runtime();
  mln_map map = create_map(runtime, MLN_MAP_MODE_CONTINUOUS, 64);
  mln_render_session session =
    attach_offscreen(map, 64, (mln_wake){.callback = on_frame_wake});

  mln_render_session_capabilities capabilities = {.size = sizeof(capabilities)};
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_get_capabilities(session, &capabilities, DIAG));
  CHECK(capabilities.flags & MLN_RENDER_SESSION_CAPABILITY_READBACK);

  const char* colors[] = {"#0000ff", "#00ff00", "#ff0000"};
  const uint8_t rgb[][3] = {{0, 0, 255}, {0, 255, 0}, {255, 0, 0}};
  for (uint64_t token = 1; token <= 3; token++) {
    set_style(map, background_style(colors[token - 1]));
    const mln_render_frame_result result = demand(session, MLN_FRAME_DEMAND_FORCE, token);
    CHECK(result.token == token);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
    mln_texture_image_info info = {0};
    uint8_t* pixels = read_back(session, &info);
    expect_pixel(pixels, &info, 10, 10, rgb[token - 1][0], rgb[token - 1][1], rgb[token - 1][2], 255);
    free(pixels);
  }

  // Once the map settles (as the ffi's tests render "until idle"), a plain
  // demand has nothing new to draw.
  mln_render_frame_result result = {0};
  int plain_frames = 0;
  // The placement fade (300 ms) keeps a continuous map repainting for a
  // while after a style change, as in C++.
  for (uint64_t token = 100; token < 200; token++) {
    sleep_ms(20);
    result = demand(session, 0, token);
    CHECK(result.token == token);
    if (result.disposition == MLN_RENDER_RESULT_NO_UPDATE) break;
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
    plain_frames += 1;
  }
  CHECK(result.disposition == MLN_RENDER_RESULT_NO_UPDATE);
  printf("continuous map idle after %d plain frame(s)\n", plain_frames);

  // A demand that waits renders at the next update.
  atomic_store(&frame_wakes, 0);
  const mln_frame_demand wait = {.size = sizeof(wait), .flags = MLN_FRAME_DEMAND_WAIT_FOR_UPDATE, .token = 11};
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_demand_frame(session, &wait, DIAG));
  sleep_ms(50);
  set_style(map, background_style("#ffffff"));
  for (int i = 0; i < 60000 && atomic_load(&frame_wakes) == 0; i++) sleep_ms(1);
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  mln_render_frame_batch_view view = {.size = sizeof(view)};
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_drain_frame_results(session, &batch, &view, DIAG));
  CHECK(view.result_count >= 1);
  if (view.result_count >= 1) {
    result = *MLN_STRIDED_AT(mln_render_frame_result, view.results, view.result_size, 0);
    CHECK(result.token == 11);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
  }
  mln_render_frame_batch_release(batch);

  // Continuous maps raise render frame events.
  CHECK(dispatch_until(&render_frame_events));

  probe p;
  const mln_completion done = completion_for(&p);
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_detach(session, &done, DIAG));
  CHECK(settle(&p) == MLN_STATUS_OK);
  mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_get_snapshot(session, &snapshot, DIAG));
  CHECK(snapshot.state == MLN_RENDER_SESSION_STATE_DETACHED);
  dispose_session(session);
  release_map(map);
  destroy_runtime(runtime);
}

int main(void) {
  test_layouts();
  test_base();
  test_static_still_image();
  test_continuous_frames();
  if (failures != 0) {
    fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  printf("all checks passed\n");
  return 0;
}
