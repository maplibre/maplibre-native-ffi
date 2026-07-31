// Raw C ABI coverage: malformed descriptor counts and unknown enum values are
// hidden by binding-owned style values.

#include <math.h>
#include <string.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

#define MLN_STRING_LITERAL(text) \
  ((mln_string_view){.data = (text), .size = sizeof(text) - 1})

// This verifies malformed coordinate counts and unknown rasterization enum
// values hidden by binding value types.
static void style_value_helpers_reject_unsafe_raw_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_lat_lng coordinates[] = {
    {.latitude = 38.0, .longitude = -123.0},
    {.latitude = 38.0, .longitude = -122.0},
    {.latitude = 37.0, .longitude = -122.0},
    {.latitude = 37.0, .longitude = -123.0},
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_image_source_coordinates(
      map, (mln_string_view){.data = "image-url-source", .size = 16},
      coordinates, 3
    )
  );

  mln_style_tile_source_options options =
    mln_style_tile_source_options_default();
  options.fields = MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  options.raster_encoding = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_raster_dem_source_url(
      map, (mln_string_view){.data = "bad-dem", .size = 7},
      (mln_string_view){.data = "https://example.com/bad.json", .size = 28},
      &options
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Bindings always emit a full struct header, so only raw C callers can present
// a short size or unknown field bits to the GeoJSON source adders.
static void geojson_source_options_reject_unsafe_raw_headers(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_string_view url = {
    .data = "https://example.com/points.geojson", .size = 34
  };

  mln_geojson_source_options short_size = mln_geojson_source_options_default();
  short_size.size = (uint32_t)(sizeof(mln_geojson_source_options) - 1);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "short-header", .size = 12}, url,
      &short_size
    )
  );

  mln_geojson_source_options unknown_field =
    mln_geojson_source_options_default();
  unknown_field.fields = 1U << 31U;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "unknown-field", .size = 13}, url,
      &unknown_field
    )
  );

  mln_geojson_source_options fractional_min_zoom =
    mln_geojson_source_options_default();
  fractional_min_zoom.fields = MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
  fractional_min_zoom.min_zoom = 1.5;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "fractional-min", .size = 14}, url,
      &fractional_min_zoom
    )
  );

  mln_geojson_source_options fractional_max_zoom =
    mln_geojson_source_options_default();
  fractional_max_zoom.fields = MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
  fractional_max_zoom.max_zoom = 17.9;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "fractional-max", .size = 14}, url,
      &fractional_max_zoom
    )
  );

  mln_geojson_source_options fractional_cluster_max_zoom =
    mln_geojson_source_options_default();
  fractional_cluster_max_zoom.fields =
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
  fractional_cluster_max_zoom.cluster_max_zoom = 12.25;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "fractional-cluster", .size = 18}, url,
      &fractional_cluster_max_zoom
    )
  );

  mln_geojson_source_options null_cluster_properties =
    mln_geojson_source_options_default();
  null_cluster_properties.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "null-properties", .size = 15}, url,
      &null_cluster_properties
    )
  );

  mln_json_value json_null_cluster_properties = {
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_NULL,
  };
  mln_geojson_source_options non_object_cluster_properties =
    mln_geojson_source_options_default();
  non_object_cluster_properties.fields =
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  non_object_cluster_properties.cluster_properties =
    &json_null_cluster_properties;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "json-null-properties", .size = 20}, url,
      &non_object_cluster_properties
    )
  );

  // A rejected descriptor leaves the source ID free for a later valid add.
  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_geojson_source_url(
                     map, (mln_string_view){.data = "short-header", .size = 12},
                     url, &clustered
                   )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Supercluster reads every feature geometry as a point, so clustered data
// carrying other geometry used to surface a bare variant access message. The
// C API rejects it up front and names the source, feature, and constraint.
static void clustered_geojson_data_reports_non_point_geometry(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_geometry child = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = {.latitude = 37.8, .longitude = -122.4}}
  };
  const mln_geometry point = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = {.latitude = 37.7, .longitude = -122.5}}
  };
  const mln_geometry collection = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION,
    .data = {.geometry_collection = {.geometries = &child, .geometry_count = 1}}
  };
  const mln_feature features[] = {
    {.size = sizeof(mln_feature), .geometry = &point},
    {.size = sizeof(mln_feature), .geometry = &collection},
  };
  const mln_geojson data = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = features, .feature_count = 2}}
  };

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;
  const mln_string_view clustered_id = {.data = "quakes", .size = 6};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_data(map, clustered_id, &data, &clustered)
  );

  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "\"quakes\""));
  TEST_ASSERT_NOT_NULL(strstr(message, "point geometry on every feature"));
  TEST_ASSERT_NOT_NULL(strstr(message, "feature 1"));
  TEST_ASSERT_NOT_NULL(strstr(message, "geometry collection"));

  // The constraint belongs to clustering alone, so the same data tiles fine on
  // an unclustered source, and the rejected ID stays free.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, clustered_id, &data, NULL)
  );

  // A source added with clustering rejects the same data on update.
  const mln_string_view updated_id = {.data = "clustered-quakes", .size = 16};
  const mln_geojson points = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = features, .feature_count = 1}}
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, updated_id, &points, &clustered)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, updated_id, &data)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_thread_last_error_message(), "\"clustered-quakes\"")
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void clustered_geojson_data_requires_a_feature_collection(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_geometry point = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = {.latitude = 37.7, .longitude = -122.5}}
  };
  const mln_feature feature = {.size = sizeof(mln_feature), .geometry = &point};
  const mln_geojson bare_geometry = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_GEOMETRY,
    .data = {.geometry = &point}
  };
  const mln_geojson single_feature = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE,
    .data = {.feature = &feature}
  };

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;

  // MapLibre Native clusters feature collections only, so both of these would
  // tile unclustered rather than honouring the requested cluster option.
  const mln_string_view geometry_id = {.data = "quakes", .size = 6};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_add_geojson_source_data(
                                   map, geometry_id, &bare_geometry, &clustered
                                 )
  );
  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "\"quakes\""));
  TEST_ASSERT_NOT_NULL(strstr(message, "requires a feature collection"));
  TEST_ASSERT_NOT_NULL(strstr(message, "a bare geometry"));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_add_geojson_source_data(
                                   map, geometry_id, &single_feature, &clustered
                                 )
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_thread_last_error_message(), "a single feature")
  );

  // The constraint belongs to clustering alone, so the same data tiles fine on
  // an unclustered source, and the rejected ID stays free.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, geometry_id, &bare_geometry, NULL)
  );

  // An empty feature collection carries nothing to cluster, so it stays
  // accepted and a later update supplies the features to cluster.
  const mln_geojson empty = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = NULL, .feature_count = 0}}
  };
  const mln_string_view empty_id = {.data = "pending", .size = 7};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, empty_id, &empty, &clustered)
  );

  // A source added with clustering rejects a bare geometry on update too.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, empty_id, &bare_geometry)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_thread_last_error_message(), "\"pending\""));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A vector source plus one layer that takes a source and one that does not, so
// the typed layer accessors can be exercised against both.
static const char layer_accessor_style_json[] =
  "{\"version\":8,\"sources\":{\"vec\":{\"type\":\"vector\","
  "\"tiles\":[\"https://example.com/{z}/{x}/{y}.mvt\"]}},"
  "\"layers\":[{\"id\":\"lines\",\"type\":\"line\",\"source\":\"vec\","
  "\"source-layer\":\"roads\"},{\"id\":\"bg\",\"type\":\"background\"}]}";

// This verifies the typed layer accessors reject a layer type that takes no
// source, which MapLibre's own setProperty path accepts as a silent no-op.
static void layer_source_accessors_reject_sourceless_layer_types(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, layer_accessor_style_json)
  );

  const mln_string_view background = MLN_STRING_LITERAL("bg");
  const mln_string_view source_layer = MLN_STRING_LITERAL("roads");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_layer_source_layer(map, background, source_layer)
  );
  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "source-layer"));

  const mln_string_view source_id = MLN_STRING_LITERAL("vec");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_layer_source_id(map, background, source_id)
  );

  // The style-spec property path still reaches the same layer and reports OK
  // without changing it, which is why the typed setters exist.
  const mln_json_value roads = {
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_STRING,
    .data = {.string_value = {.data = "roads", .size = 5}}
  };
  const mln_string_view property_name = MLN_STRING_LITERAL("source-layer");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_layer_property(map, background, property_name, &roads)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// This verifies the raw buffer contract for copied layer text: the required
// size is reported even when the caller's capacity is too small.
static void layer_text_accessors_report_required_capacity(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, layer_accessor_style_json)
  );

  // A null buffer with zero capacity is a size probe, so it reports the length
  // and succeeds rather than sharing a status with a missing layer.
  const mln_string_view lines = MLN_STRING_LITERAL("lines");
  size_t required = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_layer_source_layer(map, lines, NULL, 0, &required)
  );
  TEST_ASSERT_EQUAL_size_t(5, required);

  char too_small[4] = {0};
  required = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_layer_source_layer(
      map, lines, too_small, sizeof(too_small), &required
    )
  );
  TEST_ASSERT_EQUAL_size_t(5, required);

  char buffer[8] = {0};
  required = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_layer_source_layer(
                     map, lines, buffer, sizeof(buffer), &required
                   )
  );
  TEST_ASSERT_EQUAL_size_t(5, required);
  TEST_ASSERT_EQUAL_INT(0, memcmp(buffer, "roads", 5));

  // A sourceless layer reads back as empty rather than failing.
  const mln_string_view background = MLN_STRING_LITERAL("bg");
  required = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_layer_source_id(
                     map, background, buffer, sizeof(buffer), &required
                   )
  );
  TEST_ASSERT_EQUAL_size_t(0, required);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_layer_source_layer(map, lines, buffer, sizeof(buffer), NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_layer_source_layer(map, lines, NULL, sizeof(buffer), &required)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// This verifies the unbounded zoom range crosses the ABI as infinities and that
// a raw NaN and an unknown visibility value are rejected.
static void layer_zoom_and_visibility_accessors_carry_raw_domains(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, layer_accessor_style_json)
  );

  const mln_string_view lines = MLN_STRING_LITERAL("lines");
  double min_zoom = 0.0;
  double max_zoom = 0.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_min_zoom(map, lines, &min_zoom)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_max_zoom(map, lines, &max_zoom)
  );
  TEST_ASSERT_TRUE(isinf(min_zoom) && min_zoom < 0.0);
  TEST_ASSERT_TRUE(isinf(max_zoom) && max_zoom > 0.0);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_min_zoom(map, lines, 4.0)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_max_zoom(map, lines, 12.5)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_min_zoom(map, lines, &min_zoom)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_max_zoom(map, lines, &max_zoom)
  );
  TEST_ASSERT_EQUAL_DOUBLE(4.0, min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(12.5, max_zoom);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_layer_min_zoom(map, lines, NAN)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_layer_max_zoom(map, lines, NAN)
  );

  uint32_t visibility = 999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_visibility(map, lines, &visibility)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_LAYER_VISIBILITY_VISIBLE, visibility);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_layer_visibility(map, lines, MLN_STYLE_LAYER_VISIBILITY_NONE)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_visibility(map, lines, &visibility)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_LAYER_VISIBILITY_NONE, visibility);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_layer_visibility(map, lines, 999)
  );

  // A missing layer is rejected the same way the rest of the layer family does.
  const mln_string_view missing = MLN_STRING_LITERAL("nope");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_layer_min_zoom(map, missing, &min_zoom)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// This verifies raw stretch and content descriptors, unknown text-fit values,
// and the stretch copy buffer contract that binding image types hide.
static void style_image_stretch_descriptors_reject_unsafe_raw_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, layer_accessor_style_json)
  );

  const uint8_t pixels[4 * 4] = {0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);

  const mln_string_view image_id = MLN_STRING_LITERAL("patch");

  // A backwards interval, a non-finite bound, and a null array with a non-zero
  // count are all rejected.
  const mln_image_stretch backwards[] = {{.from = 2.0F, .to = 1.0F}};
  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  options.stretch_x = backwards;
  options.stretch_x_count = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  // Intervals that run out of order or overlap their predecessor are rejected.
  const mln_image_stretch unordered[] = {
    {.from = 2.0F, .to = 3.0F}, {.from = 0.0F, .to = 1.0F}
  };
  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  options.stretch_x = unordered;
  options.stretch_x_count = 2;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  const mln_image_stretch overlapping[] = {
    {.from = 0.0F, .to = 2.0F}, {.from = 1.0F, .to = 3.0F}
  };
  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  options.stretch_y = overlapping;
  options.stretch_y_count = 2;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  options.stretch_y = NULL;
  options.stretch_y_count = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_CONTENT;
  options.content.left = 2.0F;
  options.content.right = 1.0F;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
  options.text_fit_width = 999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  // An accepted nine-patch reports its stretches, content, and text fit back.
  const mln_image_stretch stretch_x[] = {{.from = 0.0F, .to = 1.0F}};
  const mln_image_stretch stretch_y[] = {
    {.from = 0.0F, .to = 1.0F}, {.from = 1.0F, .to = 2.0F}
  };
  options = mln_style_image_options_default();
  options.fields =
    MLN_STYLE_IMAGE_OPTION_STRETCH_X | MLN_STYLE_IMAGE_OPTION_STRETCH_Y |
    MLN_STYLE_IMAGE_OPTION_CONTENT | MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.stretch_x = stretch_x;
  options.stretch_x_count = 1;
  options.stretch_y = stretch_y;
  options.stretch_y_count = 2;
  options.content.left = 0.5F;
  options.content.top = 0.5F;
  options.content.right = 1.5F;
  options.content.bottom = 1.5F;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_image(map, image_id, &image, &options)
  );

  mln_style_image_info info = mln_style_image_info_default();
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info(map, image_id, &info, &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(1, info.stretch_x_count);
  TEST_ASSERT_EQUAL_size_t(2, info.stretch_y_count);
  TEST_ASSERT_TRUE(info.has_content);
  TEST_ASSERT_EQUAL_FLOAT(0.5F, info.content.left);
  TEST_ASSERT_EQUAL_FLOAT(1.5F, info.content.bottom);
  TEST_ASSERT_FALSE(info.has_text_fit_width);
  TEST_ASSERT_TRUE(info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info.text_fit_height
  );

  // Null arrays with zero capacity probe the counts and succeed.
  size_t x_count = 0;
  size_t y_count = 0;
  found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches(
                     map, image_id, NULL, 0, &x_count, NULL, 0, &y_count, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(1, x_count);
  TEST_ASSERT_EQUAL_size_t(2, y_count);

  // An undersized array reports the counts and fails.
  mln_image_stretch too_small[1] = {{.from = 0.0F, .to = 0.0F}};
  x_count = 0;
  y_count = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_image_stretches(
      map, image_id, NULL, 0, &x_count, too_small, 1, &y_count, &found
    )
  );
  TEST_ASSERT_EQUAL_size_t(2, y_count);

  mln_image_stretch copied_x[1] = {{.from = 0.0F, .to = 0.0F}};
  mln_image_stretch copied_y[2] = {
    {.from = 0.0F, .to = 0.0F}, {.from = 0.0F, .to = 0.0F}
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_image_stretches(
      map, image_id, copied_x, 1, &x_count, copied_y, 2, &y_count, &found
    )
  );
  TEST_ASSERT_EQUAL_FLOAT(1.0F, copied_x[0].to);
  TEST_ASSERT_EQUAL_FLOAT(1.0F, copied_y[1].from);
  TEST_ASSERT_EQUAL_FLOAT(2.0F, copied_y[1].to);

  // A missing image reports zero counts without failing.
  const mln_string_view missing = MLN_STRING_LITERAL("nope");
  x_count = 123;
  y_count = 123;
  found = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches(
                     map, missing, NULL, 0, &x_count, NULL, 0, &y_count, &found
                   )
  );
  TEST_ASSERT_FALSE(found);
  TEST_ASSERT_EQUAL_size_t(0, x_count);
  TEST_ASSERT_EQUAL_size_t(0, y_count);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// This verifies the copy-out family answers a null buffer with zero capacity as
// a size probe, so a caller can size a buffer without reading it as a failure.
static void copy_entry_points_answer_a_null_buffer_as_a_size_probe(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(
      map,
      "{\"version\":8,\"sources\":{\"vec\":{\"type\":\"vector\","
      "\"attribution\":\"probe\","
      "\"tiles\":[\"https://example.com/{z}/{x}/{y}.mvt\"]}},\"layers\":[]}"
    )
  );

  // Source attribution reports its length and succeeds.
  const mln_string_view source_id = MLN_STRING_LITERAL("vec");
  size_t attribution_size = 0;
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_source_attribution(
                     map, source_id, NULL, 0, &attribution_size, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(5, attribution_size);

  // A non-null buffer that is too small still reports the length and fails.
  char too_small[2] = {0};
  attribution_size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_source_attribution(
      map, source_id, too_small, sizeof(too_small), &attribution_size, &found
    )
  );
  TEST_ASSERT_EQUAL_size_t(5, attribution_size);

  // Image pixels report their byte length and succeed.
  const uint8_t pixels[4] = {0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);
  const mln_string_view image_id = MLN_STRING_LITERAL("dot");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_image(map, image_id, &image, NULL)
  );

  size_t byte_length = 0;
  found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_premultiplied_rgba8(
                     map, image_id, NULL, 0, &byte_length, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(4, byte_length);

  uint8_t small_pixels[2] = {0};
  byte_length = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_image_premultiplied_rgba8(
      map, image_id, small_pixels, sizeof(small_pixels), &byte_length, &found
    )
  );
  TEST_ASSERT_EQUAL_size_t(4, byte_length);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_style_values_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_value_helpers_reject_unsafe_raw_descriptors);
  RUN_TEST(geojson_source_options_reject_unsafe_raw_headers);
  RUN_TEST(clustered_geojson_data_reports_non_point_geometry);
  RUN_TEST(clustered_geojson_data_requires_a_feature_collection);
  RUN_TEST(layer_source_accessors_reject_sourceless_layer_types);
  RUN_TEST(layer_text_accessors_report_required_capacity);
  RUN_TEST(layer_zoom_and_visibility_accessors_carry_raw_domains);
  RUN_TEST(style_image_stretch_descriptors_reject_unsafe_raw_values);
  RUN_TEST(copy_entry_points_answer_a_null_buffer_as_a_size_probe);
}
