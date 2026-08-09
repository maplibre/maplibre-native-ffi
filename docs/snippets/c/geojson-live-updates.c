// Replacing the data on a GeoJSON source for each live-feed update.

#include <maplibre_native_c.h>
#include <stdio.h>
#include <string.h>

#define MAX_VEHICLES 512

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

mln_status add_vehicle_source(mln_map map) {
  // #region options
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields |= MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE;
  options.synchronous_update = true;
  // #endregion options

  // #region add
  return mln_map_add_geojson_source_data(
    map, view("vehicles"),
    view("{\"type\":\"FeatureCollection\",\"features\":[]}"), &options
  );
  // #endregion add
}

mln_status publish_vehicles(
  mln_map map, const mln_lat_lng* positions, size_t count, char* json,
  size_t json_capacity
) {
  if (count > MAX_VEHICLES) count = MAX_VEHICLES;

  // #region features
  size_t used = 0;
  int written = snprintf(
    json, json_capacity, "{\"type\":\"FeatureCollection\",\"features\":["
  );
  if (written < 0 || (size_t)written >= json_capacity) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  used = (size_t)written;
  for (size_t i = 0; i < count; i++) {
    written = snprintf(
      json + used, json_capacity - used,
      "%s{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
      "\"coordinates\":[%.8f,%.8f]},\"properties\":{}}",
      i == 0 ? "" : ",", positions[i].longitude, positions[i].latitude
    );
    if (written < 0 || (size_t)written >= json_capacity - used) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    used += (size_t)written;
  }
  if (json_capacity - used < 3) return MLN_STATUS_INVALID_ARGUMENT;
  memcpy(json + used, "]}", 3);
  // #endregion features

  // #region publish
  return mln_map_set_geojson_source_data(map, view("vehicles"), view(json));
  // #endregion publish
}
