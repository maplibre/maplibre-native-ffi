// Attaching host state to one feature for use in style expressions.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

static mln_feature_state_selector select_poi(const char* feature_id) {
  // #region select
  mln_feature_state_selector selector = {.size = sizeof(selector)};
  selector.fields = MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID |
                    MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
  selector.source_id = view("places");
  selector.source_layer_id = view("poi");
  selector.feature_id = view(feature_id);
  // #endregion select
  return selector;
}

mln_status set_selected(
  mln_render_session session, const char* feature_id, bool selected
) {
  const mln_feature_state_selector selector = select_poi(feature_id);

  // #region value
  const mln_buffer_view state =
    view(selected ? "{\"selected\":true}" : "{\"selected\":false}");
  // #endregion value

  // #region set
  // The call parses or copies the bytes before returning.
  return mln_render_session_set_feature_state(session, &selector, state);
  // #endregion set
}

mln_status clear_selected(mln_render_session session, const char* feature_id) {
  // #region remove
  mln_feature_state_selector selector = select_poi(feature_id);
  selector.fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  selector.state_key = view("selected");

  return mln_render_session_remove_feature_state(session, &selector);
  // #endregion remove
}
