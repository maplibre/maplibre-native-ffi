// Attaching host state to one feature for use in style expressions.

#include <maplibre_native_c.h>

static mln_feature_state_selector select_poi(const char* feature_id) {
  // #region select
  mln_feature_state_selector selector = {.size = sizeof(selector)};
  selector.fields = MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID |
                    MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
  selector.source_id = mln_buffer_view_from_cstr("places");
  selector.source_layer_id = mln_buffer_view_from_cstr("poi");
  selector.feature_id = mln_buffer_view_from_cstr(feature_id);
  // #endregion select
  return selector;
}

mln_status set_selected(
  mln_map map, const char* feature_id, bool selected,
  const mln_completion* completion
) {
  const mln_feature_state_selector selector = select_poi(feature_id);

  // #region value
  const mln_buffer_view state = mln_buffer_view_from_cstr(
    selected ? "{\"selected\":true}" : "{\"selected\":false}"
  );
  // #endregion value

  // #region set
  // The start call parses or copies the bytes before returning.
  return mln_map_set_feature_state(map, &selector, state, completion, NULL);
  // #endregion set
}

mln_status clear_selected(
  mln_map map, const char* feature_id, const mln_completion* completion
) {
  // #region remove
  mln_feature_state_selector selector = select_poi(feature_id);
  selector.fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  selector.state_key = mln_buffer_view_from_cstr("selected");

  return mln_map_remove_feature_state(map, &selector, completion, NULL);
  // #endregion remove
}
