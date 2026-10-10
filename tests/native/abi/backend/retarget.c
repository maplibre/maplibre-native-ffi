// Retargeting a session: a borrowed texture or surface that the host replaces
// through its backend's set_target, and the replacements a session refuses.
// Every refusal leaves the session rendering into its old target.
//
// The targets come from tests/graphics for the preset's backend, so the
// browser presets, whose canvases JavaScript owns, have no cases here.

#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

#if !defined(__EMSCRIPTEN__)

#if defined(MLN_FFI_TEST_BACKEND_METAL)
#include <objc/message.h>
#include <objc/runtime.h>
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_METAL
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_VULKAN
#elif defined(MLN_FFI_TEST_OPENGL_WGL)
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_WGL
#define HOST_OPENGL 1
#else
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_EGL
#define HOST_OPENGL 1
#endif

enum { pixel_count = MLN_TEST_HOST_TARGET_SIZE * MLN_TEST_HOST_TARGET_SIZE };

static const uint8_t red[4] = {255, 0, 0, 255};
static const uint8_t blue[4] = {0, 0, 255, 255};

// Creating or reading from another graphics object can leave its context
// current, and an OpenGL fixture's driver needs its own.
static void restore_fixture_context(const mln_test_render_fixture* fixture) {
#if defined(HOST_OPENGL)
  TEST_ASSERT_TRUE(
    mln_test_graphics_make_current(mln_test_render_fixture_graphics(fixture))
  );
#else
  (void)fixture;
#endif
}

// A surface replacement leaves the new surface configured for the session. A
// Metal layer reports the drawable size the session gave it, so the case first
// shrinks the layer tests/graphics sized to the target; the other backends'
// surfaces report nothing a host can read.
#if defined(MLN_FFI_TEST_BACKEND_METAL)
typedef struct layer_size {
  double width;
  double height;
} layer_size;

static id metal_layer(const mln_test_graphics_surface* surface) {
  mln_test_graphics_surface_info info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_surface_get_info(surface, &info));
  return (id)info.metal_layer;
}

static layer_size drawable_size(const mln_test_graphics_surface* surface) {
  return ((layer_size (*)(id, SEL))objc_msgSend)(
    metal_layer(surface), sel_registerName("drawableSize")
  );
}
#endif

static void unconfigure_surface(const mln_test_graphics_surface* surface) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  ((void (*)(id, SEL, layer_size))objc_msgSend)(
    metal_layer(surface), sel_registerName("setDrawableSize:"),
    (layer_size){1.0, 1.0}
  );
  TEST_ASSERT_EQUAL_INT(1, (int)drawable_size(surface).width);
#else
  (void)surface;
#endif
}

static void expect_surface_configured(
  const mln_test_graphics_surface* surface
) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  const layer_size size = drawable_size(surface);
  TEST_ASSERT_EQUAL_INT(MLN_TEST_HOST_TARGET_SIZE, (int)size.width);
  TEST_ASSERT_EQUAL_INT(MLN_TEST_HOST_TARGET_SIZE, (int)size.height);
#else
  (void)surface;
#endif
}

static void finish(
  const mln_test_render_fixture* fixture, mln_test_completion* completion,
  mln_status expected, const char* what
) {
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    expected, mln_test_render_fixture_finish_operation(fixture, completion),
    what
  );
  mln_test_completion_destroy(completion);
}

// Repaints the red style's background blue, with no transition, so the next
// frame shows the change.
static void paint_background_blue(mln_map map) {
  MLN_TEST_AWAIT_OK(mln_map_set_layer_property(
    map, MLN_BUFFER_LITERAL("bg"),
    MLN_BUFFER_LITERAL("background-color-transition"),
    MLN_BUFFER_LITERAL("{\"duration\":0}"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_layer_property(
    map, MLN_BUFFER_LITERAL("bg"), MLN_BUFFER_LITERAL("background-color"),
    MLN_BUFFER_LITERAL("\"#0000ff\""), &completion.descriptor, NULL
  ));
}

// Renders one frame and, once the session is idle, returns its disposition.
static uint32_t render_frame(
  const mln_test_render_fixture* fixture, uint32_t flags
) {
  static uint64_t next_token = 1;
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = flags;
  demand.token = next_token++;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  mln_test_completion barrier = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_barrier(fixture->session, &barrier.descriptor, NULL)
  );
  finish(fixture, &barrier, MLN_STATUS_OK, "the render barrier");
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture->session, &batch, NULL)
  );
  size_t count = 0;
  MLN_TEST_OK(mln_render_frame_batch_count(batch, &count, NULL));
  uint32_t disposition = UINT32_MAX;
  for (size_t index = 0; index < count; index += 1) {
    mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
    MLN_TEST_OK(mln_render_frame_batch_get(batch, index, &result, NULL));
    if (result.token == demand.token) {
      disposition = result.disposition;
    }
  }
  mln_render_frame_batch_release(batch);
  return disposition;
}

static size_t count_color(const uint8_t* pixels, const uint8_t color[4]) {
  size_t matching = 0;
  for (size_t index = 0; index < pixel_count; index += 1) {
    matching += memcmp(&pixels[index * 4], color, 4) == 0;
  }
  return matching;
}

// Every pixel of `texture` holds `color`.
static void expect_texture_color(
  const mln_test_render_fixture* fixture, mln_test_graphics_texture* texture,
  const uint8_t color[4], const char* what
) {
  static uint8_t pixels[pixel_count * 4];
  const bool read =
    texture == NULL
      ? mln_test_render_fixture_read_texture(fixture, pixels, sizeof(pixels))
      : mln_test_graphics_texture_read_rgba8(texture, pixels, sizeof(pixels));
  restore_fixture_context(fixture);
  TEST_ASSERT_TRUE_MESSAGE(read, mln_test_graphics_last_error());
  TEST_ASSERT_EQUAL_size_t_MESSAGE(
    pixel_count, count_color(pixels, color), what
  );
}

// The center pixel of a session-owned texture's latest frame holds `color`.
static void expect_owned_color(
  const mln_test_render_fixture* fixture, const uint8_t color[4],
  const char* what
) {
  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_texture_read_premultiplied_rgba8(
    fixture->session, &readback.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &readback));
  mln_texture_readback_result result = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &result, sizeof(result))
  );
  const size_t offset = (size_t)(result.info.height / 2) * result.info.stride +
                        (size_t)(result.info.width / 2) * 4;
  TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(
    color, (const uint8_t*)result.data.data + offset, 4, what
  );
  mln_test_completion_destroy(&readback);
}

typedef struct retarget_map {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture;
} retarget_map;

enum target_kind { TARGET_OWNED, TARGET_BORROWED, TARGET_SURFACE };

static void open_map(enum target_kind kind, retarget_map* out) {
  out->runtime = mln_test_create_runtime();
  out->map = mln_test_create_map(out->runtime);
  mln_test_load_style_and_wait(
    out->runtime, out->map, mln_test_red_background_style_json
  );
  bool attached = false;
  switch (kind) {
    case TARGET_OWNED:
      attached = mln_test_render_fixture_create(out->map, &out->fixture);
      break;
    case TARGET_BORROWED:
      attached = mln_test_render_fixture_create_borrowed_texture(
        out->map, &out->fixture
      );
      break;
    case TARGET_SURFACE:
      attached =
        mln_test_render_fixture_create_surface(out->map, &out->fixture);
      break;
  }
  TEST_ASSERT_TRUE_MESSAGE(attached, mln_test_graphics_last_error());
}

static void close_map(retarget_map* map) {
  mln_test_render_fixture_destroy(&map->fixture);
  mln_test_destroy_map(map->map);
  mln_test_destroy_runtime(map->runtime);
}

static void a_borrowed_texture_retarget_renders_into_the_new_texture(void) {
  retarget_map map = {0};
  open_map(TARGET_BORROWED, &map);
  const mln_test_render_fixture* fixture = &map.fixture;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, 0)
  );
  expect_texture_color(fixture, NULL, red, "the first texture");

  mln_test_graphics_texture* replacement =
    mln_test_render_fixture_new_texture(fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    mln_test_render_fixture_set_textures(
      fixture, mln_test_render_fixture_graphics(fixture), &replacement, 1,
      &completion.descriptor
    ),
    mln_test_last_error()
  );
  finish(fixture, &completion, MLN_STATUS_OK, "the texture replacement");

  // The frame after the swap lands in the replacement alone.
  paint_background_blue(map.map);
  MLN_TEST_OK(mln_test_runtime_barrier(map.runtime));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, 0)
  );
  expect_texture_color(fixture, replacement, blue, "the replacement");
  expect_texture_color(fixture, NULL, red, "the replaced texture");
  close_map(&map);
}

typedef struct demand_wait {
  const mln_test_render_fixture* fixture;
  uint64_t token;
  bool published;
} demand_wait;

// Services one driver work item at a time until the demand's result is
// published, so the driver stops right after the item that published it.
static bool service_until_published(void* context) {
  demand_wait* wait = context;
  size_t serviced = 0;
  MLN_TEST_OK(mln_render_session_service_driver_work(
    wait->fixture->session, 1, &serviced, NULL
  ));
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  if (
    mln_render_session_drain_frame_results(
      wait->fixture->session, &batch, NULL
    ) == MLN_STATUS_OK
  ) {
    size_t count = 0;
    MLN_TEST_OK(mln_render_frame_batch_count(batch, &count, NULL));
    for (size_t index = 0; index < count; index += 1) {
      mln_render_frame_result result = {
        .size = sizeof(mln_render_frame_result)
      };
      MLN_TEST_OK(mln_render_frame_batch_get(batch, index, &result, NULL));
      if (result.token == wait->token) {
        TEST_ASSERT_EQUAL_UINT32(
          MLN_RENDER_RESULT_RENDERED, result.disposition
        );
        wait->published = true;
      }
    }
    mln_render_frame_batch_release(batch);
  }
  return wait->published;
}

static void open_borrowed_ring(retarget_map* out, uint32_t driver) {
  out->runtime = mln_test_create_runtime();
  out->map = mln_test_create_map(out->runtime);
  mln_test_load_style_and_wait(
    out->runtime, out->map, mln_test_red_background_style_json
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_ring(
      out->map, &out->fixture, 2, driver
    ),
    mln_test_graphics_last_error()
  );
}

static mln_status acquire(
  const mln_test_render_fixture* fixture, mln_acquired_frame* out_frame
) {
  *out_frame = MLN_HANDLE_NULL;
  return mln_render_session_acquire_frame(fixture->session, out_frame, NULL);
}

static void release(mln_acquired_frame* frame) {
  const mln_gpu_sync sync = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(frame, &sync, NULL));
}

// A replacement is refused while the host holds a frame of the ring, because
// an acquired frame names a texture that the replacement retires.
static void a_borrowed_retarget_is_refused_while_a_frame_is_acquired(void) {
  retarget_map map = {0};
  open_borrowed_ring(&map, MLN_TEST_PRESET_DRIVER);
  const mln_test_render_fixture* fixture = &map.fixture;
  mln_test_graphics_texture* replacements[2] = {
    mln_test_render_fixture_new_texture(fixture),
    mln_test_render_fixture_new_texture(fixture),
  };
  TEST_ASSERT_NOT_NULL_MESSAGE(replacements[0], mln_test_graphics_last_error());
  TEST_ASSERT_NOT_NULL_MESSAGE(replacements[1], mln_test_graphics_last_error());
  restore_fixture_context(fixture);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, 0)
  );
  mln_acquired_frame held = MLN_HANDLE_NULL;
  MLN_TEST_OK(acquire(fixture, &held));

  mln_test_completion refused = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_test_render_fixture_set_textures(
      fixture, mln_test_render_fixture_graphics(fixture), replacements, 2,
      &refused.descriptor
    )
  );
  TEST_ASSERT_FALSE(mln_test_completion_poll(&refused));
  mln_test_completion_reject(&refused);
  mln_test_completion_destroy(&refused);

  release(&held);
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    mln_test_render_fixture_set_textures(
      fixture, mln_test_render_fixture_graphics(fixture), replacements, 2,
      &completion.descriptor
    ),
    mln_test_last_error()
  );
  finish(fixture, &completion, MLN_STATUS_OK, "the ring replacement");
  close_map(&map);
}

// A frame disposed without consumer synchronization quarantines its slot, since
// the host's GPU may still read the slot's texture. A replacement retires those
// textures, so it returns every slot to service, even in a ring that refused
// frames because each of its slots was quarantined.
static void a_borrowed_retarget_returns_quarantined_slots_to_service(void) {
  retarget_map map = {0};
  open_borrowed_ring(&map, MLN_TEST_PRESET_DRIVER);
  const mln_test_render_fixture* fixture = &map.fixture;
  mln_test_graphics_texture* replacements[2] = {
    mln_test_render_fixture_new_texture(fixture),
    mln_test_render_fixture_new_texture(fixture),
  };
  TEST_ASSERT_NOT_NULL_MESSAGE(replacements[0], mln_test_graphics_last_error());
  TEST_ASSERT_NOT_NULL_MESSAGE(replacements[1], mln_test_graphics_last_error());
  restore_fixture_context(fixture);
  for (int index = 0; index < 2; index += 1) {
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(fixture, 0)
    );
    mln_acquired_frame frame = MLN_HANDLE_NULL;
    MLN_TEST_OK(acquire(fixture, &frame));
    MLN_TEST_OK(mln_acquired_frame_dispose(frame, NULL));
  }
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = 2000;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );

  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    mln_test_render_fixture_set_textures(
      fixture, mln_test_render_fixture_graphics(fixture), replacements, 2,
      &completion.descriptor
    ),
    mln_test_last_error()
  );
  finish(fixture, &completion, MLN_STATUS_OK, "the ring replacement");

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, 0)
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_OK(acquire(fixture, &frame));
  uint32_t slot = UINT32_MAX;
  const uint64_t texture = mln_test_frame_texture_handle(frame, &slot);
  TEST_ASSERT_LESS_THAN_UINT32(2, slot);
  TEST_ASSERT_EQUAL_UINT64(
    mln_test_texture_handle(replacements[slot]), texture
  );
  release(&frame);
  close_map(&map);
}

// No frame published before a replacement can be acquired, not even one that
// a demand queued ahead of the replacement renders into a texture it
// replaces. A caller driver lets the case stop the driver between that demand
// and the replacement.
static void frames_published_before_a_retarget_cannot_be_acquired(void) {
  retarget_map map = {0};
  open_borrowed_ring(&map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  const mln_test_render_fixture* fixture = &map.fixture;
  mln_test_graphics_texture* replacements[2] = {
    mln_test_render_fixture_new_texture(fixture),
    mln_test_render_fixture_new_texture(fixture),
  };
  TEST_ASSERT_NOT_NULL_MESSAGE(replacements[0], mln_test_graphics_last_error());
  TEST_ASSERT_NOT_NULL_MESSAGE(replacements[1], mln_test_graphics_last_error());
  restore_fixture_context(fixture);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, 0)
  );

  // Forced, since the map may have published nothing newer than the frame
  // above.
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = 1000;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    mln_test_render_fixture_set_textures(
      fixture, mln_test_render_fixture_graphics(fixture), replacements, 2,
      &completion.descriptor
    ),
    mln_test_last_error()
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_NOT_READY, acquire(fixture, &frame));

  demand_wait wait = {.fixture = fixture, .token = demand.token};
  TEST_ASSERT_TRUE(mln_test_await(
    service_until_published, &wait, mln_test_deadline_default(),
    "the demand queued ahead of the replacement"
  ));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&completion));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_NOT_READY, acquire(fixture, &frame));
  finish(fixture, &completion, MLN_STATUS_OK, "the ring replacement");
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_NOT_READY, acquire(fixture, &frame));

  // The first frame after the replacement lands in a new texture.
  paint_background_blue(map.map);
  MLN_TEST_OK(mln_test_runtime_barrier(map.runtime));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, 0)
  );
  MLN_TEST_OK(acquire(fixture, &frame));
  uint32_t slot = UINT32_MAX;
  const uint64_t texture = mln_test_frame_texture_handle(frame, &slot);
  TEST_ASSERT_LESS_THAN_UINT32(2, slot);
  TEST_ASSERT_EQUAL_UINT64(
    mln_test_texture_handle(replacements[slot]), texture
  );
  expect_texture_color(fixture, replacements[slot], blue, "the new texture");
  release(&frame);
  close_map(&map);
}

// A replacement holds no frame of the map's latest update, so a demand that
// waits for an update renders into it with no update to wait for.
static void a_borrowed_retarget_renders_a_waiting_demand(void) {
  retarget_map map = {0};
  open_map(TARGET_BORROWED, &map);
  const mln_test_render_fixture* fixture = &map.fixture;
  mln_test_render_until_idle(map.runtime, fixture);
  mln_test_graphics_texture* replacement =
    mln_test_render_fixture_new_texture(fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  restore_fixture_context(fixture);

  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  demand.token = 3000;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  // The fence runs after the driver evaluated the demand, which then waits.
  // Dumping debug logs publishes no map update, which would end the wait.
  mln_test_completion fence = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_dump_debug_logs(
    fixture->session, &fence.descriptor, NULL
  ));
  finish(fixture, &fence, MLN_STATUS_OK, "the fence");
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_READY,
    mln_render_session_drain_frame_results(fixture->session, &batch, NULL)
  );

  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    mln_test_render_fixture_set_textures(
      fixture, mln_test_render_fixture_graphics(fixture), &replacement, 1,
      &completion.descriptor
    ),
    mln_test_last_error()
  );
  finish(fixture, &completion, MLN_STATUS_OK, "the texture replacement");
  batch = mln_test_render_wait_for_results(fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(3000, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  expect_texture_color(fixture, replacement, red, "the replacement");
  close_map(&map);
}

static void a_surface_retarget_presents_through_the_new_surface(void) {
  retarget_map map = {0};
  open_map(TARGET_SURFACE, &map);
  const mln_test_render_fixture* fixture = &map.fixture;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, MLN_FRAME_DEMAND_PRESENT)
  );

  mln_test_graphics_surface* replacement =
    mln_test_render_fixture_new_surface(fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  unconfigure_surface(replacement);
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    mln_test_render_fixture_set_surface(
      fixture, mln_test_render_fixture_graphics(fixture), replacement,
      &completion.descriptor
    ),
    mln_test_last_error()
  );
  finish(fixture, &completion, MLN_STATUS_OK, "the surface replacement");
  expect_surface_configured(replacement);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, MLN_FRAME_DEMAND_PRESENT)
  );
  close_map(&map);
}

// Where a replacement comes from.
enum target_origin {
  // The graphics object the session attached with.
  ORIGIN_SESSION,
  // A second graphics object of the preset's backend, with its own device or
  // context.
  ORIGIN_FOREIGN,
  // The session's Metal device, in a pixel format other than the session's.
  ORIGIN_OTHER_FORMAT,
};

typedef struct refusal_row {
  const char* label;
  enum target_kind session;
  enum target_kind replacement;
  enum target_origin origin;
  // The submission's status, and when that is OK, the completion's. The
  // driver compares a replacement's context and format with the session's
  // own, so those refusals arrive as completions, as each set_target's
  // Completes-with list says.
  mln_status submission;
  mln_status completion;
  const char* diagnostic;
} refusal_row;

static const refusal_row refusal_rows[] = {
  {"a session-owned texture takes no borrowed texture", TARGET_OWNED,
   TARGET_BORROWED, ORIGIN_SESSION, MLN_STATUS_UNSUPPORTED, MLN_STATUS_OK,
   "session-owned texture"},
  {"a session-owned texture takes no surface", TARGET_OWNED, TARGET_SURFACE,
   ORIGIN_SESSION, MLN_STATUS_UNSUPPORTED, MLN_STATUS_OK, "native surface"},
  {"a borrowed texture takes no surface", TARGET_BORROWED, TARGET_SURFACE,
   ORIGIN_SESSION, MLN_STATUS_UNSUPPORTED, MLN_STATUS_OK, "native surface"},
  {"a surface takes no texture", TARGET_SURFACE, TARGET_BORROWED,
   ORIGIN_SESSION, MLN_STATUS_UNSUPPORTED, MLN_STATUS_OK,
   "caller-owned texture"},
// One Metal device serves the host, so a second graphics object would share
// it. A Metal session refuses a texture of another pixel format instead.
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  {"a texture of another pixel format", TARGET_BORROWED, TARGET_BORROWED,
   ORIGIN_OTHER_FORMAT, MLN_STATUS_OK, MLN_STATUS_UNSUPPORTED, "pixel format"},
#else
  {"a texture from another context", TARGET_BORROWED, TARGET_BORROWED,
   ORIGIN_FOREIGN, MLN_STATUS_OK, MLN_STATUS_INVALID_ARGUMENT,
   "this session attached with"},
  {"a surface from another context", TARGET_SURFACE, TARGET_SURFACE,
   ORIGIN_FOREIGN, MLN_STATUS_OK, MLN_STATUS_INVALID_ARGUMENT,
   "this session attached with"},
#endif
};

typedef struct replacement {
  mln_test_graphics* foreign;
  mln_test_graphics* graphics;
  mln_test_graphics_texture* texture;
  mln_test_graphics_surface* surface;
  // An id<MTLTexture> the case made itself.
  void* metal_texture;
} replacement;

#if defined(MLN_FFI_TEST_BACKEND_METAL)
// MTLPixelFormatRGBA8Unorm, where tests/graphics makes BGRA8Unorm textures.
enum { metal_rgba8_unorm = 70 };

// Makes a render-target texture in RGBA order on the session's device.
static void* new_rgba_texture(const mln_test_render_fixture* fixture) {
  mln_test_graphics_context context = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(
    mln_test_render_fixture_graphics(fixture), &context
  ));
  id descriptor = ((
    id (*)(Class, SEL, unsigned long, unsigned long, unsigned long, bool)
  )objc_msgSend)(
    objc_getClass("MTLTextureDescriptor"),
    sel_registerName(
      "texture2DDescriptorWithPixelFormat:width:height:"
      "mipmapped:"
    ),
    metal_rgba8_unorm, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE,
    false
  );
  // MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget.
  ((void (*)(id, SEL, unsigned long))objc_msgSend)(
    descriptor, sel_registerName("setUsage:"), 5
  );
  return ((void* (*)(id, SEL, id))objc_msgSend)(
    (id)context.metal_device, sel_registerName("newTextureWithDescriptor:"),
    descriptor
  );
}
#endif

static replacement make_replacement(
  const refusal_row* row, const mln_test_render_fixture* fixture
) {
  replacement made = {.graphics = mln_test_render_fixture_graphics(fixture)};
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  if (row->origin == ORIGIN_OTHER_FORMAT) {
    made.metal_texture = new_rgba_texture(fixture);
    TEST_ASSERT_NOT_NULL(made.metal_texture);
    return made;
  }
#endif
  if (row->origin == ORIGIN_FOREIGN) {
    made.foreign = mln_test_graphics_create(HOST_BACKEND);
    TEST_ASSERT_NOT_NULL_MESSAGE(made.foreign, mln_test_graphics_last_error());
    made.graphics = made.foreign;
    if (row->replacement == TARGET_SURFACE) {
      made.surface = mln_test_graphics_surface_create(
        made.foreign, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
      );
    } else {
      made.texture = mln_test_graphics_texture_create(
        made.foreign, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
      );
    }
    restore_fixture_context(fixture);
  } else if (row->replacement == TARGET_SURFACE) {
    made.surface = mln_test_render_fixture_new_surface(fixture);
  } else {
    made.texture = mln_test_render_fixture_new_texture(fixture);
  }
  TEST_ASSERT_TRUE_MESSAGE(
    made.texture != NULL || made.surface != NULL, mln_test_graphics_last_error()
  );
  return made;
}

static void release_replacement(replacement* made) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  if (made->metal_texture != NULL) {
    ((void (*)(id, SEL))objc_msgSend)(
      (id)made->metal_texture, sel_registerName("release")
    );
  }
#endif
  if (made->foreign != NULL) {
    mln_test_graphics_surface_destroy(made->surface);
    mln_test_graphics_texture_destroy(made->texture);
    mln_test_graphics_destroy(made->foreign);
  }
}

// The session still renders into the target it had: its next frame, of a new
// color, lands there.
static void expect_old_target_rendering(
  const refusal_row* row, retarget_map* map
) {
  const mln_test_render_fixture* fixture = &map->fixture;
  paint_background_blue(map->map);
  MLN_TEST_OK(mln_test_runtime_barrier(map->runtime));
  const uint32_t flags =
    row->session == TARGET_SURFACE ? MLN_FRAME_DEMAND_PRESENT : 0;
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    MLN_RENDER_RESULT_RENDERED, render_frame(fixture, flags), row->label
  );
  if (row->session == TARGET_BORROWED) {
    expect_texture_color(fixture, NULL, blue, row->label);
  } else if (row->session == TARGET_OWNED) {
    expect_owned_color(fixture, blue, row->label);
  }
}

// Hands the session a Metal texture that tests/graphics did not make.
static mln_status submit_metal_texture(
  const mln_test_render_fixture* fixture, void* texture,
  const mln_test_completion* completion
) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_metal_borrowed_texture_descriptor descriptor =
    mln_metal_borrowed_texture_descriptor_default();
  descriptor.extent = (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = MLN_TEST_HOST_TARGET_SIZE,
    .height = MLN_TEST_HOST_TARGET_SIZE,
    .scale_factor = 1.0,
  };
  descriptor.physical_width = MLN_TEST_HOST_TARGET_SIZE;
  descriptor.physical_height = MLN_TEST_HOST_TARGET_SIZE;
  const mln_metal_borrowed_texture entry = {.texture = texture};
  descriptor.textures = &entry;
  descriptor.texture_count = 1;
  return mln_metal_borrowed_texture_set_target(
    fixture->session, &descriptor, &completion->descriptor, MLN_TEST_DIAGNOSTIC
  );
#else
  (void)fixture;
  (void)texture;
  (void)completion;
  return MLN_STATUS_UNSUPPORTED;
#endif
}

static void refused_retargets_leave_the_old_target_rendering(void) {
  for (size_t index = 0; index < sizeof(refusal_rows) / sizeof(refusal_rows[0]);
       index += 1) {
    const refusal_row* row = &refusal_rows[index];
    retarget_map map = {0};
    open_map(row->session, &map);
    const mln_test_render_fixture* fixture = &map.fixture;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      MLN_RENDER_RESULT_RENDERED,
      render_frame(
        fixture, row->session == TARGET_SURFACE ? MLN_FRAME_DEMAND_PRESENT : 0
      ),
      row->label
    );

    replacement made = make_replacement(row, fixture);
    mln_test_completion completion = mln_test_completion_default(0);
    const mln_status submitted =
      made.metal_texture != NULL
        ? submit_metal_texture(fixture, made.metal_texture, &completion)
      : made.surface != NULL
        ? mln_test_render_fixture_set_surface(
            fixture, made.graphics, made.surface, &completion.descriptor
          )
        : mln_test_render_fixture_set_textures(
            fixture, made.graphics, &made.texture, 1, &completion.descriptor
          );
    TEST_ASSERT_EQUAL_INT_MESSAGE(row->submission, submitted, row->label);
    const char* diagnostic = mln_test_last_error();
    if (submitted == MLN_STATUS_OK) {
      TEST_ASSERT_EQUAL_INT_MESSAGE(
        row->completion,
        mln_test_render_fixture_finish_operation(fixture, &completion),
        row->label
      );
      diagnostic = mln_test_completion_diagnostic(&completion);
    } else {
      mln_test_completion_reject(&completion);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(diagnostic, row->diagnostic), diagnostic
    );
    mln_test_completion_destroy(&completion);

    expect_old_target_rendering(row, &map);
    close_map(&map);
    release_replacement(&made);
  }
}
#endif

MLN_TEST_GROUP {
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_borrowed_texture_retarget_renders_into_the_new_texture);
  RUN_TEST(a_borrowed_retarget_is_refused_while_a_frame_is_acquired);
  RUN_TEST(a_borrowed_retarget_returns_quarantined_slots_to_service);
  RUN_TEST(frames_published_before_a_retarget_cannot_be_acquired);
  RUN_TEST(a_borrowed_retarget_renders_a_waiting_demand);
  RUN_TEST(a_surface_retarget_presents_through_the_new_surface);
  RUN_TEST(refused_retargets_leave_the_old_target_rendering);
#endif
}
