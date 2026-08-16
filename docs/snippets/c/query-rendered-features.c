// Reading features from the last frame at a screen position and copying values
// that outlive the query.

#include <maplibre_native_c.h>
#include <string.h>

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

static void read_query_result(mln_queried_feature_list result) {
  // #region read
  size_t count = 0;
  if (mln_queried_feature_list_count(result, &count) != MLN_STATUS_OK) return;
  if (count == 0) return;
  mln_queried_feature hit = mln_queried_feature_default();
  if (mln_queried_feature_list_get(result, 0, &hit) != MLN_STATUS_OK) return;
  // Copy hit.feature and any identifier or state view you keep.
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
  mln_queried_feature_list result = MLN_HANDLE_NULL;
  const mln_status queried = mln_render_session_query_rendered_features(
    session, &geometry, &options, &result
  );
  if (queried != MLN_STATUS_OK) return queried;

  read_query_result(result);
  mln_queried_feature_list_destroy(result);
  return MLN_STATUS_OK;
  // #endregion query
}
