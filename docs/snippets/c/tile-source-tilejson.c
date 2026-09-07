// A vector tile source loaded from a TileJSON URL, drawn by a line layer.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

// An empty before-layer ID puts the layer on top of the style.
static mln_status add_layer(mln_map map, const mln_completion* completion) {
  // #region layer
  const char layer[] =
    "{\"id\":\"roads\",\"type\":\"line\",\"source\":\"basemap\","
    "\"source-layer\":\"transportation\"}";
  // #endregion layer
  return mln_map_add_style_layer_json(map, view(layer), view(""), completion);
}

mln_status add_basemap(
  mln_map map, const mln_completion* source_completion,
  const mln_completion* layer_completion
) {
  // #region source
  const mln_status status = mln_map_add_vector_source_url(
    map, view("basemap"), view("https://tiles.example.com/planet/tiles.json"),
    NULL, source_completion
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return add_layer(map, layer_completion);
  // #endregion source
}
