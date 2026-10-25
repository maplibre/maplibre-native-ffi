// Expanding one supercluster cluster into the source features it contains.

#include <maplibre_native_c.h>

static void read_leaves(void* user_data, const mln_completion_result* result) {
  (void)user_data;
  // #region leaves
  if (result->status != MLN_STATUS_OK || result->value_count != 1) return;
  const mln_buffer_view json = *(const mln_buffer_view*)result->value;
  (void)json;
  // Parse json.data[0..json.size] before returning. Each feature is one point
  // that the cluster contains.
  // #endregion leaves
}

mln_status list_cluster_leaves(
  mln_render_session session, mln_buffer_view cluster
) {
  // #region arguments
  const mln_buffer_view arguments =
    mln_buffer_view_from_cstr("{\"limit\":10,\"offset\":0}");
  // #endregion arguments

  // #region query
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = read_leaves,
  };
  return mln_render_session_query_feature_extensions(
    session, mln_buffer_view_from_cstr("places"), cluster,
    mln_buffer_view_from_cstr("supercluster"),
    mln_buffer_view_from_cstr("leaves"), &arguments, &completion, NULL
  );
  // #endregion query
}
