// Reading back the feature state that a render session holds for one feature.

#include <maplibre_native_c.h>
#include <string.h>

static bool key_equals(mln_string_view key, const char* name) {
  const size_t length = strlen(name);
  return key.size == length && memcmp(key.data, name, length) == 0;
}

bool read_selected(
  mln_render_session session, const mln_feature_state_selector* selector
) {
  mln_json_snapshot snapshot = MLN_HANDLE_NULL;
  const mln_status got =
    mln_render_session_get_feature_state(session, selector, &snapshot);
  if (got != MLN_STATUS_OK) return false;

  const mln_json_value* root = NULL;
  bool selected = false;
  if (mln_json_snapshot_get(snapshot, &root) == MLN_STATUS_OK) {
    const mln_json_object state = root->data.object_value;
    for (size_t index = 0; index < state.member_count; index++) {
      const mln_json_member member = state.members[index];
      if (
        key_equals(member.key, "selected") &&
        member.value->type == MLN_JSON_VALUE_TYPE_BOOL
      ) {
        selected = member.value->data.bool_value;
      }
    }
  }

  mln_json_snapshot_destroy(snapshot);
  return selected;
}
