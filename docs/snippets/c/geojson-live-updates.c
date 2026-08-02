// Replacing the data on a GeoJSON source that is already on the style, once
// per tick of a live feed.

#include <maplibre_native_c.h>
#include <string.h>

#define MAX_VEHICLES 512

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

// #region geometry
static mln_geometry point_geometry(mln_lat_lng position) {
  return (mln_geometry){
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = position},
  };
}
// #endregion geometry

mln_status add_vehicle_source(mln_map map) {
  // #region options
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields |= MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE;
  options.synchronous_update = true;
  // #endregion options

  // #region add
  const mln_geojson empty = {
    .size = sizeof(empty),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = NULL, .feature_count = 0}},
  };
  return mln_map_add_geojson_source_data(map, sv("vehicles"), &empty, &options);
  // #endregion add
}

mln_status publish_vehicles(
  mln_map map, const mln_lat_lng* positions, size_t count
) {
  if (count > MAX_VEHICLES) count = MAX_VEHICLES;

  // #region features
  // Stack storage is enough, because the update call copies the descriptors.
  mln_geometry geometries[MAX_VEHICLES];
  mln_feature features[MAX_VEHICLES];
  for (size_t i = 0; i < count; i++) {
    geometries[i] = point_geometry(positions[i]);
    features[i] =
      (mln_feature){.size = sizeof(mln_feature), .geometry = &geometries[i]};
  }
  // #endregion features

  // #region publish
  const mln_geojson data = {
    .size = sizeof(data),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {
      .feature_collection = {.features = features, .feature_count = count}
    },
  };

  return mln_map_set_geojson_source_data(map, sv("vehicles"), &data);
  // #endregion publish
}
