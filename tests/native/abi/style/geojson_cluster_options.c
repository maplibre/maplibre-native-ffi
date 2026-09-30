// The cluster options that GeoJSON source data takes at preparation, and the
// aggregation expressions that preparation refuses.

#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const char nearby_points[] =
  "{\"type\":\"FeatureCollection\",\"features\":["
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{\"weight\":1}},"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0.001,0.001]},\"properties\":{\"weight\":2}}]}";

static mln_geojson_source_options every_cluster_option(void) {
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  options.cluster = true;
  options.cluster_max_zoom = 15;
  options.cluster_radius = 60;
  options.cluster_min_points = 2;
  options.cluster_properties =
    MLN_BUFFER_LITERAL("{\"weight_sum\":[\"+\",[\"get\",\"weight\"]]}");
  return options;
}

// Preparation takes every cluster option, and fails when MapLibre Native
// cannot parse an aggregation, leaving no handle behind. Preparation needs no
// runtime; the case holds one, as the suite's other GeoJSON cases do.
static void preparation_takes_cluster_options_and_refuses_bad_aggregations(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_geojson_source_options options = every_cluster_option();
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(nearby_points), &options, &data, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, data);
  mln_geojson_source_data_destroy(data);

  options.cluster_properties =
    MLN_BUFFER_LITERAL("{\"weight_sum\":\"not-an-expression\"}");
  data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(nearby_points), &options, &data, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "GeoJSON source options"),
    mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, data);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(preparation_takes_cluster_options_and_refuses_bad_aggregations);
}
