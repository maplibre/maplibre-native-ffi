// Expanding one supercluster cluster into the source features it contains.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

static void read_leaves(mln_buffer result) {
  // #region leaves
  mln_buffer_view json = {0};
  if (mln_buffer_get(result, &json) != MLN_STATUS_OK) return;
  // Parse json.data[0..json.size] as a GeoJSON FeatureCollection. Each feature
  // is one point that the cluster contains.
  // #endregion leaves
}

void list_cluster_leaves(mln_render_session session, mln_buffer_view cluster) {
  // #region arguments
  const mln_buffer_view arguments = view("{\"limit\":10,\"offset\":0}");
  // #endregion arguments

  // #region query
  mln_operation operation = MLN_HANDLE_NULL;
  mln_buffer result = MLN_HANDLE_NULL;
  mln_status queried = mln_render_session_query_feature_extensions_start(
    session, view("places"), cluster, view("supercluster"), view("leaves"),
    &arguments, &operation
  );
  bool completed = false;
  if (queried == MLN_STATUS_OK) {
    queried = mln_operation_wait(operation, -1, &completed);
  }
  mln_status terminal = MLN_STATUS_INVALID_STATE;
  if (queried == MLN_STATUS_OK && completed) {
    queried = mln_operation_get_status(operation, &terminal);
  }
  if (queried == MLN_STATUS_OK) queried = terminal;
  if (queried == MLN_STATUS_OK) {
    queried = mln_render_query_take_result(operation, &result);
  }
  if (queried == MLN_STATUS_OK) read_leaves(result);

  mln_operation_release(operation);
  mln_buffer_destroy(result);
  // #endregion query
}
