// Loading a style from a URL, or from text the host already holds.

#include <maplibre_native_c.h>
#include <string.h>

mln_status load_style_from_url(mln_map map, const char* style_url) {
  // #region url
  // Returns once the map accepts the request. The style, its sources, and its
  // tiles arrive across the pumps that follow.
  return mln_map_set_style_url(map, style_url);
  // #endregion url
}

mln_status load_style_from_text(mln_map map, const char* style_json) {
  // #region json
  // MapLibre parses or copies these UTF-8 bytes before the call returns.
  const mln_buffer_view json = {.data = style_json, .size = strlen(style_json)};
  return mln_map_set_style_json(map, json);
  // #endregion json
}

mln_status select_globe_after_style_load(mln_map map) {
  // #region globe
  // Apply after each style-loaded event.
  static const char projection[] = "{\"type\":\"globe\"}";
  const mln_buffer_view json = {
    .data = projection, .size = sizeof(projection) - 1
  };
  return mln_map_set_style_projection_json(map, json);
  // #endregion globe
}
