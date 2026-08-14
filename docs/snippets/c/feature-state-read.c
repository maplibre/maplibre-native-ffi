// Reading back the feature state that a render session holds for one feature.

#include <maplibre_native_c.h>

// #region member
// Implement this with the host's JSON library to read the top-level
// "selected" boolean from json.data[0..json.size].
extern bool host_json_selected(mln_buffer_view json);
// #endregion member

bool read_selected(
  mln_render_session session, const mln_feature_state_selector* selector
) {
  // #region get
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status got = mln_render_session_get_feature_state_start(
    session, selector->source_id, selector->source_layer_id,
    selector->feature_id, &operation
  );
  bool completed = false;
  if (got == MLN_STATUS_OK) {
    got = mln_operation_wait(operation, -1, &completed);
  }
  mln_status terminal = MLN_STATUS_INVALID_STATE;
  if (got == MLN_STATUS_OK && completed) {
    got = mln_operation_get_status(operation, &terminal);
  }
  mln_buffer result = MLN_HANDLE_NULL;
  if (got == MLN_STATUS_OK) got = terminal;
  if (got == MLN_STATUS_OK) {
    got = mln_render_session_get_feature_state_take_result(operation, &result);
  }
  mln_operation_release(operation);
  if (got != MLN_STATUS_OK) return false;
  // #endregion get

  // #region read
  mln_buffer_view json = {0};
  bool selected = false;
  if (mln_buffer_get(result, &json) == MLN_STATUS_OK) {
    selected = host_json_selected(json);
  }

  mln_buffer_destroy(result);
  // #endregion read
  return selected;
}
