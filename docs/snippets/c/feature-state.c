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
static mln_status finish_operation(
  mln_status started, mln_operation operation
) {
  if (started != MLN_STATUS_OK) return started;
  bool completed = false;
  mln_status status = mln_operation_wait(operation, -1, &completed);
  mln_status terminal = MLN_STATUS_INVALID_STATE;
  if (status == MLN_STATUS_OK && completed) {
    status = mln_operation_get_status(operation, &terminal);
  }
  if (status == MLN_STATUS_OK) status = terminal;
  mln_operation_release(operation);
  return status;
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
  // The start call parses or copies the bytes before returning.
  mln_operation operation = MLN_HANDLE_NULL;
  const mln_status started = mln_render_session_set_feature_state_start(
    session, selector.source_id, selector.source_layer_id, selector.feature_id,
    state, &operation
  );
  return finish_operation(started, operation);
  // #endregion set
}

mln_status clear_selected(mln_render_session session, const char* feature_id) {
  // #region remove
  mln_feature_state_selector selector = select_poi(feature_id);
  selector.fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  selector.state_key = view("selected");

  mln_operation operation = MLN_HANDLE_NULL;
  const mln_status started = mln_render_session_remove_feature_state_start(
    session, selector.source_id, selector.source_layer_id, selector.feature_id,
    selector.state_key, &operation
  );
  return finish_operation(started, operation);
  // #endregion remove
}
