// Reading the features that a loaded source holds, independent of what the
// style draws.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

static void read_features(mln_feature_query_result result) {
  size_t count = 0;
  mln_feature_query_result_count(result, &count);
  for (size_t index = 0; index < count; index++) {
    mln_queried_feature feature = {.size = sizeof(feature)};
    const mln_status got =
      mln_feature_query_result_get(result, index, &feature);
    if (got != MLN_STATUS_OK) continue;
    // feature.feature holds geometry and properties that the result owns.
  }
}

void list_source_features(mln_render_session session) {
  const mln_string_view source_layers[] = {sv("poi")};
  mln_source_feature_query_options options =
    mln_source_feature_query_options_default();
  options.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  options.source_layer_ids = source_layers;
  options.source_layer_id_count = 1;

  mln_feature_query_result result = MLN_HANDLE_NULL;
  const mln_status queried = mln_render_session_query_source_features(
    session, sv("places"), &options, &result
  );
  if (queried != MLN_STATUS_OK) return;

  read_features(result);
  mln_feature_query_result_destroy(result);
}
