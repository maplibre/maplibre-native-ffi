// A GeoJSON source added from a URL and from inline data, with a circle layer
// that draws the URL source.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

static mln_json_value json_string(const char* text) {
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_STRING,
    .data = {.string_value = sv(text)},
  };
}

static mln_json_value json_object(
  const mln_json_member* members, size_t member_count
) {
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_OBJECT,
    .data = {
      .object_value = {.members = members, .member_count = member_count}
    },
  };
}

mln_status show_earthquakes(mln_map map, const char* geojson_url) {
  // #region options
  // Each value applies when its field bit is set.
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
                    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
  options.cluster = true;
  options.cluster_radius = 60;  // pixels
  // #endregion options

  // #region source
  mln_status status = mln_map_add_geojson_source_url(
    map, sv("earthquakes"), sv(geojson_url), &options
  );
  if (status != MLN_STATUS_OK) return status;
  // #endregion source

  // #region layer
  // {"id": "earthquake-circles", "type": "circle", "source": "earthquakes"}
  const mln_json_value id = json_string("earthquake-circles");
  const mln_json_value type = json_string("circle");
  const mln_json_value source = json_string("earthquakes");
  const mln_json_member members[] = {
    {.key = sv("id"), .value = &id},
    {.key = sv("type"), .value = &type},
    {.key = sv("source"), .value = &source},
  };
  // #endregion layer

  // #region add-layer
  const mln_json_value layer = json_object(members, 3);
  return mln_map_add_style_layer_json(map, &layer, sv(""));
  // #endregion add-layer
}

mln_status show_one_point(mln_map map, mln_lat_lng position) {
  const mln_geometry geometry = {
    .size = sizeof(geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = position},
  };

  // #region inline-data
  const mln_feature feature = {.size = sizeof(feature), .geometry = &geometry};
  const mln_geojson data = {
    .size = sizeof(data),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = &feature, .feature_count = 1}},
  };

  // A null options pointer selects the defaults. Clustering is off by default.
  return mln_map_add_geojson_source_data(map, sv("pins"), &data, NULL);
  // #endregion inline-data
}
