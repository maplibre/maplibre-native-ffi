// Style image options that a copy reads back, and the stretch intervals a
// submission refuses.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const uint8_t transparent_pixels[16] = {0};

static mln_premultiplied_rgba8_image two_by_two_image(void) {
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = transparent_pixels;
  image.byte_length = sizeof(transparent_pixels);
  return image;
}

static mln_style_image_result read_image(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info(
                     map, mln_test_view_of(id), &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_completion_value_count(&completion));
  mln_style_image_result result = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&completion, &result, sizeof(result))
  );
  mln_test_completion_destroy(&completion);
  return result;
}

// A text fit the host names reads back as present, one it leaves out reads
// back as absent, and a stretch copy of an image the style lacks finds none.
static void an_image_keeps_the_text_fit_it_was_set_with(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  const mln_premultiplied_rgba8_image image = two_by_two_image();
  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("fitted"), &image, &options,
                     &completion.descriptor, NULL
                   )
  );
  const mln_style_image_info info = read_image(map, "fitted").info;
  TEST_ASSERT_TRUE(info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info.text_fit_height
  );
  TEST_ASSERT_FALSE(info.has_text_fit_width);

  mln_test_completion stretches =
    mln_test_completion_default(sizeof(mln_style_image_stretches_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_image_stretches(
      map, MLN_BUFFER_LITERAL("missing"), &stretches.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&stretches));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&stretches));
  mln_test_completion_destroy(&stretches);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// MapLibre divides by the summed interval width, so a submission refuses an
// interval of zero width before it reaches the map.
static void a_zero_width_stretch_is_refused_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_premultiplied_rgba8_image image = two_by_two_image();
  const mln_image_stretch empty = {.from = 1.0f, .to = 1.0f};
  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  options.stretch_x = &empty;
  options.stretch_x_count = 1;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "intervals must have a positive width",
    mln_map_set_style_image(
      map, MLN_BUFFER_LITERAL("flat"), &image, &options, &completion.descriptor,
      MLN_TEST_DIAGNOSTIC
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(an_image_keeps_the_text_fit_it_was_set_with);
  RUN_TEST(a_zero_width_stretch_is_refused_at_submission);
}
