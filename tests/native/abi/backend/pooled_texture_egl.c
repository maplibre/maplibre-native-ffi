// Owned textures that take turns rendering in one host EGL context.

#include "mln_test_graphics.h"
#include "support/frames.h"
#include "support/test_support.h"

enum { extent = 64, pixel_bytes = extent * extent * 4 };

// Two styles that draw with different programs, so a session that rendered
// with the program another session left bound would draw the wrong layer.
static const char circle_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#203040\"}},"
  "{\"id\":\"dot\",\"type\":\"circle\",\"source\":\"points\","
  "\"paint\":{\"circle-radius\":12,\"circle-color\":\"#ff8000\"}}]}";
static const char fill_style_json[] =
  "{\"version\":8,\"sources\":{\"square\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Polygon\","
  "\"coordinates\":[[[-10,-10],[10,-10],[10,10],[-10,10],[-10,-10]]]},"
  "\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ffffff\"}},"
  "{\"id\":\"square\",\"type\":\"fill\",\"source\":\"square\","
  "\"paint\":{\"fill-color\":\"#0080ff\"}}]}";

typedef struct host_context {
  mln_test_graphics* graphics;
  mln_test_graphics_context handles;
} host_context;

// The context the host owns and keeps current on no thread.
static host_context create_host_context(void) {
  host_context host = {
    .graphics = mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_EGL)
  };
  TEST_ASSERT_NOT_NULL_MESSAGE(host.graphics, mln_test_graphics_last_error());
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(host.graphics, &host.handles));
  return host;
}

static mln_open_gl_context_descriptor pooled_context(const host_context* host) {
  return (mln_open_gl_context_descriptor){
    .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL,
    .ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_POOLED,
    .data = {
      .egl = {
        .display = host->handles.egl_display,
        .config = host->handles.egl_config,
        .share_context = host->handles.egl_context,
        .client_api = MLN_OPEN_GL_CLIENT_API_GLES,
        .get_proc_address = NULL,
      },
    },
  };
}

static mln_test_render_fixture attach_pooled_texture(
  mln_map map, const host_context* host
) {
  mln_test_render_fixture fixture = {.driver = MLN_RENDER_DRIVER_CORE_WORKER};
  mln_open_gl_owned_texture_descriptor descriptor = {
    .size = sizeof(mln_open_gl_owned_texture_descriptor)
  };
  descriptor.extent = (mln_logical_extent){
    .width = extent, .height = extent, .scale_factor = 1.0
  };
  descriptor.context = pooled_context(host);
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = fixture.driver;
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_attach_open_gl_owned_texture(
    map, &descriptor, &options, &fixture.session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
  return fixture;
}

static void detach_pooled_texture(mln_test_render_fixture* fixture) {
  mln_test_completion detach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_detach(
    fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &detach));
  mln_test_completion_destroy(&detach);
  MLN_TEST_OK(mln_test_render_session_dispose(fixture->session));
  *fixture = (mln_test_render_fixture){0};
}

static void read_pixels(
  const mln_test_render_fixture* fixture, uint8_t* pixels
) {
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(fixture, &info, pixels, pixel_bytes));
  TEST_ASSERT_EQUAL_UINT32(extent, info.width);
  TEST_ASSERT_EQUAL_UINT32(extent, info.height);
  TEST_ASSERT_EQUAL_size_t(pixel_bytes, info.byte_length);
}

static void render_style(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture,
  mln_buffer_view style, uint8_t* pixels
) {
  MLN_TEST_OK(mln_test_map_set_style_json(map, style));
  TEST_ASSERT_TRUE(mln_test_render_until_idle(runtime, map, fixture, NULL));
  read_pixels(fixture, pixels);
}

static void render_again(
  const mln_test_render_fixture* fixture, uint64_t token, uint8_t* pixels
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
  read_pixels(fixture, pixels);
}

static void render_dedicated(
  mln_runtime runtime, mln_buffer_view style, uint8_t* pixels
) {
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_dedicated_texture_create(map, &fixture));
  render_style(runtime, map, &fixture, style, pixels);
  mln_test_dedicated_texture_destroy(&fixture);
  mln_test_destroy_map(map);
}

// Each session keeps its own framebuffer and GL objects in the shared context,
// so two maps that take turns render what a context of their own renders.
static void pooled_textures_render_as_dedicated_ones_do(void) {
  static uint8_t expected[2][pixel_bytes];
  static uint8_t actual[pixel_bytes];
  const mln_buffer_view styles[2] = {
    MLN_BUFFER_LITERAL(circle_style_json),
    MLN_BUFFER_LITERAL(fill_style_json),
  };
  mln_runtime runtime = mln_test_create_runtime();
  for (size_t index = 0; index < 2; index += 1) {
    render_dedicated(runtime, styles[index], expected[index]);
  }
  TEST_ASSERT_NOT_EQUAL_INT(0, memcmp(expected[0], expected[1], pixel_bytes));

  host_context host = create_host_context();
  mln_map maps[2];
  mln_test_render_fixture fixtures[2];
  for (size_t index = 0; index < 2; index += 1) {
    maps[index] = mln_test_create_map(runtime);
    fixtures[index] = attach_pooled_texture(maps[index], &host);
  }
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(mln_render_session_get_capabilities(
    fixtures[0].session, &capabilities, NULL
  ));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, capabilities.driver);
  TEST_ASSERT_EQUAL_UINT32(1, capabilities.texture_ring_depth);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_CAPABILITY_READBACK, capabilities.flags
  );

  for (size_t index = 0; index < 2; index += 1) {
    render_style(runtime, maps[index], &fixtures[index], styles[index], actual);
    TEST_ASSERT_EQUAL_MEMORY(expected[index], actual, pixel_bytes);
  }
  uint64_t token = 1;
  for (size_t turn = 0; turn < 3; turn += 1) {
    for (size_t index = 0; index < 2; index += 1) {
      render_again(&fixtures[index], token++, actual);
      TEST_ASSERT_EQUAL_MEMORY(expected[index], actual, pixel_bytes);
    }
  }

  for (size_t index = 0; index < 2; index += 1) {
    detach_pooled_texture(&fixtures[index]);
    mln_test_destroy_map(maps[index]);
  }
  mln_test_graphics_destroy(host.graphics);
  mln_test_destroy_runtime(runtime);
}

// A detached session hands the context to the rest, a session that attaches
// later renders as the first one did, and the host's context outlives them.
static void pooled_textures_detach_while_others_render(void) {
  static uint8_t circle[pixel_bytes];
  static uint8_t fill[pixel_bytes];
  static uint8_t actual[pixel_bytes];
  mln_runtime runtime = mln_test_create_runtime();
  host_context host = create_host_context();
  mln_map first_map = mln_test_create_map(runtime);
  mln_map second_map = mln_test_create_map(runtime);
  mln_test_render_fixture first = attach_pooled_texture(first_map, &host);
  mln_test_render_fixture second = attach_pooled_texture(second_map, &host);
  render_style(
    runtime, first_map, &first, MLN_BUFFER_LITERAL(circle_style_json), circle
  );
  render_style(
    runtime, second_map, &second, MLN_BUFFER_LITERAL(fill_style_json), fill
  );

  detach_pooled_texture(&first);
  mln_test_destroy_map(first_map);
  render_again(&second, 31, actual);
  TEST_ASSERT_EQUAL_MEMORY(fill, actual, pixel_bytes);

  mln_map third_map = mln_test_create_map(runtime);
  mln_test_render_fixture third = attach_pooled_texture(third_map, &host);
  render_style(
    runtime, third_map, &third, MLN_BUFFER_LITERAL(circle_style_json), actual
  );
  TEST_ASSERT_EQUAL_MEMORY(circle, actual, pixel_bytes);
  render_again(&second, 32, actual);
  TEST_ASSERT_EQUAL_MEMORY(fill, actual, pixel_bytes);

  detach_pooled_texture(&second);
  detach_pooled_texture(&third);
  mln_test_destroy_map(third_map);
  mln_test_destroy_map(second_map);
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_graphics_make_current(host.graphics),
    mln_test_graphics_last_error()
  );
  mln_test_graphics_destroy(host.graphics);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(pooled_textures_render_as_dedicated_ones_do);
  RUN_TEST(pooled_textures_detach_while_others_render);
}
