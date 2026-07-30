// Ask what is drawn under a screen point. Queries hang off the render session
// because they read what the renderer produced.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

void query_at_point(mln_render_session session, double x, double y) {
  const mln_rendered_query_geometry geometry =
    mln_rendered_query_geometry_point((mln_screen_point){.x = x, .y = y});

  // Restricting the query to the layers you care about keeps it from matching
  // background fills.
  const mln_string_view layers[] = {sv("poi-labels")};
  mln_rendered_feature_query_options options =
    mln_rendered_feature_query_options_default();
  options.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  options.layer_ids = layers;
  options.layer_id_count = 1;

  mln_feature_query_result result = MLN_HANDLE_NULL;
  const mln_status queried = mln_render_session_query_rendered_features(
    session, &geometry, &options, &result
  );
  if (queried != MLN_STATUS_OK) return;

  size_t count = 0;
  mln_feature_query_result_count(result, &count);
  for (size_t index = 0; index < count; index++) {
    mln_queried_feature feature = {.size = sizeof(feature)};
    const mln_status got =
      mln_feature_query_result_get(result, index, &feature);
    if (got == MLN_STATUS_OK) {
      // feature.feature holds the geometry and properties; the pointers inside
      // belong to the result, so copy anything you keep.
    }
  }

  mln_feature_query_result_destroy(result);
}
