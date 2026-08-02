// Loading a style from a URL, or from text the host already holds.

#include <maplibre_native_c.h>

mln_status load_style_from_url(mln_map map, const char* style_url) {
  // #region url
  // Returns once the map accepts the request. The style, its sources, and its
  // tiles arrive across the pumps that follow.
  return mln_map_set_style_url(map, style_url);
  // #endregion url
}

mln_status load_style_from_text(mln_map map, const char* style_json) {
  // #region json
  // MapLibre parses and copies the text before the call returns, so the host
  // storage behind style_json can be released afterwards.
  return mln_map_set_style_json(map, style_json);
  // #endregion json
}
