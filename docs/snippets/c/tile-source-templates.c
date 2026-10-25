// A raster tile source described with tile URL templates and source JSON.

#include <maplibre_native_c.h>

// An empty before-layer ID puts the layer on top of the style.
static mln_status add_layer(mln_map map, const mln_completion* completion) {
  const char layer[] =
    "{\"id\":\"ortho\",\"type\":\"raster\",\"source\":\"ortho\"}";
  return mln_map_add_style_layer_json(
    map, mln_buffer_view_from_cstr(layer), mln_buffer_view_from_cstr(""),
    completion, NULL
  );
}

mln_status add_orthophotos(
  mln_map map, const mln_completion* source_completion,
  const mln_completion* layer_completion
) {
  // #region json
  const char source[] =
    "{\"type\":\"raster\",\"tiles\":["
    "\"https://a.tiles.example.com/ortho/{z}/{x}/{y}.png\","
    "\"https://b.tiles.example.com/ortho/{z}/{x}/{y}.png\"],"
    "\"maxzoom\":19,\"tileSize\":256,"
    "\"bounds\":[5.8,47.2,15.1,55.1],"
    "\"attribution\":\"Imagery: Example Survey\"}";
  // #endregion json
  // #region source
  const mln_status status = mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr("ortho"), mln_buffer_view_from_cstr(source),
    source_completion, NULL
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return add_layer(map, layer_completion);
  // #endregion source
}
