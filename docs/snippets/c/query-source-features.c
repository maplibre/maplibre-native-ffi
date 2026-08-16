// Reading the features that a loaded source holds, independent of what the
// style draws.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

static void read_features(mln_queried_feature_list result) {
  // #region read
  size_t count = 0;
  if (mln_queried_feature_list_count(result, &count) != MLN_STATUS_OK) return;
  if (count == 0) return;
  mln_queried_feature hit = mln_queried_feature_default();
  if (mln_queried_feature_list_get(result, 0, &hit) != MLN_STATUS_OK) return;
  // Copy hit.feature and any identifier or state view you keep.
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
  mln_queried_feature_list result = MLN_HANDLE_NULL;
  const mln_status queried = mln_render_session_query_source_features(
    session, view("places"), &options, &result
  );
  if (queried != MLN_STATUS_OK) return;

  read_features(result);
  mln_queried_feature_list_destroy(result);
  // #endregion query
}
