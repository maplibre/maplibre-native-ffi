/**
 * @file maplibre_native_c/query.h
 * Public C API declarations for feature queries.
 */

#ifndef MAPLIBRE_NATIVE_C_QUERY_H
#define MAPLIBRE_NATIVE_C_QUERY_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
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
  const mln_screen_point* points MLN_BINDING("length=point_count");
  size_t point_count;
} mln_screen_line_string;

/** Screen-space query geometry data. */
typedef union mln_rendered_query_geometry_data {
  mln_screen_point point
    MLN_BINDING("variant=MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT");
  mln_screen_box box
    MLN_BINDING("variant=MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX");
  mln_screen_line_string line_string
    MLN_BINDING("variant=MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING");
} mln_rendered_query_geometry_data;

/** Rendered feature query geometry descriptor. */
typedef struct mln_rendered_query_geometry {
  uint32_t size;
  /** One of mln_rendered_query_geometry_type. */
  uint32_t type MLN_BINDING("enum=mln_rendered_query_geometry_type");
  mln_rendered_query_geometry_data data MLN_BINDING("tag=type");
} mln_rendered_query_geometry;

/** Optional fields for mln_rendered_feature_query_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_rendered_feature_query_option_field : uint32_t {
  MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1U << 0U,
} mln_rendered_feature_query_option_field;

/** Options for rendered feature queries. */
typedef struct mln_rendered_feature_query_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_rendered_feature_query_option_field");
  /** Optional style layer IDs. When absent, all rendered layers are queried. */
  const mln_buffer_view* layer_ids MLN_BINDING(
    "length=layer_id_count;mask=fields;"
    "bit=MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS"
  );
  size_t layer_id_count;
  /** Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter. */
  const mln_buffer_view* filter MLN_BINDING("nullable=true;encoding=json");
} mln_rendered_feature_query_options;

/** Optional fields for mln_source_feature_query_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_source_feature_query_option_field : uint32_t {
  MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1U << 0U,
} mln_source_feature_query_option_field;

/** Options for source feature queries. */
typedef struct mln_source_feature_query_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_source_feature_query_option_field");
  /** Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
   */
  const mln_buffer_view* source_layer_ids MLN_BINDING(
    "length=source_layer_id_count;mask=fields;"
    "bit=MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS"
  );
  size_t source_layer_id_count;
  /** Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter. */
  const mln_buffer_view* filter MLN_BINDING("nullable=true;encoding=json");
} mln_source_feature_query_options;

/** Optional fields for mln_queried_feature. */
typedef enum MLN_BINDING("kind=bitmask") mln_queried_feature_field : uint32_t {
  MLN_QUERIED_FEATURE_SOURCE_ID = 1U << 0U,
  MLN_QUERIED_FEATURE_SOURCE_LAYER_ID = 1U << 1U,
  MLN_QUERIED_FEATURE_STATE = 1U << 2U,
} mln_queried_feature_field;

/**
 * One query hit borrowed for a completion callback.
 *
 * Every view is valid only for that callback; copy what the host keeps.
 * feature is one UTF-8 GeoJSON Feature. source_id, source_layer_id, and state
 * are present when the matching field bit is set. state is a UTF-8 JSON
 * object.
 */
typedef struct mln_queried_feature {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_queried_feature_field");
  mln_buffer_view feature MLN_BINDING("encoding=json");
  mln_buffer_view source_id
    MLN_BINDING("mask=fields;bit=MLN_QUERIED_FEATURE_SOURCE_ID");
  mln_buffer_view source_layer_id
    MLN_BINDING("mask=fields;bit=MLN_QUERIED_FEATURE_SOURCE_LAYER_ID");
  mln_buffer_view state
    MLN_BINDING("encoding=json;mask=fields;bit=MLN_QUERIED_FEATURE_STATE");
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
  const mln_screen_point* points MLN_BINDING("length=point_count"),
  size_t point_count
) MLN_NOEXCEPT;

/**
 * Starts a rendered-feature query against the session's latest driver state.
 *
 * All inputs are copied before return. Core-worker sessions execute on their
 * worker. Caller-driver sessions publish driver work and complete only after
 * the host services it on the graphics thread. The completion borrows an array
 * of mln_queried_feature values (value_count entries), valid only for the
 * callback.
 *
 * Box geometry is normalized and clipped to the viewport, so a box that
 * over-covers the viewport queries everything visible. A box that lies entirely
 * outside the viewport yields an empty result. Point and line-string geometry
 * are queried as given.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, geometry is
 *   null, undersized, or names an unknown kind, options is undersized or
 *   carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session is abandoned or its target is lost
 *   before the query runs.
 * - MLN_STATUS_NATIVE_ERROR when the query throws on the driver.
 */
MLN_BINDING("execution=query;result=mln_queried_feature;shape=array")
MLN_API mln_status mln_render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options
    MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts a source-feature query against the session's latest driver state.
 * The completion borrows an array of mln_queried_feature values (value_count
 * entries), valid only for the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, source_id is
 *   invalid or empty, options is undersized or carries an invalid field, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session is abandoned or its target is lost
 *   before the query runs.
 * - MLN_STATUS_NATIVE_ERROR when the query throws on the driver.
 */
MLN_BINDING("execution=query;result=mln_queried_feature;shape=array")
MLN_API mln_status mln_render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts a feature-extension query against the latest driver state. The
 * completion borrows one mln_buffer_view holding UTF-8 JSON (value_count 1),
 * valid only for the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, any of
 *   source_id, feature, extension, or extension_field is invalid or empty,
 *   arguments is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session is abandoned or its target is lost
 *   before the query runs.
 * - MLN_STATUS_NATIVE_ERROR when the query throws on the driver.
 */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature MLN_BINDING("encoding=json"),
  mln_buffer_view extension, mln_buffer_view extension_field,
  const mln_buffer_view* arguments MLN_BINDING("encoding=json;nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_QUERY_H
