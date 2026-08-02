// An elevation tile source drawn as hillshading.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

mln_status add_hillshading(mln_map map) {
  // #region options
  mln_style_tile_source_options options =
    mln_style_tile_source_options_default();
  options.fields = MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING |
                   MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
  options.raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM;
  options.max_zoom = 12;
  // #endregion options

  // #region source
  const mln_string_view tiles[] = {
    sv("https://tiles.example.com/terrain/{z}/{x}/{y}.png"),
  };
  const mln_status status = mln_map_add_raster_dem_source_tiles(
    map, sv("terrain"), tiles, sizeof(tiles) / sizeof(tiles[0]), &options
  );
  // #endregion source
  if (status != MLN_STATUS_OK) {
    return status;
  }

  // #region layer
  // An empty before-layer ID puts the layer on top of the style.
  return mln_map_add_hillshade_layer(
    map, sv("hillshading"), sv("terrain"), sv("")
  );
  // #endregion layer
}
