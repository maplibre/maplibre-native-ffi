// Reading features from the last frame at a screen position and copying values
// that outlive the query.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

static void read_query_result(mln_buffer result) {
  // #region read
  mln_buffer_view json = {0};
  if (mln_buffer_get(result, &json) != MLN_STATUS_OK) return;
  // Parse json.data[0..json.size] as the query-envelope array and copy any
  // values that must outlive result.
  // #endregion read
}

mln_status features_at_screen_point(
  mln_render_session session, mln_screen_point at
) {
  // #region geometry
  const mln_rendered_query_geometry geometry =
    mln_rendered_query_geometry_box((mln_screen_box){
      .min = {.x = at.x - 6.0, .y = at.y - 6.0},
      .max = {.x = at.x + 6.0, .y = at.y + 6.0},
    });
  // #endregion geometry

  // #region layers
  const mln_buffer_view layer_ids[] = {
    view("poi-labels"), view("building-fill")
  };
  mln_rendered_feature_query_options options =
    mln_rendered_feature_query_options_default();
  options.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  options.layer_ids = layer_ids;
  options.layer_id_count = sizeof(layer_ids) / sizeof(layer_ids[0]);
  // #endregion layers

  // #region query
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status queried = mln_render_session_query_rendered_features_start(
    session, &geometry, &options, &operation
  );
  bool completed = false;
  if (queried == MLN_STATUS_OK) {
    queried = mln_operation_wait(operation, -1, &completed);
  }
  mln_status terminal = MLN_STATUS_INVALID_STATE;
  if (queried == MLN_STATUS_OK && completed) {
    queried = mln_operation_get_status(operation, &terminal);
  }
  mln_buffer result = MLN_HANDLE_NULL;
  if (queried == MLN_STATUS_OK) queried = terminal;
  if (queried == MLN_STATUS_OK) {
    queried = mln_render_query_take_result(operation, &result);
  }
  mln_operation_release(operation);
  if (queried != MLN_STATUS_OK) return queried;

  read_query_result(result);
  mln_buffer_destroy(result);
  return MLN_STATUS_OK;
  // #endregion query
}
