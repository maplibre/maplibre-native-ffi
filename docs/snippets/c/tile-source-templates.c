// A raster tile source built from explicit tile URL templates, with the tile
// set described through options.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

static mln_json_value json_string(const char* text) {
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_STRING,
    .data = {.string_value = sv(text)},
  };
}

// An empty before-layer ID puts the layer on top of the style.
static mln_status add_layer(
  mln_map map, const char* layer_id, const char* layer_type,
  const char* source_id
) {
  const mln_json_value id_value = json_string(layer_id);
  const mln_json_value type_value = json_string(layer_type);
  const mln_json_value source_value = json_string(source_id);
  const mln_json_member members[] = {
    {.key = sv("id"), .value = &id_value},
    {.key = sv("type"), .value = &type_value},
    {.key = sv("source"), .value = &source_value},
  };
  const mln_json_value layer = {
    .size = sizeof(layer),
    .type = MLN_JSON_VALUE_TYPE_OBJECT,
    .data = {.object_value = {.members = members, .member_count = 3}},
  };
  return mln_map_add_style_layer_json(map, &layer, sv(""));
}

mln_status add_orthophotos(mln_map map) {
  // #region tiles
  const mln_string_view tiles[] = {
    sv("https://a.tiles.example.com/ortho/{z}/{x}/{y}.png"),
    sv("https://b.tiles.example.com/ortho/{z}/{x}/{y}.png"),
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
  options.attribution = sv("Imagery: Example Survey");
  // #endregion bounds

  // #region source
  const mln_status status = mln_map_add_raster_source_tiles(
    map, sv("ortho"), tiles, sizeof(tiles) / sizeof(tiles[0]), &options
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return add_layer(map, "ortho", "raster", "ortho");
  // #endregion source
}
