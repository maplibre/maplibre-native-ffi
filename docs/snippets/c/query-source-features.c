// Reading the features that a loaded source holds, independent of what the
// style draws.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

static void read_features(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  // #region read
  if (result->status != MLN_STATUS_OK || result->value_count == 0) return;
  const mln_queried_feature* features = result->value;
  const mln_queried_feature hit = features[0];
  (void)hit;
  // Copy hit.feature and any identifier or state view you keep before return.
  // #endregion read
}

mln_status list_source_features(mln_render_session session) {
  // #region options
  const mln_buffer_view source_layers[] = {view("poi")};
  mln_source_feature_query_options options =
    mln_source_feature_query_options_default();
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
    session, view("places"), &options, &completion
  );
  // #endregion query
}
