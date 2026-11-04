// macOS only: a continuous map's Metal surface session
// (mln_map_attach_metal_surface, MLN_RENDER_DRIVER_CORE_WORKER) on a
// CAMetalLayer, driven the way MapLibre iOS's map view drives it: a frame
// demand at a time, its frame result drained after the frame wake.
//
// The layer records each drawable that it hands out, so the program reads
// back the pixels that a frame drew on it. It also resizes the session,
// releases and replaces its layer, and queries and maintains its renderer.
//
// Build and run it against a macos-arm64-metal-next build:
//
//   B=build/macos-arm64-metal-next
//   clang -fobjc-arc -Iinclude -F$B -framework MaplibreNativeC \
//     -framework Foundation -framework QuartzCore -framework Metal \
//     -Wl,-rpath,$PWD/$B src/next/tests/metal_surface_test.m \
//     -o $B/mln_next_metal_surface_test && $B/mln_next_metal_surface_test

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <maplibre_native_c.h>

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

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
    const mln_status status_ = (call);                                       \
    if (status_ != (expected)) {                                             \
      fprintf(stderr, "%s:%d: %s returned %d, expected %d: %s\n", __FILE__,  \
              __LINE__, #call, (int)status_, (int)(expected),                \
              diagnostic_.message);                                          \
      failures += 1;                                                         \
    }                                                                        \
  } while (0)

#define D (&diagnostic_)

// A CAMetalLayer that keeps the last drawable it handed out.
@interface CaptureLayer : CAMetalLayer
@property(atomic, strong) id<CAMetalDrawable> lastDrawable;
@end

@implementation CaptureLayer
- (id<CAMetalDrawable>)nextDrawable {
  id<CAMetalDrawable> drawable = [super nextDrawable];
  self.lastDrawable = drawable;
  return drawable;
}
@end

static CaptureLayer* make_layer(double width, double height, double scale) {
  CaptureLayer* layer = [CaptureLayer layer];
  layer.bounds = CGRectMake(0, 0, width, height);
  layer.contentsScale = scale;
  return layer;
}

// MARK: - Completions

typedef struct waiter {
  atomic_bool done;
  mln_status status;
  char text[4096];
  size_t value_count;
} waiter;

static void complete(void* user_data, const mln_completion_result* result) {
  waiter* w = user_data;
  w->status = result->status;
  w->value_count = result->value_count;
  if (result->value_size == sizeof(mln_buffer_view) && result->value_count == 1) {
    const mln_buffer_view* text = result->value;
    const size_t length = text->size < sizeof(w->text) - 1 ? text->size : sizeof(w->text) - 1;
    memcpy(w->text, text->data, length);
    w->text[length] = '\0';
  }
  atomic_store(&w->done, true);
}

static mln_completion completion_for(waiter* w) {
  *w = (waiter){0};
  return (mln_completion){.size = sizeof(mln_completion), .callback = complete, .user_data = w};
}

static bool wait_until(atomic_bool* flag) {
  for (int i = 0; i < 2000 && !atomic_load(flag); i += 1) {
    usleep(5000);
  }
  return atomic_load(flag);
}

static mln_status finish(waiter* w) {
  if (!wait_until(&w->done)) {
    fprintf(stderr, "a completion timed out\n");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return w->status;
}

// MARK: - Frames

static atomic_int frame_wakes = 0;

static void count_wake(void* user_data) {
  (void)user_data;
  atomic_fetch_add(&frame_wakes, 1);
}

static uint64_t next_token = 1;

// Demands one frame and returns its frame result.
static mln_render_frame_result demand(mln_render_session session, uint32_t flags) {
  const uint64_t token = next_token++;
  const mln_frame_demand frame = {.size = sizeof(mln_frame_demand), .flags = flags, .token = token};
  CHECK_STATUS(MLN_STATUS_OK, mln_render_session_demand_frame(session, &frame, D));
  for (int i = 0; i < 2000; i += 1) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    mln_render_frame_batch_view view = {.size = sizeof(mln_render_frame_batch_view)};
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_drain_frame_results(session, &batch, &view, D));
    mln_render_frame_result found = {0};
    bool matched = false;
    for (size_t index = 0; index < view.result_count; index += 1) {
      const mln_render_frame_result* result =
        (const mln_render_frame_result*)((const char*)view.results + index * view.result_size);
      if (result->token == token) {
        found = *result;
        matched = true;
      }
    }
    if (batch != MLN_HANDLE_NULL) {
      mln_render_frame_batch_release(batch);
    }
    if (matched) {
      return found;
    }
    usleep(5000);
  }
  fprintf(stderr, "frame %llu timed out\n", (unsigned long long)token);
  failures += 1;
  return (mln_render_frame_result){.disposition = UINT32_MAX};
}

// Copies the layer's last drawable into BGRA8 `pixels`; returns its size.
static MTLSize read_drawable(CaptureLayer* layer, uint8_t* pixels, size_t capacity) {
  id<CAMetalDrawable> drawable = layer.lastDrawable;
  if (drawable == nil) {
    return MTLSizeMake(0, 0, 0);
  }
  id<MTLTexture> texture = drawable.texture;
  const NSUInteger width = texture.width;
  const NSUInteger height = texture.height;
  if (width * height * 4 > capacity) {
    return MTLSizeMake(0, 0, 0);
  }
  // The frame's work ran on the session's queue; this reads on another.
  usleep(100000);
  id<MTLCommandQueue> queue = [texture.device newCommandQueue];
  id<MTLBuffer> buffer = [texture.device newBufferWithLength:width * height * 4
                                                    options:MTLResourceStorageModeShared];
  id<MTLCommandBuffer> commands = [queue commandBuffer];
  id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
  [blit copyFromTexture:texture
               sourceSlice:0
               sourceLevel:0
              sourceOrigin:MTLOriginMake(0, 0, 0)
                sourceSize:MTLSizeMake(width, height, 1)
                  toBuffer:buffer
         destinationOffset:0
    destinationBytesPerRow:width * 4
  destinationBytesPerImage:width * height * 4];
  [blit endEncoding];
  [commands commit];
  [commands waitUntilCompleted];
  memcpy(pixels, buffer.contents, width * height * 4);
  return MTLSizeMake(width, height, 1);
}

// Whether every pixel of the layer's last drawable is `bgra`.
static bool drawable_is(CaptureLayer* layer, const uint8_t bgra[4], MTLSize expected) {
  static uint8_t pixels[512 * 512 * 4];
  const MTLSize size = read_drawable(layer, pixels, sizeof(pixels));
  if (size.width != expected.width || size.height != expected.height) {
    fprintf(stderr, "the drawable is %lux%lu, expected %lux%lu\n", (unsigned long)size.width,
            (unsigned long)size.height, (unsigned long)expected.width, (unsigned long)expected.height);
    return false;
  }
  for (size_t index = 0; index < size.width * size.height; index += 1) {
    if (memcmp(&pixels[index * 4], bgra, 4) != 0) {
      fprintf(stderr, "pixel %zu is %u %u %u %u\n", index, pixels[index * 4], pixels[index * 4 + 1],
              pixels[index * 4 + 2], pixels[index * 4 + 3]);
      return false;
    }
  }
  return true;
}

// No transition, so the next frame shows a change.
static const char red_style[] =
  "{\"version\":8,\"transition\":{\"duration\":0},\"sources\":{},"
  "\"layers\":[{\"id\":\"bg\",\"type\":"
  "\"background\",\"paint\":{\"background-color\":\"#ff0000\"}}]}";
static const char blue_style[] =
  "{\"version\":8,\"transition\":{\"duration\":0},\"sources\":{},"
  "\"layers\":[{\"id\":\"bg\",\"type\":"
  "\"background\",\"paint\":{\"background-color\":\"#0000ff\"}}]}";

static mln_buffer_view text(const char* string) {
  return (mln_buffer_view){.data = string, .size = strlen(string)};
}

int main(void) {
  @autoreleasepool {
    static const uint8_t red[4] = {0, 0, 255, 255};
    static const uint8_t blue[4] = {255, 0, 0, 255};

    mln_runtime runtime = MLN_HANDLE_NULL;
    const mln_runtime_options runtime_options = {.size = sizeof(mln_runtime_options)};
    CHECK_STATUS(MLN_STATUS_OK, mln_runtime_create(&runtime_options, &runtime, D));

    waiter w;
    mln_completion c = completion_for(&w);
    mln_map map = MLN_HANDLE_NULL;
    const mln_map_options map_options = {
      .size = sizeof(mln_map_options),
      .initial_extent = {.width = 64, .height = 64, .scale_factor = 2.0},
      .map_mode = MLN_MAP_MODE_CONTINUOUS,
    };
    CHECK_STATUS(MLN_STATUS_OK, mln_runtime_create_map(runtime, &map_options, &map, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_map_set_style_json(map, text(red_style), &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);

    CaptureLayer* layer = make_layer(64, 64, 2.0);
    mln_metal_surface_descriptor descriptor = {
      .size = sizeof(mln_metal_surface_descriptor),
      .logical_size = {.width = 64, .height = 64},
      .layer = (__bridge void*)layer,
    };
    mln_render_session_attach_options options = {.size = sizeof(mln_render_session_attach_options)};
    options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
    options.frame_wake.callback = count_wake;

    // A caller-driven Metal surface is not supported.
    mln_render_session session = MLN_HANDLE_NULL;
    options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_UNSUPPORTED, mln_map_attach_metal_surface(map, &descriptor, &options, &session, &c, D));
    CHECK(session == MLN_HANDLE_NULL);

    options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_map_attach_metal_surface(map, &descriptor, &options, &session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    CHECK(layer.drawableSize.width == 128 && layer.drawableSize.height == 128);

    mln_render_session_capabilities capabilities = {.size = sizeof(capabilities)};
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_get_capabilities(session, &capabilities, D));
    CHECK(capabilities.driver == MLN_RENDER_DRIVER_CORE_WORKER);
    CHECK(capabilities.flags == MLN_RENDER_SESSION_CAPABILITY_PRESENTATION);

    // The first frame draws the red style on the layer.
    mln_render_frame_result result = demand(session, MLN_FRAME_DEMAND_FORCE);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
    CHECK(result.mode == MLN_RENDER_MODE_FULL);
    CHECK(result.view.extent.width == 64 && result.view.extent.scale_factor == 2.0);
    CHECK(atomic_load(&frame_wakes) > 0);
    CHECK(drawable_is(layer, red, MTLSizeMake(128, 128, 1)));

    // Nothing changed, so a demand without flags draws nothing.
    result = demand(session, 0);
    CHECK(result.disposition == MLN_RENDER_RESULT_NO_UPDATE);

    // A style change reaches the layer with a demand that waits for it.
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_map_set_style_json(map, text(blue_style), &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    result = demand(session, MLN_FRAME_DEMAND_WAIT_FOR_UPDATE);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
    CHECK(drawable_is(layer, blue, MTLSizeMake(128, 128, 1)));

    // A camera animation renders a frame per demand, as a display link would.
    mln_camera_update camera = {.size = sizeof(mln_camera_update), .mode = MLN_CAMERA_UPDATE_MODE_EASE};
    camera.camera = (mln_camera_options){.size = sizeof(mln_camera_options), .fields = MLN_CAMERA_OPTION_ZOOM, .zoom = 3};
    camera.animation = (mln_animation_options){.fields = MLN_ANIMATION_OPTION_DURATION, .duration_ms = 200};
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_map_update_camera(map, &camera, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    int rendered = 0;
    int repainting = 0;
    double last_zoom = 0;
    for (int frame = 0; frame < 40 && last_zoom < 3; frame += 1) {
      result = demand(session, MLN_FRAME_DEMAND_WAIT_FOR_UPDATE);
      rendered += result.disposition == MLN_RENDER_RESULT_RENDERED;
      repainting += result.needs_repaint;
      last_zoom = result.view.camera.zoom;
    }
    CHECK(rendered > 2);
    // The frames of the animation ask for the next one, until the map
    // settles.
    CHECK(repainting > 0);
    for (int frame = 0; frame < 40 && result.needs_repaint; frame += 1) {
      result = demand(session, MLN_FRAME_DEMAND_FORCE);
    }
    CHECK(!result.needs_repaint);
    CHECK(last_zoom == 3);

    // A resize sizes the layer's drawables with the next frame.
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_resize(session, (mln_logical_size){.width = 48, .height = 32}, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    result = demand(session, MLN_FRAME_DEMAND_FORCE);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
    CHECK(result.view.extent.width == 48 && result.view.extent.height == 32);
    CHECK(layer.drawableSize.width == 96 && layer.drawableSize.height == 64);
    CHECK(drawable_is(layer, blue, MTLSizeMake(96, 64, 1)));

    // A surface presents; it has no texture to read back.
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_UNSUPPORTED, mln_render_session_read_texture(session, &c, D));

    // The renderer answers queries and maintenance.
    const mln_rendered_query_geometry center = {
      .size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
      .data = {.point = {.x = 24, .y = 16}},
    };
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_query_rendered_features(session, &center, NULL, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    CHECK(w.value_count == 0);
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_reduce_memory_use(session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_dump_debug_logs(session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_clear_data(session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    result = demand(session, MLN_FRAME_DEMAND_FORCE);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);

    // After a release, the session draws nothing until a new layer.
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_release_target(session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_get_snapshot(session, &snapshot, D));
    CHECK(!snapshot.has_target);
    result = demand(session, MLN_FRAME_DEMAND_FORCE);
    CHECK(result.disposition == MLN_RENDER_RESULT_TARGET_NOT_READY);

    CaptureLayer* next = make_layer(32, 32, 2.0);
    const mln_metal_surface_replacement replacement = {
      .size = sizeof(mln_metal_surface_replacement),
      .logical_size = {.width = 32, .height = 32},
      .layer = (__bridge void*)next,
    };
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_set_metal_surface_target(session, &replacement, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    CHECK(next.drawableSize.width == 64 && next.drawableSize.height == 64);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_get_snapshot(session, &snapshot, D));
    CHECK(snapshot.has_target);
    CHECK(snapshot.extent.width == 32 && snapshot.extent.height == 32);
    result = demand(session, MLN_FRAME_DEMAND_FORCE);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
    CHECK(drawable_is(next, blue, MTLSizeMake(64, 64, 1)));

    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_detach(session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    CHECK_STATUS(MLN_STATUS_INVALID_STATE, mln_render_session_demand_frame(
                                             session, &(mln_frame_demand){.size = sizeof(mln_frame_demand), .token = 1}, D));
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_dispose(session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);

    // A second session attaches to the same map, and the map's disposal
    // abandons it.
    session = MLN_HANDLE_NULL;
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_map_attach_metal_surface(map, &descriptor, &options, &session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
    result = demand(session, MLN_FRAME_DEMAND_FORCE);
    CHECK(result.disposition == MLN_RENDER_RESULT_RENDERED);
    CHECK_STATUS(MLN_STATUS_OK, mln_map_dispose(map, D));
    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_render_session_dispose(session, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);

    c = completion_for(&w);
    CHECK_STATUS(MLN_STATUS_OK, mln_runtime_release(runtime, &c, D));
    CHECK(finish(&w) == MLN_STATUS_OK);
  }
  if (failures != 0) {
    fprintf(stderr, "%d checks failed\n", failures);
    return 1;
  }
  printf("metal surface test passed\n");
  return 0;
}
