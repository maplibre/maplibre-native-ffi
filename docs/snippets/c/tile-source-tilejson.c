// A vector tile source loaded from a TileJSON URL, drawn by a line layer.

#include <maplibre_native_c.h>

// An empty before-layer ID puts the layer on top of the style.
static mln_status add_layer(mln_map map, const mln_completion* completion) {
  // #region layer
  const char layer[] =
    "{\"id\":\"roads\",\"type\":\"line\",\"source\":\"basemap\","
    "\"source-layer\":\"transportation\"}";
  // #endregion layer
  return mln_map_add_style_layer_json(
    map, mln_buffer_view_from_cstr(layer), mln_buffer_view_from_cstr(""),
    completion, NULL
  );
}

mln_status add_basemap(
  mln_map map, const mln_completion* source_completion,
  const mln_completion* layer_completion
) {
  // #region source
  const char source[] =
    "{\"type\":\"vector\",\"url\":\"https://tiles.example.com/planet/"
    "tiles.json\"}";
  const mln_status status = mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr("basemap"),
    mln_buffer_view_from_cstr(source), source_completion, NULL
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return add_layer(map, layer_completion);
  // #endregion source
}
