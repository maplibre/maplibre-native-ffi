// Still images: a static or tile map renders one on request through its render
// session, a map in another mode refuses the request, only one request is
// pending at a time, and closing the map cancels the pending one.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "support/harness.h"
#include "support/map.h"
#include "support/test_support.h"
#include "unity.h"

static mln_map create_map_in_mode(mln_runtime runtime, uint32_t map_mode) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = map_mode;
  return mln_test_create_map_with_options(runtime, &options);
}

// Reads back the center pixel of the frame the session rendered last.
static void read_center_pixel(
  const mln_test_render_fixture* fixture, uint8_t out_rgba[4]
) {
  mln_test_completion readback = mln_test_completion_readback();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_texture_read_premultiplied_rgba8(
                     fixture->session, &readback.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(fixture, &readback)
  );
  mln_texture_readback_result image = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &image, sizeof(image))
  );
  TEST_ASSERT_EQUAL_UINT32(64, image.info.width);
  TEST_ASSERT_EQUAL_UINT32(64, image.info.height);
  TEST_ASSERT_EQUAL_size_t((size_t)64 * 64 * 4, image.data.size);
  const uint8_t* pixels = image.data.data;
  memcpy(out_rgba, pixels + ((32 * 64) + 32) * 4, 4);
  mln_test_completion_destroy(&readback);
}

// Each still image renders the map as it stands when the image is taken, so a
// second request after a style change shows the new style.
static void a_still_image_renders_the_current_style(uint32_t map_mode) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, map_mode);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_red_background_style_json
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_still_image(&fixture, map)
  );
  uint8_t rgba[4] = {0};
  read_center_pixel(&fixture, rgba);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[0]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[1]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[2]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[3]);

  static const char green_background_style_json[] =
    "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"bg\","
    "\"type\":\"background\",\"paint\":{\"background-color\":\"#00ff00\"}}]}";
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(green_background_style_json)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_still_image(&fixture, map)
  );
  read_center_pixel(&fixture, rgba);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[0]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[1]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[2]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[3]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_static_map_renders_still_images(void) {
  a_still_image_renders_the_current_style(MLN_MAP_MODE_STATIC);
}

static void a_tile_map_renders_still_images(void) {
  a_still_image_renders_the_current_style(MLN_MAP_MODE_TILE);
}

// A continuous map renders frames on demand and has no still image to give.
// The refused request leaves the completion with the caller.
static void a_continuous_map_refuses_a_still_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, MLN_MAP_MODE_CONTINUOUS);
  mln_test_completion refused = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_map_request_still_image(map, &refused.descriptor, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_last_error(), "map is not in static or tile mode")
  );
  refused.descriptor.release_user_data(refused.descriptor.user_data);
  mln_test_completion_destroy(&refused);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A second request that reaches the map worker while the first is pending
// completes MLN_STATUS_INVALID_STATE, and closing the map cancels the first.
static void map_close_cancels_the_pending_still_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, MLN_MAP_MODE_STATIC);

  // Without a render session the first request stays pending.
  mln_test_completion pending = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_request_still_image(map, &pending.descriptor, NULL)
  );
  mln_test_completion duplicate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_request_still_image(map, &duplicate.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_test_completion_finish(&duplicate)
  );
  mln_test_completion_destroy(&duplicate);

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_CANCELLED, mln_test_completion_finish(&pending)
  );
  mln_test_completion_destroy(&pending);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_static_map_renders_still_images);
  RUN_TEST(a_tile_map_renders_still_images);
  RUN_TEST(a_continuous_map_refuses_a_still_image);
  RUN_TEST(map_close_cancels_the_pending_still_image);
}
