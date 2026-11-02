/**
 * @file maplibre_native_c/query.h
 * Find the features on the map or in a source.
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

/** The shape of the screen area that a rendered feature query covers. */
typedef enum mln_rendered_query_geometry_type : uint32_t {
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT = 1,
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX = 2,
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING = 3,
} mln_rendered_query_geometry_type;

/** A rectangle of the screen, from its `min` corner to its `max` corner. */
typedef struct mln_screen_box {
  mln_screen_point min;
  mln_screen_point max;
} mln_screen_box;

/** A line through points on the screen. */
typedef struct mln_screen_line_string {
  const mln_screen_point* points MLN_BINDING("length=point_count");
  size_t point_count;
} mln_screen_line_string;

/** The point, box, or line of a rendered feature query. */
typedef union mln_rendered_query_geometry_data {
  mln_screen_point point
    MLN_BINDING("variant=MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT");
  mln_screen_box box
    MLN_BINDING("variant=MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX");
  mln_screen_line_string line_string
    MLN_BINDING("variant=MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING");
} mln_rendered_query_geometry_data;

/** The screen area that a rendered feature query covers. */
typedef struct mln_rendered_query_geometry {
  uint32_t size;
  mln_rendered_query_geometry_type type;
  mln_rendered_query_geometry_data data MLN_BINDING("tag=type");
} mln_rendered_query_geometry;

/** The fields that mln_rendered_feature_query_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_rendered_feature_query_option_field : uint32_t {
  MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1U << 0U,
} mln_rendered_feature_query_option_field;

/** A set of mln_rendered_feature_query_option_field bits. */
typedef uint32_t mln_rendered_feature_query_option_fields;

/** Options for a rendered feature query. */
typedef struct mln_rendered_feature_query_options {
  uint32_t size;
  mln_rendered_feature_query_option_fields fields;
  /** The layers to query. Without them, the query covers every layer. */
  const mln_buffer_view* layer_ids MLN_BINDING(
    "length=layer_id_count;mask=fields;"
    "bit=MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS"
  );
  size_t layer_id_count;
  /**
   * A [filter](https://maplibre.org/maplibre-style-spec/expressions/) that
   * each feature must match. Empty matches every feature.
   */
  mln_buffer_view filter MLN_BINDING("encoding=json");
} mln_rendered_feature_query_options;

/** The fields that mln_source_feature_query_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_source_feature_query_option_field : uint32_t {
  MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1U << 0U,
} mln_source_feature_query_option_field;

/** A set of mln_source_feature_query_option_field bits. */
typedef uint32_t mln_source_feature_query_option_fields;

/** Options for a source feature query. */
typedef struct mln_source_feature_query_options {
  uint32_t size;
  mln_source_feature_query_option_fields fields;
  /**
   * The source layers to query. A vector source finds features only in the
   * source layers named here.
   */
  const mln_buffer_view* source_layer_ids MLN_BINDING(
    "length=source_layer_id_count;mask=fields;"
    "bit=MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS"
  );
  size_t source_layer_id_count;
  /**
   * A [filter](https://maplibre.org/maplibre-style-spec/expressions/) that
   * each feature must match. Empty matches every feature.
   */
  mln_buffer_view filter MLN_BINDING("encoding=json");
} mln_source_feature_query_options;

/** A feature that a rendered feature query found. */
typedef struct mln_rendered_feature {
  /** The feature, as a GeoJSON feature. */
  mln_buffer_view feature MLN_BINDING("encoding=json");
  /** The layer that drew the feature. */
  mln_buffer_view layer_id;
  mln_buffer_view source_id;
  /** The feature's source layer, or empty when its source has none. */
  mln_buffer_view source_layer_id;
  /** The feature's state, as a JSON object. */
  mln_buffer_view state MLN_BINDING("encoding=json");
} mln_rendered_feature;

/** A feature that a source feature query found. */
typedef struct mln_source_feature {
  /** The feature, as a GeoJSON feature. */
  mln_buffer_view feature MLN_BINDING("encoding=json");
  /** The feature's source layer, or empty when its source has none. */
  mln_buffer_view source_layer_id;
} mln_source_feature;

/**
 * Finds the features that the session drew in an area of the screen.
 *
 * The query reads the last frame that the session rendered. A feature that
 * several layers draw appears once for each layer.
 */
MLN_BINDING("execution=query;result=mln_rendered_feature;shape=array")
MLN_API mln_status mln_render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options
    MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Finds the features of the source named `source_id`.
 *
 * The query covers only the tiles that the source has loaded for the current
 * view. A source loads tiles only while a visible layer uses it.
 */
MLN_BINDING("execution=query;result=mln_source_feature;shape=array")
MLN_API mln_status mln_render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads a value that a source computes for one of its features.
 *
 * A clustered GeoJSON source answers the `supercluster` extension for a cluster
 * feature. Its `extension_field` is `children` for the cluster's child
 * features, `leaves` for the points in the cluster, or `expansion-zoom` for the
 * zoom level where the cluster splits. For `leaves`, `arguments` can set
 * `limit` and `offset`. Without `limit`, the result has up to 10 points and
 * ignores `offset`. The result is JSON: a GeoJSON feature collection, a number,
 * or null when the source has no answer.
 */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature MLN_BINDING("encoding=json"),
  mln_buffer_view extension, mln_buffer_view extension_field,
  mln_buffer_view arguments MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_QUERY_H
