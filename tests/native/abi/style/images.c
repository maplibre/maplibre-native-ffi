// Runtime style images: the metadata, pixels, and stretch intervals a copy
// returns for the image a host set.

#include "support/style.h"
#include "support/test_support.h"

typedef struct stretch_probe {
  atomic_bool done;
  mln_status status;
  size_t value_count;
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
  probe->value_count = result->value_count;
  if (result->value_count == 1) {
    const mln_style_image_stretches_result* stretches = result->value;
    probe->x_count = stretches->stretch_x_count;
    probe->y_count = stretches->stretch_y_count;
    if (probe->x_count != 0) probe->x = stretches->stretch_x[0];
    if (probe->y_count != 0) probe->y = stretches->stretch_y[0];
  }
  mln_test_flag_set(&probe->done);
}

// Copies an image's stretches inside the completion, which borrows them.
static stretch_probe read_stretches(
  mln_runtime runtime, mln_map map, const char* id
) {
  stretch_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_stretches,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_map_copy_style_image_stretches(
    map, mln_test_view_of(id), &completion, NULL
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_TRUE(atomic_load(&probe.done));
  MLN_TEST_OK(probe.status);
  return probe;
}

static mln_style_image_info read_image_info(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_result));
  MLN_TEST_OK(mln_map_get_style_image_info(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  mln_style_image_result result = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&completion, &result, sizeof(result))
  );
  return result.info;
}

// Copies an image's pixels into `out`, and reports whether the image exists.
static bool copy_pixels(mln_map map, const char* id, uint8_t out[16]) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_copy_style_image_premultiplied_rgba8(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  // The view borrows the completion's storage, so copy before destroying it.
  const bool found = mln_test_completion_value_count(&completion) == 1;
  if (found) {
    mln_buffer_view view = {0};
    TEST_ASSERT_TRUE(
      mln_test_completion_copy_value(&completion, &view, sizeof(view))
    );
    TEST_ASSERT_EQUAL_size_t(16, view.size);
    memcpy(out, view.data, view.size);
  }
  mln_test_completion_destroy(&completion);
  return found;
}

static void set_image(
  mln_map map, const char* id, const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) {
  MLN_TEST_AWAIT_OK(mln_map_set_style_image(
    map, mln_test_view_of(id), image, options, &completion.descriptor, NULL
  ));
}

// A copy reports the metadata the host set, including only the text fit it
// names, and returns the pixels tightly packed whatever stride they arrived
// with. A stretch copy borrows the intervals for the completion. A missing
// image reads as no value rather than a failure.
static void style_images_copy_their_metadata_pixels_and_stretches(void) {
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
                   MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH |
                   MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.pixel_ratio = 2.0f;
  options.sdf = true;
  options.content = (mln_image_content){0.0f, 0.0f, 2.0f, 1.0f};
  options.text_fit_width = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY;
  set_image(map, "marker", &image, &options);

  mln_style_image_info info = read_image_info(map, "marker");
  TEST_ASSERT_EQUAL_UINT32(2, info.width);
  TEST_ASSERT_EQUAL_UINT32(2, info.height);
  TEST_ASSERT_EQUAL_UINT32(8, info.stride);
  TEST_ASSERT_EQUAL_size_t(sizeof(packed), info.byte_length);
  TEST_ASSERT_EQUAL_FLOAT(2.0f, info.pixel_ratio);
  TEST_ASSERT_TRUE(info.sdf);
  TEST_ASSERT_TRUE(info.has_content);
  TEST_ASSERT_EQUAL_MEMORY(
    &options.content, &info.content, sizeof(info.content)
  );
  TEST_ASSERT_TRUE(info.has_text_fit_width);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info.text_fit_width
  );
  TEST_ASSERT_TRUE(info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY, info.text_fit_height
  );
  TEST_ASSERT_EQUAL_size_t(0, info.stretch_x_count);
  uint8_t copied[16] = {0};
  TEST_ASSERT_TRUE(copy_pixels(map, "marker", copied));
  TEST_ASSERT_EQUAL_MEMORY(packed, copied, sizeof(packed));

  // Replacing an image replaces its metadata too, back to the defaults.
  set_image(map, "marker", &image, NULL);
  info = read_image_info(map, "marker");
  const mln_style_image_info defaults = mln_style_image_info_default();
  TEST_ASSERT_EQUAL_FLOAT(defaults.pixel_ratio, info.pixel_ratio);
  TEST_ASSERT_EQUAL(defaults.sdf, info.sdf);
  TEST_ASSERT_EQUAL(defaults.has_content, info.has_content);

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  set_image(map, "label", &image, &options);
  info = read_image_info(map, "label");
  TEST_ASSERT_FALSE(info.has_text_fit_width);
  TEST_ASSERT_TRUE(info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info.text_fit_height
  );

  const mln_image_stretch stretch_x = {.from = 0.0f, .to = 1.0f};
  const mln_image_stretch stretch_y = {.from = 0.5f, .to = 2.0f};
  options = mln_style_image_options_default();
  options.fields =
    MLN_STYLE_IMAGE_OPTION_STRETCH_X | MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  options.stretch_x = &stretch_x;
  options.stretch_x_count = 1;
  options.stretch_y = &stretch_y;
  options.stretch_y_count = 1;
  set_image(map, "stretched", &image, &options);
  stretch_probe stretches = read_stretches(runtime, map, "stretched");
  TEST_ASSERT_EQUAL_size_t(1, stretches.x_count);
  TEST_ASSERT_EQUAL_size_t(1, stretches.y_count);
  TEST_ASSERT_EQUAL_MEMORY(&stretch_x, &stretches.x, sizeof(stretch_x));
  TEST_ASSERT_EQUAL_MEMORY(&stretch_y, &stretches.y, sizeof(stretch_y));

  TEST_ASSERT_FALSE(copy_pixels(map, "missing", copied));
  TEST_ASSERT_EQUAL_size_t(
    0, read_stretches(runtime, map, "missing").value_count
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An image with no ID, or a stretch interval with no width or one that runs
// backwards, never reaches the map worker.
static void style_image_inputs_are_validated_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "image_id must not be empty",
    mln_map_set_style_image(
      map, (mln_buffer_view){.data = "", .size = 0}, &image, NULL,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );

  const mln_image_stretch invalid[] = {{0.5f, 0.5f}, {2.0f, 1.0f}};
  for (size_t index = 0; index < 2; index += 1) {
    mln_style_image_options options = mln_style_image_options_default();
    options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
    options.stretch_x = &invalid[index];
    options.stretch_x_count = 1;
    MLN_TEST_EXPECT_COMMAND_REJECTED(
      "intervals must have a positive width",
      mln_map_set_style_image(
        map, MLN_BUFFER_LITERAL("invalid-stretch"), &image, &options,
        &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(style_images_copy_their_metadata_pixels_and_stretches);
  RUN_TEST(style_image_inputs_are_validated_at_submission);
}
