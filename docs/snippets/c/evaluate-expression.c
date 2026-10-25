// Evaluating a style expression without a map, as a layer evaluates it, for a
// value that the host shows itself.

#include <maplibre_native_c.h>
#include <stdlib.h>

// Returns the circle radius that a layer styled by magnitude draws for one
// GeoJSON feature, as JSON that the caller frees, or NULL on failure.
char* circle_radius_json(const char* feature) {
  // #region create
  mln_style_expression radius = MLN_HANDLE_NULL;
  mln_status status = mln_style_expression_create(
    mln_buffer_view_from_cstr(
      "[\"interpolate\",[\"linear\"],[\"get\",\"mag\"],1,4,6,24]"
    ),
    NULL, &radius, NULL
  );
  if (status != MLN_STATUS_OK) return NULL;
  // #endregion create

  // #region inputs
  mln_style_expression_inputs inputs = mln_style_expression_inputs_default();
  inputs.fields = MLN_STYLE_EXPRESSION_INPUT_FEATURE;
  inputs.feature = mln_buffer_view_from_cstr(feature);
  // #endregion inputs

  // #region evaluate
  size_t length = 0;
  status =
    mln_style_expression_evaluate(radius, &inputs, NULL, 0, &length, NULL);
  char* json = status == MLN_STATUS_OK ? malloc(length + 1) : NULL;
  if (json != NULL) {
    status = mln_style_expression_evaluate(
      radius, &inputs, json, length + 1, &length, NULL
    );
  }
  // #endregion evaluate

  mln_style_expression_release(radius);
  if (status != MLN_STATUS_OK) {
    free(json);
    return NULL;
  }
  return json;
}
