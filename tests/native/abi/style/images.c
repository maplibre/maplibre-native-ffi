// Runtime style images: the metadata, pixels, and stretch intervals a copy
// returns for the image a host set.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

typedef struct stretch_probe {
  atomic_bool done;
  mln_status status;
  mln_image_stretch x;
  mln_image_stretch y;
  size_t x_count;
  size_t y_count;
} stretch_probe;

static void copy_stretches(
  void* user_data, const mln_completion_result* result
) {
  stretch_probe* probe = user_data;
  probe->status = result->status;
  if (result->value_count == 1) {
    const mln_style_image_stretches_result* stretches = result->value;
    probe->x_count = stretches->stretch_x_count;
    probe->y_count = stretches->stretch_y_count;
    if (probe->x_count != 0) probe->x = stretches->stretch_x[0];
    if (probe->y_count != 0) probe->y = stretches->stretch_y[0];
  }
  mln_test_flag_set(&probe->done);
}

static void style_image_stretches_are_borrowed_by_the_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  const mln_image_stretch stretch_x = {.from = 0.0f, .to = 1.0f};
  const mln_image_stretch stretch_y = {.from = 0.0f, .to = 1.0f};
  mln_style_image_options options = mln_style_image_options_default();
  options.fields =
    MLN_STYLE_IMAGE_OPTION_STRETCH_X | MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  options.stretch_x = &stretch_x;
  options.stretch_x_count = 1;
  options.stretch_y = &stretch_y;
  options.stretch_y_count = 1;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("probe-stretches"), &image,
                     &options, &completion.descriptor, NULL
                   )
  );

  stretch_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_stretches,
    .user_data = &probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_image_stretches(
      map, MLN_BUFFER_LITERAL("probe-stretches"), &completion, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_TRUE(atomic_load(&probe.done));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.status);
  TEST_ASSERT_EQUAL_size_t(1, probe.x_count);
  TEST_ASSERT_EQUAL_size_t(1, probe.y_count);
  TEST_ASSERT_EQUAL_MEMORY(&stretch_x, &probe.x, sizeof(stretch_x));
  TEST_ASSERT_EQUAL_MEMORY(&stretch_y, &probe.y, sizeof(stretch_y));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_style_image_info read_image_info(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info(
                     map, (mln_buffer_view){.data = id, .size = strlen(id)},
                     &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_completion_value_count(&completion));
  mln_style_image_result result = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&completion, &result, sizeof(result))
  );
  mln_test_completion_destroy(&completion);
  return result.info;
}

static mln_status copy_pixels(
  mln_map map, const char* id, uint8_t* out, size_t capacity, bool* found
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_premultiplied_rgba8(
                     map, (mln_buffer_view){.data = id, .size = strlen(id)},
                     &completion.descriptor, NULL
                   )
  );
  const mln_status status = mln_test_completion_finish(&completion);
  *found = mln_test_completion_value_count(&completion) == 1;
  if (*found) {
    mln_buffer_view view = {0};
    TEST_ASSERT_TRUE(
      mln_test_completion_copy_value(&completion, &view, sizeof(view))
    );
    TEST_ASSERT_LESS_OR_EQUAL_size_t(capacity, view.size);
    memcpy(out, view.data, view.size);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

// A copy reports the metadata the host set, and returns the pixels tightly
// packed whatever stride they arrived with.
static void style_images_copy_their_metadata_and_packed_pixels(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  // Two rows of two pixels, each row padded to three pixels.
  const uint8_t padded[24] = {
    255, 0, 0,   255, 0,   255, 0,   255, 9, 9, 9, 9,
    0,   0, 255, 255, 128, 128, 128, 255, 9, 9, 9, 9,
  };
  const uint8_t packed[16] = {
    255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 128, 128, 128, 255,
  };
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 12;
  image.pixels = padded;
  image.byte_length = sizeof(padded);
  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO |
                   MLN_STYLE_IMAGE_OPTION_SDF | MLN_STYLE_IMAGE_OPTION_CONTENT |
                   MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
  options.pixel_ratio = 2.0f;
  options.sdf = true;
  options.content = (mln_image_content){
    .left = 0.0f, .top = 0.0f, .right = 2.0f, .bottom = 1.0f
  };
  options.text_fit_width = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("marker"), &image, &options,
                     &completion.descriptor, NULL
                   )
  );

  mln_style_image_info expected = mln_style_image_info_default();
  expected.width = 2;
  expected.height = 2;
  expected.stride = 8;
  expected.byte_length = sizeof(packed);
  expected.content = options.content;
  expected.text_fit_width = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  expected.pixel_ratio = 2.0f;
  expected.sdf = true;
  expected.has_content = true;
  expected.has_text_fit_width = true;
  const mln_style_image_info info = read_image_info(map, "marker");
  TEST_ASSERT_EQUAL_UINT32(expected.width, info.width);
  TEST_ASSERT_EQUAL_UINT32(expected.height, info.height);
  TEST_ASSERT_EQUAL_UINT32(expected.stride, info.stride);
  TEST_ASSERT_EQUAL_size_t(expected.byte_length, info.byte_length);
  TEST_ASSERT_EQUAL_FLOAT(expected.pixel_ratio, info.pixel_ratio);
  TEST_ASSERT_EQUAL(expected.sdf, info.sdf);
  TEST_ASSERT_EQUAL(expected.has_content, info.has_content);
  TEST_ASSERT_EQUAL_MEMORY(
    &expected.content, &info.content, sizeof(info.content)
  );
  TEST_ASSERT_EQUAL(expected.has_text_fit_width, info.has_text_fit_width);
  TEST_ASSERT_EQUAL_UINT32(expected.text_fit_width, info.text_fit_width);
  TEST_ASSERT_EQUAL(expected.has_text_fit_height, info.has_text_fit_height);
  TEST_ASSERT_EQUAL_size_t(0, info.stretch_x_count);

  uint8_t copied[16] = {0};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, copy_pixels(map, "marker", copied, sizeof(copied), &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_MEMORY(packed, copied, sizeof(packed));

  // Replacing an image replaces its metadata too, back to the defaults.
  image.stride = 12;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("marker"), &image, NULL,
                     &completion.descriptor, NULL
                   )
  );
  const mln_style_image_info replaced = read_image_info(map, "marker");
  const mln_style_image_info defaults = mln_style_image_info_default();
  TEST_ASSERT_EQUAL_FLOAT(defaults.pixel_ratio, replaced.pixel_ratio);
  TEST_ASSERT_EQUAL(defaults.sdf, replaced.sdf);
  TEST_ASSERT_EQUAL(defaults.has_content, replaced.has_content);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, copy_pixels(map, "missing", copied, sizeof(copied), &found)
  );
  TEST_ASSERT_FALSE(found);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(style_image_stretches_are_borrowed_by_the_completion);
  RUN_TEST(style_images_copy_their_metadata_and_packed_pixels);
}
