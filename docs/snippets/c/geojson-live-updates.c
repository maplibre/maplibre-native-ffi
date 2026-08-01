// Replacing the data on a GeoJSON source that is already on the style, once
// per tick of a live feed.

#include <maplibre_native_c.h>
#include <string.h>

#define MAX_VEHICLES 512

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

mln_status add_vehicle_source(mln_map map) {
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields |= MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE;
  options.synchronous_update = true;

  // The feed has yet to deliver anything, so the source starts empty.
  const mln_geojson empty = {
    .size = sizeof(empty),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = NULL, .feature_count = 0}},
  };
  return mln_map_add_geojson_source_data(map, sv("vehicles"), &empty, &options);
}

mln_status publish_vehicles(
  mln_map map, const mln_lat_lng* positions, size_t count
) {
  if (count > MAX_VEHICLES) count = MAX_VEHICLES;

  // Descriptors are borrowed for the call, so stack storage is enough.
  mln_geometry geometries[MAX_VEHICLES];
  mln_feature features[MAX_VEHICLES];
  for (size_t i = 0; i < count; i++) {
    geometries[i] = (mln_geometry){
      .size = sizeof(mln_geometry),
      .type = MLN_GEOMETRY_TYPE_POINT,
      .data = {.point = positions[i]},
    };
    features[i] = (mln_feature){
      .size = sizeof(mln_feature),
      .geometry = &geometries[i],
    };
  }
  const mln_geojson data = {
    .size = sizeof(data),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {
      .feature_collection = {.features = features, .feature_count = count}
    },
  };

  return mln_map_set_geojson_source_data(map, sv("vehicles"), &data);
}
