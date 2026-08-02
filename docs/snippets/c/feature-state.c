// Attaching host state to one feature, so that style expressions reading
// feature-state draw it differently from its neighbours.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

static mln_feature_state_selector select_poi(const char* feature_id) {
  // #region select
  mln_feature_state_selector selector = {.size = sizeof(selector)};
  selector.fields = MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID |
                    MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
  selector.source_id = sv("places");
  selector.source_layer_id = sv("poi");
  selector.feature_id = sv(feature_id);
  // #endregion select
  return selector;
}

mln_status set_selected(
  mln_render_session session, const char* feature_id, bool selected
) {
  const mln_feature_state_selector selector = select_poi(feature_id);

  // #region value
  const mln_json_value flag = {
    .size = sizeof(flag),
    .type = MLN_JSON_VALUE_TYPE_BOOL,
    .data = {.bool_value = selected},
  };
  // #endregion value

  // #region set
  const mln_json_member members[] = {{.key = sv("selected"), .value = &flag}};
  const mln_json_value state = {
    .size = sizeof(state),
    .type = MLN_JSON_VALUE_TYPE_OBJECT,
    .data = {.object_value = {.members = members, .member_count = 1}},
  };

  // The call copies the value, so `state` can go out of scope afterwards.
  return mln_render_session_set_feature_state(session, &selector, &state);
  // #endregion set
}

mln_status clear_selected(mln_render_session session, const char* feature_id) {
  // #region remove
  mln_feature_state_selector selector = select_poi(feature_id);
  selector.fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  selector.state_key = sv("selected");

  return mln_render_session_remove_feature_state(session, &selector);
  // #endregion remove
}
