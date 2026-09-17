// A raster tile source built from explicit tile URL templates, with the tile
// set described through options.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

// An empty before-layer ID puts the layer on top of the style.
static mln_status add_layer(mln_map map, const mln_completion* completion) {
  const char layer[] =
    "{\"id\":\"ortho\",\"type\":\"raster\",\"source\":\"ortho\"}";
  return mln_map_add_style_layer_json(map, view(layer), view(""), completion);
}

mln_status add_orthophotos(
  mln_map map, const mln_completion* source_completion,
  const mln_completion* layer_completion
) {
  // #region tiles
  const mln_buffer_view tiles[] = {
    view("https://a.tiles.example.com/ortho/{z}/{x}/{y}.png"),
    view("https://b.tiles.example.com/ortho/{z}/{x}/{y}.png"),
  };
  // #endregion tiles

  // #region options
  mln_style_tile_source_options options =
    mln_style_tile_source_options_default();
  options.fields = MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM |
                   MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE |
                   MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS |
                   MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
  options.max_zoom = 19;
  // 256 suits classic slippy tiles, and the default is 512.
  options.tile_size = 256;
  // #endregion options

  // #region bounds
  options.bounds = (mln_lat_lng_bounds){
    .southwest = {.latitude = 47.2, .longitude = 5.8},
    .northeast = {.latitude = 55.1, .longitude = 15.1},
  };
  options.attribution = view("Imagery: Example Survey");
  // #endregion bounds
  // #region source
  const mln_status status = mln_map_add_raster_source_tiles(
    map, view("ortho"), tiles, sizeof(tiles) / sizeof(tiles[0]), &options,
    source_completion
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return add_layer(map, layer_completion);
  // #endregion source
}
