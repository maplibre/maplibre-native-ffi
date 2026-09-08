// Expanding one supercluster cluster into the source features it contains.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

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
  const mln_buffer_view arguments = view("{\"limit\":10,\"offset\":0}");
  // #endregion arguments

  // #region query
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = read_leaves,
  };
  return mln_render_session_query_feature_extensions(
    session, view("places"), cluster, view("supercluster"), view("leaves"),
    &arguments, &completion
  );
  // #endregion query
}
