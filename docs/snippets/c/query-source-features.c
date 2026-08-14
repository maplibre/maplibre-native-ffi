// Reading the features that a loaded source holds, independent of what the
// style draws.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

static void read_features(mln_buffer result) {
  // #region read
  mln_buffer_view json = {0};
  if (mln_buffer_get(result, &json) != MLN_STATUS_OK) return;
  // Parse json.data[0..json.size] as the query-envelope array. The bytes remain
  // valid until result is destroyed.
  // #endregion read
}

void list_source_features(mln_render_session session) {
  // #region options
  const mln_buffer_view source_layers[] = {view("poi")};
  mln_source_feature_query_options options =
    mln_source_feature_query_options_default();
  options.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  options.source_layer_ids = source_layers;
  options.source_layer_id_count = 1;
  // #endregion options

  // #region query
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status queried = mln_render_session_query_source_features_start(
    session, view("places"), &options, &operation
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
  if (queried != MLN_STATUS_OK) return;

  read_features(result);
  mln_buffer_destroy(result);
  // #endregion query
}
