// Loading a style from a URL, or from text the host already holds.

#include <maplibre_native_c.h>
#include <string.h>

mln_status load_style_from_url(mln_map map, const char* style_url) {
  // #region url
  // The command ID correlates the terminal command event.
  uint64_t command_id = 0;
  return mln_map_set_style_url(map, style_url, &command_id);
  // #endregion url
}

mln_status load_style_from_text(mln_map map, const char* style_json) {
  // #region json
  const mln_buffer_view json = {.data = style_json, .size = strlen(style_json)};
  uint64_t command_id = 0;
  return mln_map_set_style_json(map, json, &command_id);
  // #endregion json
}
