// An elevation tile source drawn as hillshading.

#include <maplibre_native_c.h>

mln_status add_hillshading(
  mln_map map, const mln_completion* source_completion,
  const mln_completion* layer_completion
) {
  // #region json
  const char source[] =
    "{\"type\":\"raster-dem\",\"encoding\":\"terrarium\",\"maxzoom\":12,"
    "\"tiles\":[\"https://tiles.example.com/terrain/{z}/{x}/{y}.png\"]}";
  // #endregion json
  // #region source
  const mln_status status = mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr("terrain"),
    mln_buffer_view_from_cstr(source), source_completion, NULL
  );
  // #endregion source
  if (status != MLN_STATUS_OK) {
    return status;
  }

  // #region layer
  // An empty before-layer ID puts the layer on top of the style.
  return mln_map_add_style_layer_json(
    map,
    mln_buffer_view_from_cstr(
      "{\"id\":\"hillshading\",\"type\":\"hillshade\",\"source\":\"terrain\"}"
    ),
    mln_buffer_view_from_cstr(""), layer_completion, NULL
  );
  // #endregion layer
}
