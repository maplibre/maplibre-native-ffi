// Adding a source and a layer to a style that has already loaded. Layers and
// property values are style-spec JSON, passed as UTF-8 bytes.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

void add_cities_layer(mln_map map, const char* geojson_url) {
  uint64_t command_id = 0;
  // Null options takes the defaults.
  mln_map_add_geojson_source_url(
    map, view("cities"), view(geojson_url), NULL, &command_id
  );

  const char layer[] =
    "{\"id\":\"cities\",\"type\":\"circle\",\"source\":\"cities\"}";
  mln_map_add_style_layer_json(map, view(layer), view(""), &command_id);
  mln_map_set_layer_property(
    map, view("cities"), view("circle-radius"), view("6.0"), &command_id
  );
}

void refresh_cities(mln_map map, const char* new_url) {
  uint64_t command_id = 0;
  mln_map_set_geojson_source_url(
    map, view("cities"), view(new_url), &command_id
  );
}
