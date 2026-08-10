// A GeoJSON source added from a URL and from inline data, with a circle layer
// that draws the URL source.

#include <maplibre_native_c.h>
#include <stdio.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
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
    map, view("earthquakes"), view(geojson_url), &options
  );
  if (status != MLN_STATUS_OK) return status;
  // #endregion source

  // #region layer
  const char layer[] =
    "{\"id\":\"earthquake-circles\",\"type\":\"circle\","
    "\"source\":\"earthquakes\"}";
  // #endregion layer

  // #region add-layer
  return mln_map_add_style_layer_json(map, view(layer), view(""));
  // #endregion add-layer
}

mln_status show_one_point(mln_map map, mln_lat_lng position) {
  // #region inline-data
  char data[256];
  const int length = snprintf(
    data, sizeof(data),
    "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
    "\"geometry\":{\"type\":\"Point\",\"coordinates\":[%.8f,%.8f]},"
    "\"properties\":{}}]}",
    position.longitude, position.latitude
  );
  if (length < 0 || (size_t)length >= sizeof(data)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // A null options pointer selects the defaults. Clustering is off by default.
  return mln_map_add_geojson_source_data(map, view("pins"), view(data), NULL);
  // #endregion inline-data
}
