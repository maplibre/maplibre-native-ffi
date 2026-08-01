// Reading the features that the renderer drew at a screen position, and
// copying the part of a result that outlives the query.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

static void copy_text(mln_string_view view, char* out, size_t out_size) {
  const size_t length = view.size < out_size - 1 ? view.size : out_size - 1;
  memcpy(out, view.data, length);
  out[length] = '\0';
}

static bool copy_first_source_id(
  mln_feature_query_result result, char* out_source_id, size_t out_size
) {
  size_t count = 0;
  mln_feature_query_result_count(result, &count);
  for (size_t index = 0; index < count; index++) {
    mln_queried_feature feature = {.size = sizeof(feature)};
    const mln_status got =
      mln_feature_query_result_get(result, index, &feature);
    if (got != MLN_STATUS_OK) continue;
    if ((feature.fields & MLN_QUERIED_FEATURE_SOURCE_ID) == 0) continue;
    copy_text(feature.source_id, out_source_id, out_size);
    return true;
  }
  return false;
}

bool source_at_screen_point(
  mln_render_session session, mln_screen_point at, char* out_source_id,
  size_t out_size
) {
  const mln_rendered_query_geometry geometry =
    mln_rendered_query_geometry_box((mln_screen_box){
      .min = {.x = at.x - 6.0, .y = at.y - 6.0},
      .max = {.x = at.x + 6.0, .y = at.y + 6.0},
    });

  const mln_string_view layer_ids[] = {sv("poi-labels"), sv("building-fill")};
  mln_rendered_feature_query_options options =
    mln_rendered_feature_query_options_default();
  options.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  options.layer_ids = layer_ids;
  options.layer_id_count = sizeof(layer_ids) / sizeof(layer_ids[0]);

  mln_feature_query_result result = MLN_HANDLE_NULL;
  const mln_status queried = mln_render_session_query_rendered_features(
    session, &geometry, &options, &result
  );
  if (queried != MLN_STATUS_OK) return false;

  const bool found = copy_first_source_id(result, out_source_id, out_size);
  mln_feature_query_result_destroy(result);
  return found;
}
