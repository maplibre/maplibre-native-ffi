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
 * Starts a rendered-feature query against the session's latest driver state.
 *
 * All inputs are copied before return. Core-worker sessions execute on their
 * worker. Caller-driver sessions publish driver work and complete only after
 * the host services it on the graphics thread.
 */
MLN_API mln_status mln_render_session_query_rendered_features_start(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/** Starts a source-feature query against the session's latest driver state. */
MLN_API mln_status mln_render_session_query_source_features_start(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Starts a feature-extension query against the latest driver state. */
MLN_API mln_status mln_render_session_query_feature_extensions_start(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the owned JSON bytes from a completed render query operation. */
MLN_API mln_status mln_render_query_take_result(
  mln_operation operation, mln_buffer* out_result
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_QUERY_H
