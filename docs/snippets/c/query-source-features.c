// Reading the features that a loaded source holds, independent of what the
// style draws.

#include <maplibre_native_c.h>

static void read_features(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  // #region read
  if (result->status != MLN_STATUS_OK || result->value_count == 0) return;
  const mln_source_feature* features = result->value;
  const mln_source_feature hit = features[0];
  (void)hit;
  // hit.source_layer_id names the source layer when the source has them. Copy
  // hit.feature and any view you keep before return.
  // #endregion read
}

mln_status list_source_features(mln_render_session session) {
  // #region options
  const mln_buffer_view source_layers[] = {mln_buffer_view_from_cstr("poi")};
  mln_source_feature_query_options options = {
    .size = sizeof(mln_source_feature_query_options)
  };
  options.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  options.source_layer_ids = source_layers;
  options.source_layer_id_count = 1;
  // #endregion options

  // #region query
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = read_features,
  };
  return mln_render_session_query_source_features(
    session, mln_buffer_view_from_cstr("places"), &options, &completion, NULL
  );
  // #endregion query
}
