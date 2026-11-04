// Offscreen sessions on an EGL device that renders with desktop OpenGL.

#include "support/frames.h"
#include "support/map.h"
#include "support/test_support.h"

// Only desktop EGL on Linux and FreeBSD renders with desktop OpenGL. ANGLE on
// Apple platforms and the EGL of Android and OpenHarmony implement OpenGL ES.
#if (defined(__linux__) || defined(__FreeBSD__)) && !defined(__ANDROID__) && \
  !defined(__OHOS__)
#define DESKTOP_GL_SUPPORTED 1
#endif

#if !defined(DESKTOP_GL_SUPPORTED)
static void a_gles_only_egl_device_rejects_desktop_gl(void) {
  mln_offscreen_device_options options = {
    .size = sizeof(mln_offscreen_device_options),
    .fields = MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API,
    .client_api = MLN_OPEN_GL_CLIENT_API_GL,
  };
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_offscreen_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}

#else
enum { extent = 64, pixel_bytes = extent * extent * 4 };

// A fill draws with a shader program, where a background alone may only clear.
static const char fill_style_json[] =
  "{\"version\":8,\"sources\":{\"square\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Polygon\","
  "\"coordinates\":[[[-10,-10],[10,-10],[10,10],[-10,10],[-10,-10]]]},"
  "\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ffffff\"}},"
  "{\"id\":\"square\",\"type\":\"fill\",\"source\":\"square\","
  "\"paint\":{\"fill-color\":\"#0080ff\"}}]}";

static mln_offscreen_device create_desktop_gl_device(bool single_context) {
  mln_offscreen_device_options options = {
    .size = sizeof(mln_offscreen_device_options)
  };
  options.fields = MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API |
                   MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT;
  options.client_api = MLN_OPEN_GL_CLIENT_API_GL;
  options.single_context = single_context;
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_offscreen_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  return device;
}

static mln_test_render_fixture attach_offscreen_session(
  mln_map map, mln_offscreen_device device
) {
  mln_test_render_fixture fixture = {.driver = MLN_RENDER_DRIVER_CORE_WORKER};
  mln_offscreen_descriptor descriptor = {
    .size = sizeof(mln_offscreen_descriptor)
  };
  descriptor.logical_size =
    (mln_logical_size){.width = extent, .height = extent};
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = fixture.driver;
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_attach_offscreen(
    map, &descriptor, &options, device, &fixture.session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
  return fixture;
}

static void detach_offscreen_session(mln_test_render_fixture* fixture) {
  mln_test_completion detach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_detach(
    fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &detach));
  mln_test_completion_destroy(&detach);
  MLN_TEST_OK(mln_test_render_session_release(fixture->session));
  *fixture = (mln_test_render_fixture){0};
}

static void expect_pixel(
  const uint8_t* pixels, size_t x, size_t y, const uint8_t rgba[4]
) {
  TEST_ASSERT_EQUAL_UINT8_ARRAY(rgba, &pixels[(y * extent + x) * 4], 4);
}

// Renders a still image of the square, and reads the fill at the center and
// the background at a corner.
static void renders_a_still_image(bool single_context) {
  static uint8_t pixels[pixel_bytes];
  static const uint8_t fill[4] = {0x00, 0x80, 0xff, 0xff};
  static const uint8_t background[4] = {0xff, 0xff, 0xff, 0xff};
  mln_offscreen_device device = create_desktop_gl_device(single_context);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options map_options = mln_test_map_options();
  map_options.initial_extent = (mln_logical_extent){
    .width = extent, .height = extent, .scale_factor = 1.0
  };
  map_options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &map_options);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(fill_style_json));
  mln_test_render_fixture fixture = attach_offscreen_session(map, device);

  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(&fixture, &info, pixels, pixel_bytes));
  TEST_ASSERT_EQUAL_UINT32(extent, info.width);
  TEST_ASSERT_EQUAL_UINT32(extent, info.height);
  expect_pixel(pixels, extent / 2, extent / 2, fill);
  expect_pixel(pixels, 0, 0, background);

  detach_offscreen_session(&fixture);
  mln_offscreen_device_release(device);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_desktop_gl_device_renders_a_still_image(void) {
  renders_a_still_image(false);
}

static void a_single_context_desktop_gl_device_renders_a_still_image(void) {
  renders_a_still_image(true);
}
#endif

MLN_TEST_GROUP {
#if !defined(DESKTOP_GL_SUPPORTED)
  RUN_TEST(a_gles_only_egl_device_rejects_desktop_gl);
#else
  RUN_TEST(a_desktop_gl_device_renders_a_still_image);
  RUN_TEST(a_single_context_desktop_gl_device_renders_a_still_image);
#endif
}
