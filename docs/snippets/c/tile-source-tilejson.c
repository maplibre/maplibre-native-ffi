// A vector tile source loaded from a TileJSON URL, drawn by a line layer.

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

// Builds {"id": ..., "type": ..., "source": ..., "source-layer": ...} and
// appends it. An empty before-layer ID puts the layer on top of the style.
static mln_status add_layer(
  mln_map map, const char* layer_id, const char* layer_type,
  const char* source_id, const char* source_layer
) {
  const mln_json_value id_value = json_string(layer_id);
  const mln_json_value type_value = json_string(layer_type);
  const mln_json_value source_value = json_string(source_id);
  const mln_json_value source_layer_value = json_string(source_layer);
  const mln_json_member members[] = {
    {.key = sv("id"), .value = &id_value},
    {.key = sv("type"), .value = &type_value},
    {.key = sv("source"), .value = &source_value},
    {.key = sv("source-layer"), .value = &source_layer_value},
  };
  const mln_json_value layer = {
    .size = sizeof(layer),
    .type = MLN_JSON_VALUE_TYPE_OBJECT,
    .data = {.object_value = {.members = members, .member_count = 4}},
  };
  return mln_map_add_style_layer_json(map, &layer, sv(""));
}

mln_status add_basemap(mln_map map) {
  const mln_status status = mln_map_add_vector_source_url(
    map, sv("basemap"), sv("https://tiles.example.com/planet/tiles.json"), NULL
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return add_layer(map, "roads", "line", "basemap", "transportation");
}
