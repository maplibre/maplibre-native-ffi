// Raw C ABI coverage: MapLibre Native's plugin registration reaches the
// library's exports, and a layer type registered through it renders through
// the C API on every backend the plugin API declares shaders for.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "abi_tests.h"
#include "maplibre_native_c/plugin.h"
#include "test_support.h"
#include "unity.h"

#if defined(__EMSCRIPTEN__)
#include "plugin_square.h"
#endif

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

static void plugin_load_library_rejects_bad_arguments(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_plugin_load_library(
      MLN_BUFFER_LITERAL("plugin\0suffix"), MLN_BUFFER_LITERAL("register")
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_plugin_load_library(
      MLN_BUFFER_LITERAL("plugin"), MLN_BUFFER_LITERAL("register\0suffix")
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_plugin_load_library(
      MLN_BUFFER_LITERAL(""), MLN_BUFFER_LITERAL("mln_test_plugin_register")
    )
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(mln_thread_last_error_message()));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_plugin_load_library(
      MLN_BUFFER_LITERAL("/nonexistent/libmaplibre-test-plugin.so"),
      MLN_BUFFER_LITERAL("")
    )
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(mln_thread_last_error_message()));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_plugin_load_library(
      (mln_buffer_view){.data = NULL, .size = 3},
      MLN_BUFFER_LITERAL("mln_test_plugin_register")
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NATIVE_ERROR,
    mln_plugin_load_library(
      MLN_BUFFER_LITERAL("/nonexistent/libmaplibre-test-plugin.so"),
      MLN_BUFFER_LITERAL("mln_test_plugin_register")
    )
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(mln_thread_last_error_message()));
}

static void register_square_plugin(void) {
  TEST_ASSERT_TRUE(
    mln_plugin_get_register_function_v1() == &mln_plugin_register_v1
  );
#if defined(__EMSCRIPTEN__)
  // Browser builds link plugins statically; the module has no dynamic linker.
  char error[256] = "";
  TEST_ASSERT_EQUAL_INT(
    MLN_PLUGIN_STATUS_OK, mln_plugin_get_register_function_v1()(
                            &square_descriptor, error, sizeof(error)
                          )
  );
  TEST_ASSERT_EQUAL_STRING("", error);
#else
  const char* path = getenv("MLN_FFI_TEST_PLUGIN_PATH");
  if (path == NULL) path = MLN_FFI_TEST_PLUGIN_PATH;
  const mln_buffer_view library = {path, strlen(path)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NATIVE_ERROR,
    mln_plugin_load_library(library, MLN_BUFFER_LITERAL("missing_entry_point"))
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(mln_thread_last_error_message()));
  for (int load = 0; load < 2; ++load) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK,
      mln_plugin_load_library(
        library, MLN_BUFFER_LITERAL("mln_test_plugin_register")
      ),
      mln_thread_last_error_message()
    );
  }
#endif
}

#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)

// The plugin API declares shaders for OpenGL, Vulkan, and Metal, so a WebGPU
// build registers a plugin but has no shader path to render its layers.
static void registration_reaches_the_library_exports(void) {
  register_square_plugin();
}

#else

static void a_registered_layer_type_renders_through_the_c_api(void) {
  register_square_plugin();

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(square_style_json))
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  // The source tiles on a worker, so the first rendered frames show only the
  // background. Render until the square lands on the center pixel.
  static uint8_t pixels[64 * 64 * 4];
  const uint8_t* center = pixels + ((32 * 64) + 32) * 4;
  bool square_rendered = false;
  for (unsigned int attempt = 0; attempt < 2000 && !square_rendered;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0, -1));
    mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
    bool needs_repaint = false;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_render_session_render_update(fixture.session, &result, &needs_repaint)
    );
    if (result == MLN_RENDER_RESULT_RENDERED) {
      mln_texture_image_info info = {.size = sizeof(mln_texture_image_info)};
      TEST_ASSERT_EQUAL_INT(
        MLN_STATUS_OK, mln_texture_read_premultiplied_rgba8(
                         fixture.session, pixels, sizeof(pixels), &info
                       )
      );
      TEST_ASSERT_EQUAL_UINT32(64, info.width);
      TEST_ASSERT_EQUAL_UINT32(64, info.height);
      square_rendered = center[1] == 255;
    }
    if (!square_rendered) {
      mln_test_sleep_millisecond();
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

void run_plugin_layer_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(plugin_load_library_rejects_bad_arguments);
#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  RUN_TEST(registration_reaches_the_library_exports);
#else
  RUN_TEST(a_registered_layer_type_renders_through_the_c_api);
#endif
}
