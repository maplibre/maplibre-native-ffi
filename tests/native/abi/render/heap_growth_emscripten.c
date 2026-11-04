// A browser module's heap grows past its initial size, and the library keeps
// working in the larger heap. Growth replaces the buffer behind JavaScript's
// views of the heap, so a view that a thread made before the growth covers only
// the smaller heap.

#include <emscripten/heap.h>

#include "support/frames.h"
#include "support/map.h"
#include "support/test_support.h"

enum { side = 64, pixel_bytes = side * side * 4 };

static void render_and_read_back_red(uint8_t* pixels) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options map_options = mln_test_map_options();
  map_options.initial_extent.width = side;
  map_options.initial_extent.height = side;
  map_options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &map_options);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);

  mln_test_render_fixture fixture = {.driver = MLN_RENDER_DRIVER_CORE_WORKER};
  mln_offscreen_descriptor descriptor = {
    .size = sizeof(mln_offscreen_descriptor)
  };
  descriptor.logical_size = (mln_logical_size){.width = side, .height = side};
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_attach_offscreen(
    map, &descriptor, &options, MLN_HANDLE_NULL, &fixture.session,
    &attach.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);

  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(&fixture, &info, pixels, pixel_bytes));
  TEST_ASSERT_EQUAL_size_t(pixel_bytes, info.byte_length);

  MLN_TEST_OK(mln_test_render_session_release(fixture.session));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The runtime, its core worker, and the session's device all start after the
// growth. The pixels go to the end of the held block, past where the heap
// ended before.
static void a_session_renders_after_the_heap_grows(void) {
  // A block as large as the whole heap cannot fit in it.
  const size_t held_size = emscripten_get_heap_size();
  uint8_t* held = malloc(held_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(held, "the heap cannot grow past its size");
  uint8_t* pixels = held + held_size - pixel_bytes;
  if (TEST_PROTECT()) {
    render_and_read_back_red(pixels);
    TEST_ASSERT_EQUAL_UINT8(255, pixels[0]);
    TEST_ASSERT_EQUAL_UINT8(0, pixels[1]);
    TEST_ASSERT_EQUAL_UINT8(0, pixels[2]);
    TEST_ASSERT_EQUAL_UINT8(255, pixels[3]);
  }
  free(held);
}

MLN_TEST_GROUP { RUN_TEST(a_session_renders_after_the_heap_grows); }
