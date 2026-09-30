// Optional style inputs that the other style files leave at their defaults:
// image text fit and stretch intervals, placement transitions, the location
// indicator's top image, zoomed URL sources and custom vector sources, and the
// lookups that find nothing.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "support/harness.h"
#include "support/resources.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const char empty_style[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static mln_premultiplied_rgba8_image one_pixel(const uint8_t* pixel) {
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = 4;
  return image;
}

static void image_text_fit_and_stretches_are_checked_and_kept(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const uint8_t pixel[4] = {0, 0, 0, 0};
  const mln_premultiplied_rgba8_image image = one_pixel(pixel);

  // An interval with no width would make MapLibre divide by zero.
  const mln_image_stretch empty = {.from = 0.5f, .to = 0.5f};
  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  options.stretch_x = &empty;
  options.stretch_x_count = 1;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "positive width", mln_map_set_style_image(
                        map, MLN_BUFFER_LITERAL("fitted"), &image, &options,
                        &completion.descriptor, MLN_TEST_DIAGNOSTIC
                      )
  );

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("fitted"), &image, &options,
                     &completion.descriptor, NULL
                   )
  );
  mln_test_completion info =
    mln_test_completion_default(sizeof(mln_style_image_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info(
                     map, MLN_BUFFER_LITERAL("fitted"), &info.descriptor, NULL
                   )
  );
  mln_style_image_result result = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&info, &result, sizeof(result))
  );
  TEST_ASSERT_TRUE(result.info.has_text_fit_height);
  TEST_ASSERT_FALSE(result.info.has_text_fit_width);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, result.info.text_fit_height
  );

  // A missing image has no stretches to copy.
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

static void placement_transitions_and_the_top_image_round_trip(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, MLN_BUFFER_LITERAL(empty_style));

  mln_style_transition_options transition =
    mln_style_transition_options_default();
  transition.fields = MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  transition.enable_placement_transitions = false;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_transition_options(
                     map, &transition, &completion.descriptor, NULL
                   )
  );
  mln_test_completion read =
    mln_test_completion_default(sizeof(mln_style_transition_options));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_transition_options(map, &read.descriptor, NULL)
  );
  mln_style_transition_options stored = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&read, &stored, sizeof(stored))
  );
  TEST_ASSERT_BITS_HIGH(
    MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS, stored.fields
  );
  TEST_ASSERT_FALSE(stored.enable_placement_transitions);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_location_indicator_layer(
                     map, MLN_BUFFER_LITERAL("puck"), MLN_BUFFER_LITERAL(""),
                     &completion.descriptor, NULL
                   )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_location_indicator_image_name(
      map, MLN_BUFFER_LITERAL("puck"), MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP,
      MLN_BUFFER_LITERAL("puck-top"), &completion.descriptor, NULL
    )
  );
  mln_test_completion property = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_property(
                     map, MLN_BUFFER_LITERAL("puck"),
                     MLN_BUFFER_LITERAL("top-image"), &property.descriptor, NULL
                   )
  );
  char value[64];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&property, value, sizeof(value), NULL)
  );
  TEST_ASSERT_NOT_NULL(strstr(value, "puck-top"));

  // The light accepts JSON values only.
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style light property",
    mln_map_set_style_light_property(
      map, MLN_BUFFER_LITERAL("intensity"), MLN_BUFFER_LITERAL("{"),
      &completion.descriptor, NULL
    )
  );

  // An image source that does not exist reports no coordinates.
  mln_test_completion coordinates =
    mln_test_completion_default(4 * sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_image_source_coordinates(
      map, MLN_BUFFER_LITERAL("missing"), &coordinates.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&coordinates)
  );
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&coordinates));
  mln_test_completion_destroy(&coordinates);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void no_mvt_tile(void* user_data, mln_canonical_tile_id tile) {
  (void)user_data;
  (void)tile;
}

// Zoom bounds a caller sets reach the source; out-of-range ones are refused.
static void zoomed_url_and_custom_sources_take_their_bounds(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_style_serve(runtime, NULL, 0);
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, MLN_BUFFER_LITERAL(empty_style));

  mln_style_tile_source_options zoomed =
    mln_style_tile_source_options_default();
  zoomed.fields = MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM |
                  MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM |
                  MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
  zoomed.min_zoom = 2.0;
  zoomed.max_zoom = 10.0;
  zoomed.vector_encoding = MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_vector_source_url(
                     map, MLN_BUFFER_LITERAL("vector"),
                     MLN_BUFFER_LITERAL("fixture://vector.json"), &zoomed,
                     &completion.descriptor, NULL
                   )
  );
  mln_style_tile_source_options terrain =
    mln_style_tile_source_options_default();
  terrain.fields = MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  terrain.raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_raster_dem_source_url(
                     map, MLN_BUFFER_LITERAL("terrain"),
                     MLN_BUFFER_LITERAL("fixture://terrain.json"), &terrain,
                     &completion.descriptor, NULL
                   )
  );
  const mln_test_style_list sources = mln_test_style_list_source_ids(map);
  TEST_ASSERT_EQUAL_size_t(2, sources.count);

  mln_custom_mvt_vector_source_options custom =
    mln_custom_mvt_vector_source_options_default();
  custom.fetch_tile = no_mvt_tile;
  custom.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM |
                  MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  custom.min_zoom = 1.0;
  custom.max_zoom = 8.0;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL("custom"), &custom, &completion.descriptor, NULL
    )
  );
  custom.max_zoom = 99.0;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "max_zoom", mln_map_add_custom_mvt_vector_source(
                  map, MLN_BUFFER_LITERAL("too-deep"), &custom,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  custom.min_zoom = 9.0;
  custom.max_zoom = 8.0;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "min_zoom", mln_map_add_custom_mvt_vector_source(
                  map, MLN_BUFFER_LITERAL("inverted"), &custom,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An image source's URL reaches the provider as an image request.
static void an_image_source_requests_an_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider* provider = mln_test_provider_create(NULL, 0);
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, MLN_BUFFER_LITERAL(empty_style));

  const mln_lat_lng corners[4] = {
    {.latitude = 1.0, .longitude = -1.0},
    {.latitude = 1.0, .longitude = 1.0},
    {.latitude = -1.0, .longitude = 1.0},
    {.latitude = -1.0, .longitude = -1.0},
  };
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_image_source_url(
      map, MLN_BUFFER_LITERAL("overlay"), corners, 4,
      MLN_BUFFER_LITERAL("fixture://overlay.png"), &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_style_layer_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"id\":\"overlay\",\"type\":\"raster\",\"source\":\"overlay\"}"
      ),
      MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_TRUE(
    mln_test_provider_wait_for_requests(provider, "fixture://overlay.png", 1)
  );
  const mln_test_provider_request* request =
    mln_test_provider_request_at(provider, "fixture://overlay.png", 0);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_IMAGE, request->kind);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

MLN_TEST_GROUP {
  RUN_TEST(image_text_fit_and_stretches_are_checked_and_kept);
  RUN_TEST(placement_transitions_and_the_top_image_round_trip);
  RUN_TEST(zoomed_url_and_custom_sources_take_their_bounds);
  RUN_TEST(an_image_source_requests_an_image);
}
