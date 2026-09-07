// Loading a style from a URL, or from text the host already holds.

#include <maplibre_native_c.h>
#include <string.h>

mln_status load_style_from_url(
  mln_map map, const char* style_url, const mln_completion* completion
) {
  // #region url
  return mln_map_set_style_url(map, style_url, completion);
  // #endregion url
}

mln_status load_style_from_text(
  mln_map map, const char* style_json, const mln_completion* completion
) {
  // #region json
  const mln_buffer_view json = {.data = style_json, .size = strlen(style_json)};
  return mln_map_set_style_json(map, json, completion);
  // #endregion json
}
