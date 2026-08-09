// A vector tile source loaded from a TileJSON URL, drawn by a line layer.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

// An empty before-layer ID puts the layer on top of the style.
static mln_status add_layer(mln_map map) {
  // #region layer
  const char layer[] =
    "{\"id\":\"roads\",\"type\":\"line\",\"source\":\"basemap\","
    "\"source-layer\":\"transportation\"}";
  // #endregion layer
  return mln_map_add_style_layer_json(map, view(layer), view(""));
}

mln_status add_basemap(mln_map map) {
  // #region source
  const mln_status status = mln_map_add_vector_source_url(
    map, view("basemap"), view("https://tiles.example.com/planet/tiles.json"),
    NULL
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return add_layer(map);
  // #endregion source
}
