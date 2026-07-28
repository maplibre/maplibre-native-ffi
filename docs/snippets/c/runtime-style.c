// Adding a source and a layer to a style that has already loaded. Layers and
// property values are style-spec JSON, passed as a value descriptor rather than
// a JSON string.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

void add_cities_layer(mln_map* map, const char* geojson_url) {
  mln_map_add_geojson_source_url(map, sv("cities"), sv(geojson_url));

  // {"id": "cities", "type": "circle", "source": "cities"}
  const mln_json_value id = {
    .size = sizeof(id),
    .type = MLN_JSON_VALUE_TYPE_STRING,
    .data = {.string_value = sv("cities")},
  };
  const mln_json_value type = {
    .size = sizeof(type),
    .type = MLN_JSON_VALUE_TYPE_STRING,
    .data = {.string_value = sv("circle")},
  };
  const mln_json_value source = {
    .size = sizeof(source),
    .type = MLN_JSON_VALUE_TYPE_STRING,
    .data = {.string_value = sv("cities")},
  };
  const mln_json_member members[] = {
    {.key = sv("id"), .value = &id},
    {.key = sv("type"), .value = &type},
    {.key = sv("source"), .value = &source},
  };
  const mln_json_value layer = {
    .size = sizeof(layer),
    .type = MLN_JSON_VALUE_TYPE_OBJECT,
    .data = {.object_value = {.members = members, .member_count = 3}},
  };

  // An empty before-layer ID puts the layer on top.
  mln_map_add_style_layer_json(map, &layer, sv(""));

  const mln_json_value radius = {
    .size = sizeof(radius),
    .type = MLN_JSON_VALUE_TYPE_DOUBLE,
    .data = {.double_value = 6.0},
  };
  mln_map_set_layer_property(map, sv("cities"), sv("circle-radius"), &radius);
}

void refresh_cities(mln_map* map, const char* new_url) {
  // The source already exists, so this is a set, not an add.
  mln_map_set_geojson_source_url(map, sv("cities"), sv(new_url));
}
