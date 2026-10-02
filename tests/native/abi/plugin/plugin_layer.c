// Raw C ABI coverage: MapLibre Native's plugin registration reaches the
// library's exports, and a layer type registered through it renders through
// the C API on every backend the plugin API declares shaders for. The plugin
// is its own shared library, registered through the function this library
// exports, the way a host loads one.

#include "maplibre_native_c/plugin.h"
#include "square_plugin.h"
#include "support/test_support.h"

// One point at the camera center, which the 64x64 fixture places at (32, 32),
// under a square wide enough to cover the center pixel and no corner.
static const char square_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ff0000\"}},"
  "{\"id\":\"square\",\"type\":\"ffi-test-square\",\"source\":\"points\","
  "\"paint\":{\"square-color\":\"#00ff00\",\"square-radius\":8}}]}";

static mln_plugin_status register_square_plugin(void) {
  char error[256] = "";
  const mln_plugin_status status = mln_native_test_square_plugin_register(
    mln_plugin_get_register_function_v1(), error, sizeof(error)
  );
  TEST_ASSERT_EQUAL_STRING("", error);
  return status;
}

#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)

// The plugin API declares shaders for OpenGL, Vulkan, and Metal, so a WebGPU
// build registers a plugin but has no shader path to render its layers.
static void registration_reaches_the_library_exports(void) {
  TEST_ASSERT_EQUAL_INT(MLN_PLUGIN_STATUS_OK, register_square_plugin());
}

#else

static void a_registered_layer_type_renders_through_the_c_api(void) {
  TEST_ASSERT_EQUAL_INT(MLN_PLUGIN_STATUS_OK, register_square_plugin());

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(square_style_json))
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  // The source tiles on a worker, so the first rendered frames show only the
  // background. Render until the square lands on the center pixel.
  static uint8_t pixels[64 * 64 * 4];
  const uint8_t* center = pixels + ((32 * 64) + 32) * 4;
  bool square_rendered = false;
  const mln_test_deadline deadline = mln_test_deadline_default();
  while (!square_rendered && !mln_test_deadline_passed(deadline)) {
    mln_frame_demand demand = mln_frame_demand_default();
    demand.flags = 0;
    MLN_TEST_OK(
      mln_render_session_request_frame(fixture.session, &demand, NULL)
    );
    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
    );
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    MLN_TEST_OK(
      mln_render_session_drain_frame_results(fixture.session, &batch, NULL)
    );
    mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
    MLN_TEST_OK(mln_render_frame_batch_get(batch, 0, &result, NULL));
    mln_render_frame_batch_release(batch);
    if (result.disposition == MLN_RENDER_RESULT_RENDERED) {
      mln_test_completion readback = mln_test_completion_readback();
      MLN_TEST_OK(mln_texture_read_premultiplied_rgba8(
        fixture.session, &readback.descriptor, NULL
      ));
      MLN_TEST_OK(
        mln_test_render_fixture_finish_operation(&fixture, &readback)
      );
      mln_texture_readback_result image = {0};
      TEST_ASSERT_TRUE(
        mln_test_completion_copy_value(&readback, &image, sizeof(image))
      );
      TEST_ASSERT_EQUAL_UINT32(64, image.info.width);
      TEST_ASSERT_EQUAL_UINT32(64, image.info.height);
      TEST_ASSERT_EQUAL_size_t(sizeof(pixels), image.data.size);
      memcpy(pixels, image.data.data, sizeof(pixels));
      mln_test_completion_destroy(&readback);
      square_rendered = center[1] == 255;
    }
  }
  TEST_ASSERT_TRUE_MESSAGE(
    square_rendered, "the plugin layer never covered the center pixel"
  );
  TEST_ASSERT_EQUAL_UINT8(0, center[0]);
  TEST_ASSERT_EQUAL_UINT8(255, center[1]);
  TEST_ASSERT_EQUAL_UINT8(0, center[2]);
  TEST_ASSERT_EQUAL_UINT8(255, center[3]);
  // The square stops short of the corner, which keeps the background.
  TEST_ASSERT_EQUAL_UINT8(255, pixels[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels[1]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels[2]);
  TEST_ASSERT_EQUAL_UINT8(255, pixels[3]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#endif

MLN_TEST_GROUP {
#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  RUN_TEST(registration_reaches_the_library_exports);
#else
  RUN_TEST(a_registered_layer_type_renders_through_the_c_api);
#endif
}
