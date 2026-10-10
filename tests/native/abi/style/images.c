// Runtime style images: the metadata, pixels, and stretch intervals a query
// returns for the image a host set.

#include "support/style.h"
#include "support/test_support.h"

// Deep-copies one image result, whose pixels and intervals die with the
// callback.
typedef struct image_probe {
  atomic_bool done;
  mln_status status;
  bool found;
  mln_style_image_info info;
  uint8_t pixels[16];
  size_t pixel_size;
  mln_image_stretch x;
  mln_image_stretch y;
} image_probe;

static void copy_image(void* user_data, const mln_completion_result* result) {
  image_probe* probe = user_data;
  probe->status = result->status;
  probe->found = result->value_count == 1;
  if (probe->found) {
    const mln_style_image_info* info = result->value;
    probe->info = *info;
    probe->pixel_size = info->pixels.size;
    if (info->pixels.size <= sizeof(probe->pixels)) {
      memcpy(probe->pixels, info->pixels.data, info->pixels.size);
    }
    if (info->stretch_x_count != 0) probe->x = info->stretch_x[0];
    if (info->stretch_y_count != 0) probe->y = info->stretch_y[0];
  }
  mln_test_flag_set(&probe->done);
}

static image_probe read_image(
  mln_runtime runtime, mln_map map, const char* id
) {
  image_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_image,
    .user_data = &probe,
  };
  MLN_TEST_OK(
    mln_map_get_style_image(map, mln_test_view_of(id), &completion, NULL)
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_TRUE(atomic_load(&probe.done));
  MLN_TEST_OK(probe.status);
  return probe;
}

static void set_image(
  mln_map map, const char* id, const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) {
  MLN_TEST_AWAIT_OK(mln_map_set_style_image(
    map, mln_test_view_of(id), image, options, &completion.descriptor, NULL
  ));
}

// A query reports the metadata the host set, including only the text fit it
// names, and returns the pixels tightly packed whatever stride they arrived
// with. The result borrows the pixels and intervals for the completion. A
// missing image reads as no value rather than a failure.
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

  image_probe probe = read_image(runtime, map, "marker");
  TEST_ASSERT_TRUE(probe.found);
  const mln_style_image_info* info = &probe.info;
  TEST_ASSERT_EQUAL_UINT32(2, info->width);
  TEST_ASSERT_EQUAL_UINT32(2, info->height);
  TEST_ASSERT_EQUAL_size_t(sizeof(packed), probe.pixel_size);
  TEST_ASSERT_EQUAL_MEMORY(packed, probe.pixels, sizeof(packed));
  TEST_ASSERT_EQUAL_FLOAT(2.0f, info->pixel_ratio);
  TEST_ASSERT_TRUE(info->sdf);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_STYLE_IMAGE_INFO_CONTENT | MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH |
      MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT,
    info->fields
  );
  TEST_ASSERT_EQUAL_MEMORY(
    &options.content, &info->content, sizeof(info->content)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info->text_fit_width
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY, info->text_fit_height
  );
  TEST_ASSERT_EQUAL_size_t(0, info->stretch_x_count);
  TEST_ASSERT_EQUAL_size_t(0, info->stretch_y_count);

  // Replacing an image replaces its metadata too, back to the defaults.
  set_image(map, "marker", &image, NULL);
  probe = read_image(runtime, map, "marker");
  TEST_ASSERT_EQUAL_FLOAT(1.0f, info->pixel_ratio);
  TEST_ASSERT_FALSE(info->sdf);
  TEST_ASSERT_EQUAL_HEX32(0, info->fields);
  // A member whose bit is absent reads as zero.
  const mln_image_content no_content = {0};
  TEST_ASSERT_EQUAL_MEMORY(&no_content, &info->content, sizeof(info->content));
  TEST_ASSERT_EQUAL_UINT32(0, info->text_fit_width);
  TEST_ASSERT_EQUAL_UINT32(0, info->text_fit_height);

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  set_image(map, "label", &image, &options);
  probe = read_image(runtime, map, "label");
  TEST_ASSERT_EQUAL_HEX32(MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT, info->fields);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info->text_fit_height
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
  probe = read_image(runtime, map, "stretched");
  TEST_ASSERT_EQUAL_size_t(1, info->stretch_x_count);
  TEST_ASSERT_EQUAL_size_t(1, info->stretch_y_count);
  TEST_ASSERT_EQUAL_MEMORY(&stretch_x, &probe.x, sizeof(stretch_x));
  TEST_ASSERT_EQUAL_MEMORY(&stretch_y, &probe.y, sizeof(stretch_y));

  TEST_ASSERT_FALSE(read_image(runtime, map, "missing").found);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A stretch interval with no width or one that runs backwards never reaches
// the map worker.
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
