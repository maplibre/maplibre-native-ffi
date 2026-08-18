// Reading back the feature state that a render session holds for one feature.

#include <maplibre_native_c.h>

// #region member
// Implement this with the host's JSON library to read the top-level
// "selected" boolean from json.data[0..json.size].
extern bool host_json_selected(mln_buffer_view json);
// #endregion member

static void read_selected(
  void* user_data, const mln_completion_result* result
) {
  bool* selected = user_data;
  // #region read
  if (result->status != MLN_STATUS_OK || result->value_count != 1) return;
  const mln_buffer_view json = *(const mln_buffer_view*)result->value;
  *selected = host_json_selected(json);
  // #endregion read
}

mln_status start_read_selected(
  mln_render_session session, const mln_feature_state_selector* selector,
  bool* selected
) {
  // #region get
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = read_selected,
    .user_data = selected,
  };
  return mln_render_session_get_feature_state(
    session, selector->source_id, selector->source_layer_id,
    selector->feature_id, &completion
  );
  // #endregion get
}
