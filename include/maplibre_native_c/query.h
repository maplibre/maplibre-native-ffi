/**
 * @file maplibre_native_c/query.h
 * Public C API declarations for feature queries.
 */

#ifndef MAPLIBRE_NATIVE_C_QUERY_H
#define MAPLIBRE_NATIVE_C_QUERY_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Rendered feature query geometry variants. */
typedef enum mln_rendered_query_geometry_type : uint32_t {
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT = 1,
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX = 2,
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING = 3,
} mln_rendered_query_geometry_type;

/**
 * Screen-space box in logical map pixels.
 *
 * Corners may be given in any order and may extend past the viewport. Rendered
 * queries normalize the corners and clip the box to the viewport.
 */
typedef struct mln_screen_box {
  mln_screen_point min;
  mln_screen_point max;
} mln_screen_box;

/** Screen-space line string in logical map pixels. */
typedef struct mln_screen_line_string {
  /** Points. Null only when point_count is 0. */
  const mln_screen_point* points;
  size_t point_count;
} mln_screen_line_string;

/** Rendered feature query geometry descriptor. */
typedef struct mln_rendered_query_geometry {
  uint32_t size;
  /** One of mln_rendered_query_geometry_type. */
  uint32_t type;
  union {
    mln_screen_point point;
    mln_screen_box box;
    mln_screen_line_string line_string;
  } data;
} mln_rendered_query_geometry;

/** Optional fields for mln_rendered_feature_query_options. */
typedef enum mln_rendered_feature_query_option_field : uint32_t {
  MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1U << 0U,
} mln_rendered_feature_query_option_field;

/** Options for rendered feature queries. */
typedef struct mln_rendered_feature_query_options {
  uint32_t size;
  uint32_t fields;
  /** Optional style layer IDs. When absent, all rendered layers are queried. */
  const mln_buffer_view* layer_ids;
  size_t layer_id_count;
  /** Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter. */
  const mln_buffer_view* filter;
} mln_rendered_feature_query_options;

/** Optional fields for mln_source_feature_query_options. */
typedef enum mln_source_feature_query_option_field : uint32_t {
  MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1U << 0U,
} mln_source_feature_query_option_field;

/** Options for source feature queries. */
typedef struct mln_source_feature_query_options {
  uint32_t size;
  uint32_t fields;
  /** Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
   */
  const mln_buffer_view* source_layer_ids;
  size_t source_layer_id_count;
  /** Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter. */
  const mln_buffer_view* filter;
} mln_source_feature_query_options;

/** Returns default rendered feature query options. */
MLN_API mln_rendered_feature_query_options
mln_rendered_feature_query_options_default(void) MLN_NOEXCEPT;

/** Returns default source feature query options. */
MLN_API mln_source_feature_query_options
mln_source_feature_query_options_default(void) MLN_NOEXCEPT;

/** Returns a rendered point query geometry descriptor. */
MLN_API mln_rendered_query_geometry
mln_rendered_query_geometry_point(mln_screen_point point) MLN_NOEXCEPT;

/** Returns a rendered box query geometry descriptor. */
MLN_API mln_rendered_query_geometry
mln_rendered_query_geometry_box(mln_screen_box box) MLN_NOEXCEPT;

/** Returns a rendered line-string query geometry descriptor. */
MLN_API mln_rendered_query_geometry mln_rendered_query_geometry_line_string(
  const mln_screen_point* points, size_t point_count
) MLN_NOEXCEPT;

/**
 * Queries rendered features from the latest render session state.
 *
 * The session renderer must already exist. geometry and options are borrowed
 * for the duration of the call. Passing null for options uses default options.
 * On success, *out_result receives an owned buffer containing a JSON
 * array. Each element has a GeoJSON Feature in `feature` and may have
 * `sourceId`, `sourceLayerId`, and `state` members. Destroy the buffer with
 * mln_buffer_destroy().
 *
 * Box geometry is normalized and clipped to the viewport, so a box that
 * over-covers the viewport queries everything visible. A box that lies entirely
 * outside the viewport yields an empty result. Point and line-string geometry
 * are queried as given.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, geometry is
 *   null or invalid, options are invalid, out_result is null, or *out_result is
 *   not null.
 * - MLN_STATUS_INVALID_STATE when the session is detached or no renderer has
 *   been created for the session yet.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options, mln_buffer* out_result
) MLN_NOEXCEPT;

/**
 * Queries source features from the latest render session state.
 *
 * The session renderer must already exist. source_id and options are borrowed
 * for the duration of the call. Passing null for options uses default options.
 * On success, *out_result receives an owned buffer using the same JSON
 * envelope as rendered feature queries. Destroy it with
 * mln_buffer_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, source_id is
 *   invalid or empty, options are invalid, out_result is null, or *out_result
 * is not null.
 * - MLN_STATUS_INVALID_STATE when the session is detached or no renderer has
 *   been created for the session yet.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options, mln_buffer* out_result
) MLN_NOEXCEPT;

/**
 * Queries a feature extension from the latest render session state.
 *
 * The session renderer must already exist. source_id, feature, extension,
 * extension_field, and arguments are borrowed for the duration of the call.
 * feature contains one UTF-8 GeoJSON Feature. arguments may be null; when
 * present, it contains a UTF-8 JSON object. On success, *out_result receives an
 * owned buffer containing either a JSON value or a GeoJSON Feature
 * Collection. Destroy it with mln_buffer_destroy().
 *
 * The "supercluster" extension requires "cluster_id" feature properties and
 * "limit" and "offset" arguments to be nonnegative integer JSON literals.
 * Floating-point or negative representations are treated as absent and still
 * produce MLN_STATUS_OK. An absent cluster ID produces JSON null. Absent limit
 * and offset values use the native defaults of ten leaves at offset zero. The
 * result buffer preserves integer JSON representations.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, source_id,
 *   feature, extension, extension_field, or arguments are invalid, out_result
 *   is null, or *out_result is not null.
 * - MLN_STATUS_INVALID_STATE when the session is detached or no renderer has
 *   been created for the session yet.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_buffer* out_result
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_QUERY_H
