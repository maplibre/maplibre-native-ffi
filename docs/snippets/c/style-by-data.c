// Styling a layer from feature data. Property values and filters use style-spec
// JSON. An expression is a JSON array.

#include <maplibre_native_c.h>
#include <string.h>

// #region node
static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}
// #endregion node

mln_status size_and_filter_by_magnitude(mln_map map, const char* layer_id) {
  // #region expression
  const char radius[] =
    "[\"interpolate\",[\"linear\"],[\"get\",\"mag\"],1,4,6,24]";
  // #endregion expression

  // #region property
  const mln_buffer_view radius_json = view(radius);
  // #endregion property

  // #region set
  const mln_status status = mln_map_set_layer_property(
    map, view(layer_id), view("circle-radius"), radius_json
  );
  if (status != MLN_STATUS_OK) return status;
  // #endregion set

  // #region filter
  // [">=", ["get", "mag"], 2.5]
  const mln_buffer_view filter = view("[\">=\",[\"get\",\"mag\"],2.5]");
  return mln_map_set_layer_filter(map, view(layer_id), &filter);
  // #endregion filter
}
