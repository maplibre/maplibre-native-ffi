// Drives the Orbis build of maplibre_native_c through the header: the flows
// of the "Render images" guide (tests/native/docsnippets/images.c) for a
// STATIC and a TILE map, and a CONTINUOUS map on an offscreen session driven
// by frame demands, each read back and written as a PNG.
//
// Usage: render_test <style URL> <output directory>

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

static int failures = 0;

#define CHECK_OK(call)                                                    \
  do {                                                                    \
    mln_diagnostic d_ = {.size = sizeof(mln_diagnostic)};                 \
    mln_status s_ = (call);                                               \
    if (s_ != MLN_STATUS_OK) {                                            \
      fprintf(                                                            \
        stderr, "%s:%d: %s: %d %s\n", __FILE__, __LINE__, #call, (int)s_, \
        d_.message                                                        \
      );                                                                  \
      failures++;                                                         \
    }                                                                     \
  } while (0)
#define DIAG (&d_)

// MARK: - Waiting for completions

typedef struct probe {
  atomic_int done;
  mln_status status;
  char message[512];
  uint8_t* pixels;
  mln_texture_image_info info;
} probe;

static void on_complete(void* user_data, const mln_completion_result* r) {
  probe* p = user_data;
  p->status = r->status;
  snprintf(
    p->message, sizeof p->message, "%.*s", (int)r->diagnostic.size,
    (const char*)r->diagnostic.data
  );
  if (
    r->status == MLN_STATUS_OK && r->value != NULL &&
    r->value_size == sizeof(mln_texture_readback_result)
  ) {
    const mln_texture_readback_result* rb = r->value;
    p->info = rb->info;
    p->pixels = malloc(rb->data.size);
    memcpy(p->pixels, rb->data.data, rb->data.size);
  }
  atomic_store(&p->done, 1);
}

static mln_completion completion_for(probe* p) {
  memset(p, 0, sizeof *p);
  return (mln_completion){
    .size = sizeof(mln_completion), .callback = on_complete, .user_data = p
  };
}

static void sleep_ms(long ms) {
  struct timespec t = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
  nanosleep(&t, NULL);
}

static double now_ms(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static bool wait_for(probe* p, const char* what) {
  for (int i = 0; i < 90000; i++) {
    if (atomic_load(&p->done)) {
      if (p->status != MLN_STATUS_OK) {
        fprintf(stderr, "%s failed: %d %s\n", what, p->status, p->message);
        failures++;
        return false;
      }
      return true;
    }
    sleep_ms(1);
  }
  fprintf(stderr, "%s timed out\n", what);
  failures++;
  return false;
}

// MARK: - Events

static atomic_int events_by_type[32];

static void on_events(void* user_data, const mln_event_batch_view* batch) {
  (void)user_data;
  for (size_t i = 0; i < batch->event_count; i++) {
    const mln_runtime_event* e = MLN_STRIDED_AT(
      const mln_runtime_event, batch->events, batch->event_size, i
    );
    if (e->type < 32) atomic_fetch_add(&events_by_type[e->type], 1);
    if (e->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
      fprintf(
        stderr, "loading failed: %.*s\n", (int)e->message.size,
        (const char*)e->message.data
      );
    }
  }
}

// MARK: - PNG

static void put32(uint8_t* p, uint32_t v) {
  p[0] = v >> 24;
  p[1] = v >> 16;
  p[2] = v >> 8;
  p[3] = v;
}

static void chunk(FILE* f, const char* type, const uint8_t* data, uint32_t n) {
  uint8_t head[8];
  put32(head, n);
  memcpy(head + 4, type, 4);
  fwrite(head, 1, 8, f);
  fwrite(data, 1, n, f);
  uLong crc = crc32(0, (const Bytef*)type, 4);
  crc = crc32(crc, data, n);
  uint8_t c[4];
  put32(c, (uint32_t)crc);
  fwrite(c, 1, 4, f);
}

// Writes premultiplied RGBA as straight RGBA.
static void write_png(
  const char* path, const uint8_t* rgba, uint32_t w, uint32_t h, uint32_t stride
) {
  size_t raw_n = (size_t)h * (w * 4 + 1);
  uint8_t* raw = malloc(raw_n);
  for (uint32_t y = 0; y < h; y++) {
    uint8_t* row = raw + y * (w * 4 + 1);
    row[0] = 0;
    for (uint32_t x = 0; x < w; x++) {
      const uint8_t* s = rgba + (size_t)y * stride + x * 4;
      uint8_t a = s[3];
      for (int c = 0; c < 3; c++)
        row[1 + x * 4 + c] = a ? (uint8_t)(s[c] * 255 / a) : 0;
      row[1 + x * 4 + 3] = a;
    }
  }
  uLongf z_n = compressBound(raw_n);
  uint8_t* z = malloc(z_n);
  compress(z, &z_n, raw, raw_n);
  FILE* f = fopen(path, "wb");
  fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
  uint8_t ihdr[13];
  put32(ihdr, w);
  put32(ihdr + 4, h);
  ihdr[8] = 8;
  ihdr[9] = 6;
  ihdr[10] = 0;
  ihdr[11] = 0;
  ihdr[12] = 0;
  chunk(f, "IHDR", ihdr, 13);
  chunk(f, "IDAT", z, (uint32_t)z_n);
  chunk(f, "IEND", NULL, 0);
  fclose(f);
  free(raw);
  free(z);
  printf("wrote %s (%ux%u)\n", path, w, h);
}

static bool read_back(mln_render_session session, const char* path) {
  probe p;
  mln_completion c = completion_for(&p);
  CHECK_OK(mln_render_session_read_texture(session, &c, DIAG));
  if (!wait_for(&p, "read_texture")) return false;
  // A blank image is a failure: every style here paints its background.
  size_t opaque = 0;
  for (size_t i = 3; i < p.info.byte_length; i += 4) opaque += p.pixels[i] != 0;
  if (opaque == 0) {
    fprintf(stderr, "%s is blank\n", path);
    failures++;
  }
  write_png(path, p.pixels, p.info.width, p.info.height, p.info.stride);
  free(p.pixels);
  return true;
}

// MARK: - Maps

static mln_map create_map(
  mln_runtime runtime, uint32_t w, uint32_t h, double scale, mln_map_mode mode
) {
  mln_map_options options = {
    .size = sizeof(mln_map_options),
    .initial_extent = {.width = w, .height = h, .scale_factor = scale},
    .map_mode = mode,
  };
  mln_map map = MLN_HANDLE_NULL;
  probe p;
  mln_completion c = completion_for(&p);
  CHECK_OK(mln_runtime_create_map(runtime, &options, &map, &c, DIAG));
  wait_for(&p, "create_map");
  return map;
}

static mln_render_session attach_offscreen(
  mln_map map, uint32_t w, uint32_t h
) {
  mln_offscreen_descriptor offscreen = {
    .size = sizeof(mln_offscreen_descriptor),
    .logical_size = {.width = w, .height = h}
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
  };
  mln_render_session session = MLN_HANDLE_NULL;
  probe p;
  mln_completion c = completion_for(&p);
  CHECK_OK(mln_map_attach_offscreen(
    map, &offscreen, &options, MLN_HANDLE_NULL, &session, &c, DIAG
  ));
  wait_for(&p, "attach_offscreen");
  return session;
}

static void jump(
  mln_map map, double lat, double lng, double zoom, double bearing, double pitch
) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {
      .size = sizeof(mln_camera_options),
      .fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH,
      .center = {.latitude = lat, .longitude = lng},
      .zoom = zoom,
      .bearing = bearing,
      .pitch = pitch,
    },
  };
  CHECK_OK(mln_map_update_camera(map, &update, NULL, DIAG));
}

static void release(mln_render_session session, mln_map map) {
  CHECK_OK(mln_render_session_dispose(session, DIAG));
  CHECK_OK(mln_map_dispose(map, DIAG));
}

// The images guide: a STATIC map's still image.
static void still_image(
  mln_runtime runtime, const char* style, const char* dir
) {
  mln_map map = create_map(runtime, 800, 600, 2.0, MLN_MAP_MODE_STATIC);
  CHECK_OK(
    mln_map_set_style_url(map, mln_buffer_view_from_cstr(style), NULL, DIAG)
  );
  mln_render_session session = attach_offscreen(map, 800, 600);
  jump(map, 48.8566, 2.3522, 4, 0, 0);
  probe p;
  mln_completion c = completion_for(&p);
  double start = now_ms();
  CHECK_OK(mln_map_request_still_image(map, &c, DIAG));
  if (wait_for(&p, "request_still_image")) {
    printf("still image in %.0f ms\n", now_ms() - start);
    char path[1024];
    snprintf(path, sizeof path, "%s/still.png", dir);
    read_back(session, path);
  }
  release(session, map);
}

// The images guide's tile: a TILE map at mln_camera_for_tile's camera.
static void tile_image(
  mln_runtime runtime, const char* style, const char* dir
) {
  mln_map map = create_map(runtime, 512, 512, 1.0, MLN_MAP_MODE_TILE);
  CHECK_OK(
    mln_map_set_style_url(map, mln_buffer_view_from_cstr(style), NULL, DIAG)
  );
  mln_render_session session = attach_offscreen(map, 512, 512);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {.size = sizeof(mln_camera_options)},
  };
  CHECK_OK(mln_camera_for_tile(
    (mln_canonical_tile_id){.z = 2, .x = 2, .y = 1},
    (mln_logical_size){.width = 512, .height = 512}, &update.camera, DIAG
  ));
  CHECK_OK(mln_map_update_camera(map, &update, NULL, DIAG));
  probe p;
  mln_completion c = completion_for(&p);
  CHECK_OK(mln_map_request_still_image(map, &c, DIAG));
  if (wait_for(&p, "tile still image")) {
    char path[1024];
    snprintf(path, sizeof path, "%s/tile-2-2-1.png", dir);
    read_back(session, path);
  }
  release(session, map);
}

// A CONTINUOUS map on an offscreen session: demand frames until a frame
// reports nothing left to repaint, through an eased camera move.
static void continuous(
  mln_runtime runtime, const char* style, const char* dir
) {
  mln_map map = create_map(runtime, 1024, 768, 1.0, MLN_MAP_MODE_CONTINUOUS);
  CHECK_OK(
    mln_map_set_style_url(map, mln_buffer_view_from_cstr(style), NULL, DIAG)
  );
  mln_render_session session = attach_offscreen(map, 1024, 768);
  jump(map, 35.68, 139.76, 3, 0, 0);
  mln_camera_update ease = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_EASE,
    .camera =
      {
        .size = sizeof(mln_camera_options),
        .fields = MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_BEARING |
                  MLN_CAMERA_OPTION_PITCH,
        .zoom = 5,
        .bearing = 20,
        .pitch = 40,
      },
    .animation = {.fields = MLN_ANIMATION_OPTION_DURATION, .duration_ms = 500},
  };
  CHECK_OK(mln_map_update_camera(map, &ease, NULL, DIAG));

  int rendered = 0, idle_streak = 0;
  uint64_t token = 1;
  double start = now_ms();
  while (now_ms() - start < 60000 && idle_streak < 3) {
    mln_frame_demand demand = {
      .size = sizeof(mln_frame_demand), .token = token++
    };
    CHECK_OK(mln_render_session_demand_frame(session, &demand, DIAG));
    sleep_ms(16);
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    mln_render_frame_batch_view view = {.size = sizeof view};
    CHECK_OK(
      mln_render_session_drain_frame_results(session, &batch, &view, DIAG)
    );
    for (size_t i = 0; i < view.result_count; i++) {
      const mln_render_frame_result* r = MLN_STRIDED_AT(
        const mln_render_frame_result, view.results, view.result_size, i
      );
      if (r->disposition == MLN_RENDER_RESULT_RENDERED) {
        rendered++;
        idle_streak = r->needs_repaint ? 0 : idle_streak + 1;
      } else if (r->disposition == MLN_RENDER_RESULT_NO_UPDATE) {
        idle_streak++;
      } else if (r->disposition == MLN_RENDER_RESULT_FAILED) {
        fprintf(stderr, "a frame failed\n");
        failures++;
        idle_streak = 99;
      }
    }
    mln_render_frame_batch_release(batch);
  }
  printf("continuous: %d frames in %.0f ms\n", rendered, now_ms() - start);
  mln_map_snapshot snapshot = {.size = sizeof snapshot};
  CHECK_OK(mln_map_get_snapshot(map, &snapshot, DIAG));
  printf(
    "camera: %.4f,%.4f z%.2f bearing %.1f pitch %.1f\n",
    snapshot.view.camera.center.latitude, snapshot.view.camera.center.longitude,
    snapshot.view.camera.zoom, snapshot.view.camera.bearing,
    snapshot.view.camera.pitch
  );
  char path[1024];
  snprintf(path, sizeof path, "%s/continuous.png", dir);
  read_back(session, path);
  // A forced frame draws the same camera again.
  mln_frame_demand force = {
    .size = sizeof force, .flags = MLN_FRAME_DEMAND_FORCE, .token = token++
  };
  CHECK_OK(mln_render_session_demand_frame(session, &force, DIAG));
  sleep_ms(200);
  snprintf(path, sizeof path, "%s/continuous-forced.png", dir);
  read_back(session, path);
  release(session, map);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <style URL> <output directory>\n", argv[0]);
    return 2;
  }
  const char* style = argv[1];
  const char* dir = argv[2];
  if (mln_c_version() != MLN_C_VERSION) return 1;

  mln_runtime runtime = MLN_HANDLE_NULL;
  double start = now_ms();
  CHECK_OK(mln_runtime_create(NULL, &runtime, DIAG));
  printf("runtime in %.0f ms\n", now_ms() - start);
  mln_runtime_event_handler handler = {
    .size = sizeof handler, .callback = on_events
  };
  CHECK_OK(mln_runtime_set_event_handler(runtime, &handler, NULL, DIAG));

  still_image(runtime, style, dir);
  tile_image(runtime, style, dir);
  continuous(runtime, style, dir);

  CHECK_OK(mln_runtime_dispose(runtime, DIAG));
  printf(
    "events: style loaded %d, loading finished %d, idle %d, camera will/is/did "
    "%d/%d/%d, frames %d\n",
    events_by_type[MLN_RUNTIME_EVENT_MAP_STYLE_LOADED],
    events_by_type[MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED],
    events_by_type[MLN_RUNTIME_EVENT_MAP_IDLE],
    events_by_type[MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE],
    events_by_type[MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING],
    events_by_type[MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE],
    events_by_type[MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED]
  );
  printf("%s\n", failures ? "FAILED" : "OK");
  return failures ? 1 : 0;
}
