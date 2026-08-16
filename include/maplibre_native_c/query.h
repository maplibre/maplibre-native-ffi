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

typedef uint64_t mln_queried_feature_list;

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

/** Optional fields for mln_queried_feature. */
typedef enum mln_queried_feature_field : uint32_t {
  MLN_QUERIED_FEATURE_SOURCE_ID = 1U << 0U,
  MLN_QUERIED_FEATURE_SOURCE_LAYER_ID = 1U << 1U,
  MLN_QUERIED_FEATURE_STATE = 1U << 2U,
} mln_queried_feature_field;

/**
 * One query hit borrowed from a queried-feature list.
 *
 * Views remain valid until the owner list is destroyed. feature is one UTF-8
 * GeoJSON Feature. source_id, source_layer_id, and state are present when the
 * matching field bit is set. state is a UTF-8 JSON object.
 */
typedef struct mln_queried_feature {
  uint32_t size;
  uint32_t fields;
  mln_buffer_view feature;
  mln_buffer_view source_id;
  mln_buffer_view source_layer_id;
  mln_buffer_view state;
} mln_queried_feature;

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

/** Returns a default queried-feature descriptor. */
MLN_API mln_queried_feature mln_queried_feature_default(void) MLN_NOEXCEPT;

/**
 * Gets the number of hits in a queried-feature list handle.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, or out_count is
 *   null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_queried_feature_list_count(
  mln_queried_feature_list list, size_t* out_count
) MLN_NOEXCEPT;

/**
 * Borrows one queried feature from a list handle.
 *
 * On success, *out_feature receives views into list-owned storage. The views
 * remain valid until the list is destroyed. out_feature->size must be at least
 * sizeof(mln_queried_feature).
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, index is out of
 *   range, out_feature is null, or out_feature->size is too small.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_queried_feature_list_get(
  mln_queried_feature_list list, size_t index, mln_queried_feature* out_feature
) MLN_NOEXCEPT;

/** Destroys a queried-feature list handle. Null is accepted as a no-op. */
MLN_API void mln_queried_feature_list_destroy(
  mln_queried_feature_list list
) MLN_NOEXCEPT;

/**
 * Starts a rendered-feature query against the session's latest driver state.
 *
 * All inputs are copied before return. Core-worker sessions execute on their
 * worker. Caller-driver sessions publish driver work and complete only after
 * the host services it on the graphics thread. Take the completed result with
 * mln_render_query_features_take_result().
 *
 * Box geometry is normalized and clipped to the viewport, so a box that
 * over-covers the viewport queries everything visible. A box that lies entirely
 * outside the viewport yields an empty result. Point and line-string geometry
 * are queried as given.
 */
MLN_API mln_status mln_render_session_query_rendered_features_start(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Starts a source-feature query against the session's latest driver state.
 * Take the completed result with mln_render_query_features_take_result().
 */
MLN_API mln_status mln_render_session_query_source_features_start(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Starts a feature-extension query against the latest driver state. Take the
 * completed result with mln_render_query_take_result().
 */
MLN_API mln_status mln_render_session_query_feature_extensions_start(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Takes the owned queried-feature list from a completed rendered- or
 * source-feature query operation. Destroy the list with
 * mln_queried_feature_list_destroy().
 */
MLN_API mln_status mln_render_query_features_take_result(
  mln_operation operation, mln_queried_feature_list* out_result
) MLN_NOEXCEPT;

/**
 * Takes the owned JSON bytes from a completed feature-extension query
 * operation.
 */
MLN_API mln_status mln_render_query_take_result(
  mln_operation operation, mln_buffer* out_result
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_QUERY_H
