// Replacing the data on a GeoJSON source for each live-feed update.

#include <maplibre_native_c.h>
#include <stdio.h>
#include <string.h>

#define MAX_VEHICLES 512

mln_status add_vehicle_source(mln_map map, const mln_completion* completion) {
  // #region add
  mln_geo_json_source_data empty = MLN_HANDLE_NULL;
  mln_status status = mln_geo_json_source_data_create(
    mln_buffer_view_from_cstr(
      "{\"type\":\"FeatureCollection\",\"features\":[]}"
    ),
    NULL, &empty, NULL
  );
  if (status != MLN_STATUS_OK) return status;

  status = mln_map_add_geo_json_source_data(
    map, mln_buffer_view_from_cstr("vehicles"), empty, completion, NULL
  );
  mln_geo_json_source_data_release(empty);
  return status;
  // #endregion add
}

mln_status publish_vehicles(
  mln_map map, const mln_lat_lng* positions, size_t count, char* json,
  size_t json_capacity, const mln_completion* completion
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

  // #region prepare
  // Preparation parses and tiles the collection, and runs on any thread. The
  // data carries the same default options the source was added with.
  mln_geo_json_source_data prepared = MLN_HANDLE_NULL;
  mln_status status = mln_geo_json_source_data_create(
    mln_buffer_view_from_cstr(json), NULL, &prepared, NULL
  );
  if (status != MLN_STATUS_OK) return status;
  // #endregion prepare

  // #region publish
  // The install is a cheap command submitted to the map owner.
  status = mln_map_set_geo_json_source_data(
    map, mln_buffer_view_from_cstr("vehicles"), prepared, completion, NULL
  );
  mln_geo_json_source_data_release(prepared);
  return status;
  // #endregion publish
}

mln_status track_position_closely(
  mln_map map, bool tracking, const mln_completion* completion
) {
  // #region synchronous-tiling
  return mln_map_set_geo_json_source_synchronous_tiling(
    map, mln_buffer_view_from_cstr("vehicles"), tracking, completion, NULL
  );
  // #endregion synchronous-tiling
}
