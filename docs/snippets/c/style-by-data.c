// Styling a layer from feature data. Property values and filters cross the
// boundary as style-spec JSON, so an expression is a JSON array.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

// One helper per value type keeps the descriptor graph readable at the call
// site.
static mln_json_value json_string(const char* text) {
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_STRING,
    .data = {.string_value = sv(text)},
  };
}

static mln_json_value json_double(double number) {
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_DOUBLE,
    .data = {.double_value = number},
  };
}

static mln_json_value json_array(const mln_json_value* values, size_t count) {
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_ARRAY,
    .data = {.array_value = {.values = values, .value_count = count}},
  };
}

mln_status size_and_filter_by_magnitude(mln_map map, const char* layer_id) {
  // ["interpolate", ["linear"], ["get", "mag"], 1, 4, 6, 24]
  const mln_json_value get_mag[] = {json_string("get"), json_string("mag")};
  const mln_json_value magnitude = json_array(get_mag, 2);
  const mln_json_value linear_operator = json_string("linear");
  const mln_json_value linear = json_array(&linear_operator, 1);
  const mln_json_value ramp[] = {
    json_string("interpolate"), linear,           magnitude,
    json_double(1.0),           json_double(4.0), json_double(6.0),
    json_double(24.0),
  };
  const mln_json_value radius = json_array(ramp, 7);

  const mln_status status =
    mln_map_set_layer_property(map, sv(layer_id), sv("circle-radius"), &radius);
  if (status != MLN_STATUS_OK) return status;

  // [">=", ["get", "mag"], 2.5]
  const mln_json_value test[] = {
    json_string(">="), magnitude, json_double(2.5)
  };
  const mln_json_value filter = json_array(test, 3);
  return mln_map_set_layer_filter(map, sv(layer_id), &filter);
}
