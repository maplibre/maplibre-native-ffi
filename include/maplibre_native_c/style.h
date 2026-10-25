/**
 * @file maplibre_native_c/style.h
 * Public C API declarations for style sources, layers, and images.
 *
 * Every map mutation is a command. It copies input bytes, arrays, option
 * structs, and image pixels before returning and applies them in runtime
 * submission order. Each function lists submission and completion failures
 * separately. Its completion reports committed, superseded, failed, or
 * cancelled disposition. A command that ran, whether it committed or failed,
 * completes with the map snapshot generation that it published, so a caller
 * can fence a later mln_map_get_snapshot() on it.
 *
 * Every query is ordered. It copies its inputs before returning and observes
 * all commands accepted earlier by the runtime. Its completion borrows the
 * typed result for the duration of the callback.
 *
 * A query that reads a whole source, layer, or image by ID completes
 * successfully with no value when no object has that ID. A query that reads one
 * member of an object completes with MLN_STATUS_NOT_FOUND when the object is
 * missing, as a command on it does, and completes with no value only when the
 * object exists and lacks that member, such as a layer without a filter.
 *
 * All declarations in this header are callable from any thread.
 */

#ifndef MAPLIBRE_NATIVE_C_STYLE_H
#define MAPLIBRE_NATIVE_C_STYLE_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Submits a global-state JSON value. JSON null restores the style default.
 *
 * Returns:
 * - MLN_STATUS_OK when accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, property_name is
 *   invalid, value is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when value is not valid JSON.
 * - MLN_STATUS_INVALID_STATE when the style has not loaded.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_global_state_property(
  mln_map map, mln_buffer_view property_name,
  mln_buffer_view value MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/**
 * Queries the global-state JSON object, including style defaults.
 * Completion borrows one mln_buffer_view for the duration of the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when accepted.
 * - MLN_STATUS_INVALID_ARGUMENT for an invalid map handle or completion.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_map_get_global_state(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Style source type values returned by source metadata queries. */
typedef enum mln_style_source_type : uint32_t {
  MLN_STYLE_SOURCE_TYPE_UNKNOWN = 0,
  MLN_STYLE_SOURCE_TYPE_VECTOR = 1,
  MLN_STYLE_SOURCE_TYPE_RASTER = 2,
  MLN_STYLE_SOURCE_TYPE_RASTER_DEM = 3,
  MLN_STYLE_SOURCE_TYPE_GEOJSON = 4,
  MLN_STYLE_SOURCE_TYPE_IMAGE = 5,
  MLN_STYLE_SOURCE_TYPE_VIDEO = 6,
  MLN_STYLE_SOURCE_TYPE_ANNOTATIONS = 7,
  MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR = 8,
  MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR = 9,
} mln_style_source_type;

/** Fields available in mln_style_source_info. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_source_info_field : uint32_t {
  /** The tile source was defined with an inline TileJSON description. */
  MLN_STYLE_SOURCE_INFO_TILEJSON = 1U << 0U,
  /** The inline TileJSON description contains geographic bounds. */
  MLN_STYLE_SOURCE_INFO_BOUNDS = 1U << 1U,
  /** The source exposes a tile size. */
  MLN_STYLE_SOURCE_INFO_TILE_SIZE = 1U << 2U,
  /** The source exposes a vector tile encoding. */
  MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING = 1U << 3U,
  /** The source exposes a DEM raster encoding. */
  MLN_STYLE_SOURCE_INFO_RASTER_ENCODING = 1U << 4U,
} mln_style_source_info_field;

/** A set of mln_style_source_info_field bits. */
typedef uint32_t mln_style_source_info_fields;

/** Tile URL coordinate scheme values. */
typedef enum mln_style_tile_scheme : uint32_t {
  MLN_STYLE_TILE_SCHEME_XYZ = 0,
  MLN_STYLE_TILE_SCHEME_TMS = 1,
} mln_style_tile_scheme;

/** Vector tile encoding values. */
typedef enum mln_style_vector_tile_encoding : uint32_t {
  MLN_STYLE_VECTOR_TILE_ENCODING_MVT = 0,
  MLN_STYLE_VECTOR_TILE_ENCODING_MLT = 1,
} mln_style_vector_tile_encoding;

/** DEM raster encoding values. */
typedef enum mln_style_raster_dem_encoding : uint32_t {
  MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX = 0,
  MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM = 1,
} mln_style_raster_dem_encoding;

/**
 * One stretchable interval along an image axis, in image pixels.
 *
 * MapLibre scales only the stretchable intervals when it resizes the image, so
 * the pixels outside them keep their size. An interval runs from a lower `from`
 * to a higher `to`, and the intervals along one axis run in increasing order
 * without overlapping. mln_map_set_style_image() rejects intervals that break
 * that shape.
 */
typedef struct mln_image_stretch {
  float from;
  float to;
} mln_image_stretch;

/**
 * Content-box insets in image pixels, measured from the image's top-left.
 *
 * MapLibre places a symbol's text inside this box when `icon-text-fit` applies.
 */
typedef struct mln_image_content {
  float left;
  float top;
  float right;
  float bottom;
} mln_image_content;

/** How a stretchable image fits text along one axis. */
typedef enum mln_style_image_text_fit : uint32_t {
  MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK = 0,
  MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY = 1,
  MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL = 2,
} mln_style_image_text_fit;

/** Layer visibility values used by the visibility setter and layer info. */
typedef enum mln_style_layer_visibility : uint32_t {
  MLN_STYLE_LAYER_VISIBILITY_VISIBLE = 0,
  MLN_STYLE_LAYER_VISIBILITY_NONE = 1,
} mln_style_layer_visibility;

/** Field mask values for mln_geojson_source_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_geojson_source_option_field : uint32_t {
  MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
  MLN_GEOJSON_SOURCE_OPTION_TOLERANCE = 1U << 2U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM = 1U << 3U,
  MLN_GEOJSON_SOURCE_OPTION_BUFFER = 1U << 4U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS = 1U << 5U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS = 1U << 6U,
  MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS = 1U << 7U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER = 1U << 8U,
  MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING = 1U << 9U,
} mln_geojson_source_option_field;

/** A set of mln_geojson_source_option_field bits. */
typedef uint32_t mln_geojson_source_option_fields;

/** Field mask values for mln_custom_geometry_source_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_custom_geometry_source_option_field : uint32_t {
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE = 1U << 2U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE = 1U << 3U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER = 1U << 4U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP = 1U << 5U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP = 1U << 6U,
} mln_custom_geometry_source_option_field;

/** A set of mln_custom_geometry_source_option_field bits. */
typedef uint32_t mln_custom_geometry_source_option_fields;

/** Field mask values for mln_custom_mvt_vector_source_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_custom_mvt_vector_source_option_field : uint32_t {
  MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
} mln_custom_mvt_vector_source_option_field;

/** A set of mln_custom_mvt_vector_source_option_field bits. */
typedef uint32_t mln_custom_mvt_vector_source_option_fields;

/** Field mask values for mln_style_image_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_image_option_field : uint32_t {
  MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO = 1U << 0U,
  MLN_STYLE_IMAGE_OPTION_SDF = 1U << 1U,
  MLN_STYLE_IMAGE_OPTION_STRETCH_X = 1U << 2U,
  MLN_STYLE_IMAGE_OPTION_STRETCH_Y = 1U << 3U,
  MLN_STYLE_IMAGE_OPTION_CONTENT = 1U << 4U,
  MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH = 1U << 5U,
  MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT = 1U << 6U,
} mln_style_image_option_field;

/** A set of mln_style_image_option_field bits. */
typedef uint32_t mln_style_image_option_fields;

/** Field mask values for mln_style_image_info. */
typedef enum MLN_BINDING("kind=bitmask") mln_style_image_info_field : uint32_t {
  /** The image declares a content box. */
  MLN_STYLE_IMAGE_INFO_CONTENT = 1U << 0U,
  /** The image declares a horizontal text-fit mode. */
  MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH = 1U << 1U,
  /** The image declares a vertical text-fit mode. */
  MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT = 1U << 2U,
} mln_style_image_info_field;

/** A set of mln_style_image_info_field bits. */
typedef uint32_t mln_style_image_info_fields;

/** Field mask values for mln_style_transition_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_transition_option_field : uint32_t {
  MLN_STYLE_TRANSITION_OPTION_DURATION = 1U << 0U,
  MLN_STYLE_TRANSITION_OPTION_DELAY = 1U << 1U,
  MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS = 1U << 2U,
} mln_style_transition_option_field;

/** A set of mln_style_transition_option_field bits. */
typedef uint32_t mln_style_transition_option_fields;

/**
 * Inline TileJSON metadata of a tile source.
 *
 * mln_style_source_info carries it when fields contains TILEJSON.
 */
typedef struct mln_style_source_tile_info {
  /** Tile URL templates in TileJSON order. */
  const mln_buffer_view* tile_urls MLN_BINDING("length=tile_url_count");
  size_t tile_url_count;
  /** Lowest zoom level the TileJSON provides tiles for. */
  double min_zoom;
  /** Highest zoom level the TileJSON provides tiles for. */
  double max_zoom;
  mln_style_tile_scheme scheme;
} mln_style_source_tile_info;

/**
 * Complete metadata of one style source, borrowed for a completion callback.
 *
 * Views and arrays remain valid until the completion callback returns. A
 * masked member whose bit is absent from fields is zero.
 */
typedef struct mln_style_source_info {
  /** Source ID. */
  mln_buffer_view id;
  mln_style_source_type type;
  /** Bitwise combination of mln_style_source_info_field values. */
  mln_style_source_info_fields fields;
  /** Whether the source is marked volatile. */
  bool is_volatile;
  /** Attribution string, or empty when the source sets none. */
  mln_buffer_view attribution;
  /** URL that the source loads from, or empty when it has none. */
  mln_buffer_view url;
  /** Inline TileJSON metadata, when the source was defined with it. */
  mln_style_source_tile_info tilejson
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_TILEJSON");
  /** Geographic bounds, when inline TileJSON sets them. */
  mln_lat_lng_bounds bounds
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_BOUNDS");
  /** Tile size in pixels, for a tile source. */
  uint32_t tile_size
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_TILE_SIZE");
  /** Vector tile encoding, for a vector source. */
  mln_style_vector_tile_encoding vector_encoding
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING");
  /** DEM raster encoding, when inline TileJSON sets one. */
  mln_style_raster_dem_encoding raster_encoding
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_RASTER_ENCODING");
} mln_style_source_info;

/**
 * One style layer, borrowed for a completion callback.
 *
 * Every view stays valid until the callback returns.
 */
typedef struct mln_style_layer_info {
  /** Layer ID. */
  mln_buffer_view id;
  /** The style-spec layer type string. */
  mln_buffer_view type;
  /** Source ID. Empty for a layer type that takes no source. */
  mln_buffer_view source_id;
  /** Source-layer ID. Empty when the layer sets none. */
  mln_buffer_view source_layer;
  /** Lowest zoom at which the layer draws; -INFINITY with no lower bound. */
  double min_zoom;
  /** Highest zoom at which the layer draws; INFINITY with no upper bound. */
  double max_zoom;
  mln_style_layer_visibility visibility;
} mln_style_layer_info;

/**
 * Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel tile
 * size.
 *
 * These options are fixed for the source's lifetime. URL updates preserve
 * them, and prepared data updates require matching options.
 */
typedef struct mln_geojson_source_options {
  uint32_t size;
  mln_geojson_source_option_fields fields;
  /** Minimum tiling zoom. Defaults to 0. */
  double min_zoom
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM");
  /** Maximum tiling zoom, an integer within [0, 24]. Defaults to 18. */
  double max_zoom
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM");
  /** Douglas-Peucker simplification tolerance. Defaults to 0.375. */
  double tolerance
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_TOLERANCE");
  /** Highest zoom that clusters points. Defaults to 17. */
  double cluster_max_zoom
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM");
  /**
   * Cluster aggregation expressions keyed by property name, as a JSON object
   * whose members follow the MapLibre Style Spec clusterProperties form, or
   * empty for none. The UTF-8 bytes are borrowed for the call.
   */
  mln_buffer_view cluster_properties MLN_BINDING("encoding=json");
  /** Tile buffer in pixels within [0, 512]. Defaults to 128. */
  uint32_t buffer
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_BUFFER");
  /** Cluster radius in pixels within [0, 4095]. Defaults to 50. */
  uint32_t cluster_radius
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS");
  /** Points required to form a cluster. Defaults to 2. */
  uint32_t cluster_min_points
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS");
  /** Adds line distance metrics to line features. Defaults to false. */
  bool line_metrics
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS");
  /**
   * Clusters point features. Defaults to false.
   *
   * Clustering applies to feature collections whose every feature carries point
   * geometry. mln_geojson_source_data_create() rejects a bare geometry, a
   * single feature, or a feature collection that mixes in other geometry. An
   * empty feature collection stays accepted and carries nothing to cluster.
   */
  bool cluster MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_CLUSTER");
  /**
   * Tiles the data for the next render update instead of in the background.
   * Defaults to false.
   *
   * A rendered frame may still wait for tile layout.
   * mln_map_set_geojson_source_synchronous_tiling() overrides this at runtime.
   */
  bool synchronous_tiling
    MLN_BINDING("mask=fields;bit=MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING");
} mln_geojson_source_options;

/** Canonical tile identity used by custom geometry and custom MVT vector source
 * callbacks. */
typedef struct mln_canonical_tile_id {
  uint32_t z;
  uint32_t x;
  uint32_t y;
} mln_canonical_tile_id;

/**
 * Requests one tile of a custom geometry or custom MVT vector source.
 *
 * A source in queue mode queues a copy as an mln_custom_source_tile_delivery
 * instead.
 */
MLN_BINDING("delivery=MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE")
typedef void (*mln_custom_source_fetch_tile_callback)(
  void* user_data, mln_canonical_tile_id tile_id
);

/**
 * Cancels a request for one tile of a custom geometry or custom MVT vector
 * source, best effort.
 *
 * A source in queue mode queues a copy as an mln_custom_source_tile_delivery
 * instead.
 */
MLN_BINDING("delivery=MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE")
typedef void (*mln_custom_source_cancel_tile_callback)(
  void* user_data, mln_canonical_tile_id tile_id
);

/** Options for custom geometry sources. */
typedef struct mln_custom_geometry_source_options {
  uint32_t size;
  mln_custom_geometry_source_option_fields fields;
  /** Tile fetch callback. Required in callback mode; null in queue mode. */
  mln_custom_source_fetch_tile_callback fetch_tile MLN_BINDING("nullable=true");
  /** Optional best-effort tile cancel callback. Null in queue mode. */
  mln_custom_source_cancel_tile_callback cancel_tile
    MLN_BINDING("nullable=true");
  /**
   * Caller-owned callback context retained by pointer, or an opaque host token
   * in queue mode.
   */
  void* user_data MLN_BINDING("kind=context");
  double min_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM");
  double max_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM");
  double tolerance
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE");
  uint32_t tile_size
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE");
  uint32_t buffer
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER");
  bool clip
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP");
  bool wrap
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP");
  /**
   * Optional. Invoked once after an accepted command stops referencing
   * user_data.
   *
   * The call runs on the runtime execution thread when the source is removed
   * explicitly, when a style command replaces the style that held the source,
   * when the map is destroyed, or when asynchronous application rejects the
   * source. It runs at most once for each accepted command. A synchronous
   * rejection before command acceptance invokes no release. A host transfers
   * its callback state to each accepted command and frees that state here. Null
   * means the host needs no release.
   *
   * Free callback state and return promptly. The callback may call the C API,
   * but must not wait for runtime work that only its own thread can advance.
   * Null in queue mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery in place of fetch_tile and cancel_tile: each
   * fetch and each cancel is copied into one
   * MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE or
   * MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE delivery carrying user_data,
   * and one MLN_DELIVERY_TYPE_RETIRED delivery follows the last of them where
   * callback mode runs release_user_data. A command rejects an invalid queue
   * handle with MLN_STATUS_INVALID_ARGUMENT and a released queue with
   * MLN_STATUS_INVALID_STATE.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_custom_geometry_source_options MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** Options for custom MVT vector sources. */
typedef struct mln_custom_mvt_vector_source_options {
  uint32_t size;
  mln_custom_mvt_vector_source_option_fields fields;
  /** Tile fetch callback. Required in callback mode; null in queue mode. */
  mln_custom_source_fetch_tile_callback fetch_tile MLN_BINDING("nullable=true");
  /** Optional best-effort tile cancel callback. Null in queue mode. */
  mln_custom_source_cancel_tile_callback cancel_tile
    MLN_BINDING("nullable=true");
  /**
   * Caller-owned callback context retained by pointer, or an opaque host token
   * in queue mode.
   */
  void* user_data MLN_BINDING("kind=context");
  double min_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM");
  double max_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM");
  /**
   * Optional. Invoked once after an accepted command stops referencing
   * user_data.
   *
   * The call runs on the runtime execution thread when the source is removed
   * explicitly, when a style command replaces the style that held the source,
   * when the map is destroyed, or when asynchronous application rejects the
   * source. It runs at most once for each accepted command. A synchronous
   * rejection before command acceptance invokes no release. A host transfers
   * its callback state to each accepted command and frees that state here.
   * Null means the host needs no release.
   *
   * Free callback state and return promptly. The callback may call the C API,
   * but must not wait for runtime work that only its own thread can advance.
   * Null in queue mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery in place of fetch_tile and cancel_tile: each
   * fetch and each cancel is copied into one
   * MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE or
   * MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE delivery carrying user_data,
   * and one MLN_DELIVERY_TYPE_RETIRED delivery follows the last of them where
   * callback mode runs release_user_data. A command rejects an invalid queue
   * handle with MLN_STATUS_INVALID_ARGUMENT and a released queue with
   * MLN_STATUS_INVALID_STATE.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_custom_mvt_vector_source_options MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** Caller-owned premultiplied RGBA8 image pixels. */
typedef struct mln_premultiplied_rgba8_image {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  /** Bytes per image row. Must be at least width * 4. */
  uint32_t stride;
  /** Premultiplied RGBA8 pixels. Must not be null for a non-empty image. */
  const uint8_t* pixels MLN_BINDING("length=byte_length;encoding=bytes");
  /** Available bytes at pixels. */
  size_t byte_length;
} mln_premultiplied_rgba8_image;

/** Options for runtime style images. */
typedef struct mln_style_image_options {
  uint32_t size;
  mln_style_image_option_fields fields;
  /**
   * Horizontally stretchable intervals. Borrowed for the call and copied before
   * return. May be null only when stretch_x_count is 0.
   */
  const mln_image_stretch* stretch_x MLN_BINDING(
    "length=stretch_x_count;mask=fields;"
    "bit=MLN_STYLE_IMAGE_OPTION_STRETCH_X"
  );
  size_t stretch_x_count;
  /**
   * Vertically stretchable intervals. Borrowed for the call and copied before
   * return. May be null only when stretch_y_count is 0.
   */
  const mln_image_stretch* stretch_y MLN_BINDING(
    "length=stretch_y_count;mask=fields;"
    "bit=MLN_STYLE_IMAGE_OPTION_STRETCH_Y"
  );
  size_t stretch_y_count;
  /** Content box used when icon-text-fit applies. */
  mln_image_content content
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_CONTENT");
  /** Defaults to STRETCH_OR_SHRINK. */
  mln_style_image_text_fit text_fit_width
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH");
  /** Defaults to STRETCH_OR_SHRINK. */
  mln_style_image_text_fit text_fit_height
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT");
  /** Sprite pixel ratio. Defaults to 1. */
  float pixel_ratio
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO");
  /** Whether the image is a signed distance field icon. Defaults to false. */
  bool sdf MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_SDF");
} mln_style_image_options;

/**
 * One complete runtime style image, borrowed for a completion callback.
 *
 * pixels and the stretch arrays remain valid until the completion callback
 * returns. A masked member whose bit is absent from fields is zero.
 */
typedef struct mln_style_image_info {
  /** Image width in pixels. */
  uint32_t width;
  /** Image height in pixels. */
  uint32_t height;
  /** Bitwise combination of mln_style_image_info_field values. */
  mln_style_image_info_fields fields;
  /**
   * Premultiplied RGBA8 pixels in tightly packed rows of width * 4 bytes, top
   * row first.
   */
  mln_buffer_view pixels MLN_BINDING("encoding=bytes");
  /** Horizontally stretchable intervals. */
  const mln_image_stretch* stretch_x MLN_BINDING("length=stretch_x_count");
  size_t stretch_x_count;
  /** Vertically stretchable intervals. */
  const mln_image_stretch* stretch_y MLN_BINDING("length=stretch_y_count");
  size_t stretch_y_count;
  /** Content box, when the image sets one. */
  mln_image_content content
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_INFO_CONTENT");
  /** How the image fits text horizontally, when it sets this. */
  mln_style_image_text_fit text_fit_width
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH");
  /** How the image fits text vertically, when it sets this. */
  mln_style_image_text_fit text_fit_height
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT");
  /** Sprite pixel ratio. */
  float pixel_ratio;
  /** Whether the image is a signed distance field icon. */
  bool sdf;
} mln_style_image_info;

/**
 * Global style transition options.
 *
 * These control how the style animates paint property changes and whether
 * symbol placement changes cross-fade. They are distinct from camera animation
 * options, which time camera moves.
 *
 * A paint property's own style-spec transition, such as
 * "fill-color-transition", overrides these for that property. These apply to
 * every property that declares none.
 */
typedef struct mln_style_transition_options {
  uint32_t size;
  mln_style_transition_option_fields fields;
  /**
   * Transition duration in milliseconds. Must be finite, non-negative, and
   * short enough to represent as a duration.
   *
   * When this field is omitted, paint property changes apply instantly, and
   * a 300 millisecond default governs the symbol placement
   * cross-fade that enable_placement_transitions gates. A continuous-mode map
   * applies that same default to the pattern cross-fade across integer zoom
   * levels, while a still-mode map fades those patterns instantly instead.
   *
   * A still-mode map ignores this field and delay_ms for paint property
   * transitions. A property using these global options reaches its final value
   * in the next still image.
   */
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_STYLE_TRANSITION_OPTION_DURATION");
  /**
   * Transition delay in milliseconds. Must be finite, non-negative, and short
   * enough to represent as a duration.
   *
   * When this field is omitted, transitions start without delay.
   */
  double delay_ms
    MLN_BINDING("mask=fields;bit=MLN_STYLE_TRANSITION_OPTION_DELAY");
  /**
   * Whether symbol placement changes cross-fade.
   *
   * Clearing this makes symbol placement changes apply to the next rendered
   * frame, in every map mode. Hosts that move symbol-backed features at pointer
   * frequency clear it for the duration of the interaction.
   *
   * When this field is omitted, the cross-fade stays on. A style carries no
   * equivalent, so loading a style leaves the cross-fade on.
   */
  bool enable_placement_transitions MLN_BINDING(
    "mask=fields;bit=MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS"
  );
} mln_style_transition_options;

/** Returns default GeoJSON source options. */
MLN_API mln_geojson_source_options
mln_geojson_source_options_default(void) MLN_NOEXCEPT;

/** Returns default custom geometry source options. */
MLN_API mln_custom_geometry_source_options
mln_custom_geometry_source_options_default(void) MLN_NOEXCEPT;

/** Returns default custom MVT vector source options. */
MLN_API mln_custom_mvt_vector_source_options
mln_custom_mvt_vector_source_options_default(void) MLN_NOEXCEPT;

/** Returns a default premultiplied RGBA8 image descriptor. */
MLN_API mln_premultiplied_rgba8_image
mln_premultiplied_rgba8_image_default(void) MLN_NOEXCEPT;

/** Returns default runtime style image options. */
MLN_API mln_style_image_options
mln_style_image_options_default(void) MLN_NOEXCEPT;

/** Returns default global style transition options. */
MLN_API mln_style_transition_options
mln_style_transition_options_default(void) MLN_NOEXCEPT;

/**
 * Adds one style source from a style-spec source JSON object.
 *
 * Vector sources with a TileJSON URL accept minzoom, maxzoom, and encoding
 * ("mvt" or "mlt") in the source JSON. These fields override the loaded
 * TileJSON. For example:
 * {"type":"vector","url":"https://tiles.example/source.json",
 *  "minzoom":2,"maxzoom":14,"encoding":"mlt"}.
 *
 * source_id and source_json are borrowed for the call and copied before
 * return. source_json is the object that appears under sources[source_id] in a
 * style document.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, source_json is empty or invalid, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID, or the
 *   source JSON cannot be converted.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_style_source_json(
  mln_map map, mln_buffer_view source_id,
  mln_buffer_view source_json MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes one style source by ID.
 *
 * source_id is borrowed for the call. The command commits when a source with
 * that ID existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_STATE when the source exists but a layer still uses it.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the complete metadata of one style source.
 *
 * A found source completes with one borrowed mln_style_source_info that
 * includes its attribution, URL, and inline TileJSON tile URLs. A missing
 * source completes successfully with no value. Copy retained views before the
 * callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=query;result=mln_style_source_info;nullable=true")
MLN_API mln_status mln_map_get_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets whether one style source stores fetched tiles in the persistent cache.
 *
 * source_id is copied before acceptance. When is_volatile is true, sources
 * that fetch tiles stop storing them in persistent storage.
 * Other source types retain the value for inspection without changing how they
 * load. The change applies when the command commits and is visible through
 * mln_map_get_style_source as is_volatile.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_source_volatile(
  mln_map map, mln_buffer_view source_id, bool is_volatile,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Lists every style source in style order.
 *
 * The completion borrows value_count mln_style_source_info values, value_size
 * bytes apart, one per source: the sources the style document declares, in its
 * order, then added sources in the order they were added. Copy retained
 * entries, their views, and their tile URLs before the callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=query;result=mln_style_source_info;shape=array")
MLN_API mln_status mln_map_list_style_sources(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds a GeoJSON source with URL data.
 *
 * source_id, url, and options are borrowed for the call. The source loads
 * GeoJSON from url through MapLibre Native's resource system. options may be
 * null for defaults, and the options are fixed for the lifetime of the source.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id or url
 *   is invalid or empty, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID, or the
 *   options cannot be converted.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geojson_source_options* options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Prepares GeoJSON source data for installation on a map.
 *
 * data contains UTF-8 GeoJSON. data and options are borrowed for the call.
 * Preparation finishes before return. options may be null for defaults and
 * must match the options of every source the data is installed on.
 *
 * When options enable clustering, the data must be a feature collection whose
 * every feature carries point geometry. Data that does not is rejected, and
 * the diagnostic names the constraint.
 *
 * The prepared data is immutable and independent of any map or runtime.
 * Preparation, installation, and release may happen on different threads.
 *
 * *out_data must be MLN_HANDLE_NULL on entry. On success it receives an owned
 * handle the host releases with mln_geojson_source_data_release(). Installing
 * the data borrows the handle, so one prepared handle may be installed on any
 * number of sources and released at any time afterward.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when data is empty or invalid, options is
 *   invalid, out_data is null or *out_data is not MLN_HANDLE_NULL, or
 *   clustering is enabled and the data is a bare geometry or a single feature,
 *   or a feature carries geometry other than a point.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_geojson_source_data_create(
  mln_buffer_view data MLN_BINDING("encoding=bytes"),
  const mln_geojson_source_options* options MLN_BINDING("nullable=true"),
  mln_geojson_source_data* out_data MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases prepared GeoJSON source data.
 *
 * Callable from any thread. A null or already-released handle is a no-op.
 * Sources the data was installed on keep their own reference, so releasing
 * the handle preserves each source.
 */
MLN_API void mln_geojson_source_data_release(
  mln_geojson_source_data data
) MLN_NOEXCEPT;

/**
 * Adds a GeoJSON source with prepared inline data.
 *
 * source_id is borrowed for the call. data names a live handle from
 * mln_geojson_source_data_create(); the call borrows the handle, and the
 * source adopts the options the data was prepared with, fixed for the lifetime
 * of the source.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, or data is an invalid handle.
 * - MLN_STATUS_INVALID_STATE when map or data has been released, or the map is
 *   closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Updates one GeoJSON source to load data from a URL.
 *
 * source_id and url are borrowed for the call. The source keeps the
 * mln_geojson_source_options it was added with.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id or url
 *   is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a GeoJSON source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Updates one GeoJSON source with prepared inline data.
 *
 * source_id is borrowed for the call. data names a live handle from
 * mln_geojson_source_data_create(); the call borrows the handle and retains
 * prepared data before returning.
 *
 * The data must have been prepared with options equal to the options the
 * source was added with. Cluster aggregation expressions compare by parsed
 * equality, so equivalent cluster_properties JSON matches regardless of
 * formatting. A mismatch is rejected.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, data is an invalid handle, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map or data has been released, or the map is
 *   closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a GeoJSON source, or
 *   the data was prepared with options that do not match the source's options.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Overrides one GeoJSON source's synchronous tiling at runtime.
 *
 * source_id is borrowed for the call. While enabled is true, the source
 * behaves as if its options had set synchronous_tiling; false restores the
 * option the source was added with. The override applies to render updates
 * after the command commits.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a GeoJSON source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_geojson_source_synchronous_tiling(
  mln_map map, mln_buffer_view source_id, bool enabled,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds a custom geometry source.
 *
 * source_id is borrowed for the call. options is borrowed for the call, but the
 * callback function pointers and user_data pointer are retained by value. The
 * callback functions and user_data must remain valid until the source is
 * removed, the style is replaced, or the map is destroyed, and until any
 * in-flight callback invocation has returned. For URL loads, style replacement
 * occurs when the new style loads, not when the load request is accepted. For
 * inline JSON loads, style replacement completes when
 * mln_map_set_style_json() commits.
 *
 * fetch_tile and cancel_tile may run on arbitrary native worker threads, may be
 * concurrent with runtime and map calls. The style command functions in this
 * header may be called directly from these callbacks. Callbacks must not block
 * waiting for command execution. Every callback returns normally; see
 * mln_user_data_release. cancel_tile is best-effort and may be repeated or
 * race with fetch_tile.
 *
 * Custom geometry sources belong to the current style. Replacing the style
 * drops sources that were added to the previous style.
 *
 * A host that owns callback state frees it in options.release_user_data, which
 * this API invokes once after it stops referencing user_data. See
 * mln_custom_geometry_source_options.release_user_data.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, options is null or invalid, or options sets neither or
 *   both of fetch_tile and queue.
 * - MLN_STATUS_INVALID_STATE when map or options->queue has been released, or
 *   the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID. This API
 *   runs release_user_data once.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets custom geometry source data for one canonical tile.
 *
 * source_id and the UTF-8 GeoJSON data are borrowed for the call and copied
 * before return.
 *
 * MapLibre ignores the data unless fetch_tile requested that tile and
 * cancel_tile has not cancelled it since. This call still returns MLN_STATUS_OK
 * after validation.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, tile_id is invalid, data is invalid or empty, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the data is not valid GeoJSON, or the
 *   source is not a custom geometry source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reports a custom geometry source error for one canonical tile.
 *
 * source_id and the UTF-8 diagnostic message are borrowed for the call. The
 * function copies accepted bytes before return. An empty message is accepted.
 *
 * MapLibre ignores the error unless fetch_tile requested that tile and
 * cancel_tile has not cancelled it since. This call still returns MLN_STATUS_OK
 * after validation. The tile stays failed until the host delivers its data or
 * invalidates it, which fetches it again.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, tile_id is invalid, message is a null pointer with a
 *   nonzero size, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom geometry
 *   source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_geometry_source_tile_error(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Invalidates custom geometry source data for one canonical tile.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, tile_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom geometry
 *   source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Invalidates custom geometry source data inside one geographic region.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, bounds is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom geometry
 *   source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds a custom MVT vector source.
 *
 * source_id is borrowed for the call. options is borrowed for the call, but the
 * callback function pointers and user_data pointer are retained by value. The
 * callback functions and user_data must remain valid until the source is
 * removed, the style is replaced, or the map is destroyed, and until any
 * in-flight callback invocation has returned. For URL loads, style replacement
 * occurs when the new style loads, not when the load request is accepted. For
 * inline JSON loads, style replacement completes when
 * mln_map_set_style_json() commits.
 *
 * fetch_tile and cancel_tile may run on arbitrary native worker threads, may be
 * concurrent with runtime and map calls. The style command functions in this
 * header, including the tile-delivery commands below, may be called directly
 * from these callbacks. Callbacks must not block waiting for command
 * execution. Every callback returns normally; see mln_user_data_release.
 * cancel_tile is best-effort and may be repeated or race with fetch_tile.
 *
 * Custom MVT vector sources belong to the current style. Replacing the style
 * drops sources that were added to the previous style. A layer that draws this
 * source names a source-layer that exists inside the MVT bytes the host
 * delivers.
 *
 * A host that owns callback state frees it in options.release_user_data, which
 * this API invokes once after it stops referencing user_data. See
 * mln_custom_mvt_vector_source_options.release_user_data.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, options is null or invalid, or options sets neither or
 *   both of fetch_tile and queue.
 * - MLN_STATUS_INVALID_STATE when map or options->queue has been released, or
 *   the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID. This API
 *   runs release_user_data once.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_custom_mvt_vector_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_mvt_vector_source_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets custom MVT vector source data for one canonical tile.
 *
 * source_id and the MVT protobuf bytes are borrowed for the call. The function
 * copies accepted bytes before return. A zero-length view, including a null
 * pointer with size 0, is an empty tile.
 *
 * MapLibre ignores the bytes unless fetch_tile requested that tile and
 * cancel_tile has not cancelled it since. This call still returns MLN_STATUS_OK
 * after validation.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, tile_id is invalid, data is a null pointer with a nonzero
 *   size, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom MVT vector
 *   source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_mvt_vector_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data MLN_BINDING("encoding=bytes"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reports a custom MVT vector source error for one canonical tile.
 *
 * source_id and the UTF-8 diagnostic message are borrowed for the call. The
 * function copies accepted bytes before return. An empty message is accepted.
 *
 * MapLibre ignores the error unless fetch_tile requested that tile and
 * cancel_tile has not cancelled it since. This call still returns MLN_STATUS_OK
 * after validation. The tile stays failed until the host delivers its data or
 * invalidates it, which fetches it again.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, tile_id is invalid, message is a null pointer with a
 *   nonzero size, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom MVT vector
 *   source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_mvt_vector_source_tile_error(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Invalidates custom MVT vector source data for one canonical tile.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, tile_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom MVT vector
 *   source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_invalidate_custom_mvt_vector_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets one runtime style image.
 *
 * image_id, image, and image pixels are borrowed for the call and copied
 * before return. If image_id already exists, the image is replaced when
 * the command commits.
 *
 * Runtime style images belong to the current style. Loading another style URL
 * or JSON document drops images that were added to the previous style.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, image_id is
 *   invalid or empty, image or options is invalid, image pixels are null, image
 *   dimensions or stride are invalid, or image byte_length is too small.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes one runtime style image by ID.
 *
 * image_id is borrowed for the call. The command commits when an image with
 * that ID existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, image_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no runtime style image has that ID.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_style_image(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies one complete runtime style image.
 *
 * A found image completes with one borrowed mln_style_image_info that includes
 * its pixels and stretch intervals. A missing image completes successfully with
 * no value. Copy retained pixels and intervals before the callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, image_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=query;result=mln_style_image_info;nullable=true")
MLN_API mln_status mln_map_get_style_image(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds an image source that loads its image from a URL.
 *
 * source_id, coordinates, and url are borrowed for the call. coordinates must
 * contain exactly four coordinates in top-left, top-right, bottom-right,
 * bottom-left order. The strings and coordinates are copied before return.
 * Later URL load or decode failures are reported through runtime events.
 *
 * Image sources belong to the current style. Loading another style URL or JSON
 * document drops sources that were added to the previous style.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id or url
 *   is invalid or empty, coordinates is null or invalid, or coordinate_count is
 *   not 4.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_image_source_url(
  mln_map map, mln_buffer_view source_id,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds an image source with inline image pixels.
 *
 * source_id, coordinates, image, and image pixels are borrowed for the call.
 * coordinates must contain exactly four coordinates in top-left, top-right,
 * bottom-right, bottom-left order. The coordinates and pixels are copied
 * before return.
 *
 * Image sources belong to the current style. Loading another style URL or JSON
 * document drops sources that were added to the previous style.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, coordinates is null or invalid, coordinate_count is not
 *   4, image is invalid, image pixels are null, image dimensions or stride are
 *   invalid, or image byte_length is too small.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Updates an image source to load its image from a URL.
 *
 * source_id and url are borrowed for the call. Later URL load or decode
 * failures are reported through runtime events.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id or url
 *   is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not an image source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Updates an image source with inline image pixels.
 *
 * source_id, image, and image pixels are borrowed for the call and copied
 * before return.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, image is invalid, image pixels are null, image dimensions
 *   or stride are invalid, image byte_length is too small, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not an image source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Updates image source coordinates.
 *
 * coordinates is borrowed for the call and copied before return, and must
 * contain exactly four coordinates in top-left, top-right, bottom-right,
 * bottom-left order.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, coordinates is null or invalid, coordinate_count is not
 *   4, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not an image source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies image source coordinates.
 *
 * The completion borrows four mln_lat_lng values in top-left, top-right,
 * bottom-right, bottom-left order.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not an image source.
 */
MLN_BINDING("execution=query;result=mln_lat_lng;shape=array")
MLN_API mln_status mln_map_get_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds one style layer from a full style-spec layer JSON object.
 *
 * layer_json and before_layer_id are borrowed for the call. layer_json must
 * contain id and type members. Passing an empty before_layer_id appends the
 * layer; otherwise the layer is inserted before that existing layer.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, layer_json is
 *   empty or invalid, before_layer_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when before_layer_id is non-empty and no style layer
 *   has it, or the layer requires a source no style source has.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer JSON cannot be converted, or a
 *   layer already has the JSON's id, or the source does not support the layer
 *   type.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json MLN_BINDING("encoding=json"),
  mln_buffer_view before_layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes one style layer by ID.
 *
 * layer_id is borrowed for the call. The command commits when a layer with
 * that ID existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_style_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the complete metadata of one style layer.
 *
 * A found layer completes with one borrowed mln_style_layer_info whose id is
 * the requested ID. A missing layer completes successfully with no value. Copy
 * retained views before the callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=query;result=mln_style_layer_info;nullable=true")
MLN_API mln_status mln_map_get_style_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered query of every style layer in style order.
 *
 * The completion borrows value_count mln_style_layer_info values, value_size
 * bytes apart, in style order. Copy retained entries and their views before the
 * callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=query;result=mln_style_layer_info;shape=array")
MLN_API mln_status mln_map_list_style_layers(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves one style layer before another layer or to the top.
 *
 * layer_id and before_layer_id are borrowed for the call. Passing an empty
 * before_layer_id moves the layer to the top of the style order.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, layer_id is
 *   invalid or empty, before_layer_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has layer_id, or before_layer_id
 *   is non-empty and no style layer has it.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Serializes one style layer as a full style-spec layer JSON object.
 *
 * A found layer completes with one borrowed mln_buffer_view. A missing layer
 * completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING(
  "execution=query;result=mln_buffer_view;nullable=true;"
  "encoding=json"
)
MLN_API mln_status mln_map_get_style_layer_json(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets the style light from a style-spec light JSON object.
 *
 * light_json is borrowed for the call.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, light_json is
 *   empty or invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the light JSON cannot be converted.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_light_json(
  mln_map map, mln_buffer_view light_json MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets one style light property using its MapLibre style-spec property name.
 *
 * property_name and value are borrowed for the call. value is a style-spec JSON
 * value.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, property_name is
 *   invalid or empty, value is empty or invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the property name is unknown, or the
 *   value cannot be converted for that property.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_light_property(
  mln_map map, mln_buffer_view property_name,
  mln_buffer_view value MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Serializes one style light property as a style-spec JSON value.
 *
 * A defined property completes with one borrowed mln_buffer_view. An undefined
 * property completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, property_name is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING(
  "execution=query;result=mln_buffer_view;nullable=true;"
  "encoding=json"
)
MLN_API mln_status mln_map_get_style_light_property(
  mln_map map, mln_buffer_view property_name, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets the style's global transition options.
 *
 * options is borrowed for the call and copied before return. When the command
 * commits, omitted duration and delay fields clear the style-wide override.
 * The command replaces the whole transition configuration.
 *
 * Loading a style replaces these options with the ones that style declares, so
 * a host that overrides them applies the override after the style loads.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, options is null
 *   or undersized, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when options->fields contains unknown bits, or
 *   an enabled duration or delay is negative, non-finite, or too long to
 *   represent. A rejected command changes nothing.
 * - MLN_STATUS_NATIVE_ERROR when applying the options fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the style's global transition options.
 *
 * The completion borrows the last-known transition configuration. Duration
 * and delay report through their field-mask bits and may be unset.
 *
 * A map that has loaded no style yet reports duration and delay unset. A style
 * carrying no "transition" member reports a 300 millisecond duration, while a
 * style carrying one reports only the members that object names, so a style
 * whose transition declares a delay alone reports no duration. Read the
 * field-mask bits rather than assuming a loaded style sets either.
 *
 * MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS is always set on
 * return and reports nothing about the field.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=query;result=mln_style_transition_options")
MLN_API mln_status mln_map_get_style_transition_options(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets one layer property using its MapLibre style-spec property name.
 *
 * layer_id, property_name, and value are borrowed for the call. value is a
 * style-spec JSON value; expressions use style-spec expression JSON arrays.
 * Paint and layout property names are unprefixed. The root keys filter,
 * source, source-layer, minzoom, and maxzoom are also supported.
 *
 * JSON null clears a filter and resets minzoom or maxzoom to an unbounded
 * range (-INFINITY or INFINITY). For paint and layout properties, JSON null
 * restores the property's default. source and source-layer take JSON strings;
 * an empty source-layer string clears it. A source ID may name a source that
 * has not been added yet. Zoom values are rounded to single precision.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, layer_id or
 *   property_name is invalid or empty, value is empty or invalid, or completion
 *   is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the property name is unknown for that
 *   layer, the value cannot be converted for that property, or source or
 *   source-layer is set on a layer type that takes no source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Serializes one layer property as a style-spec JSON value.
 *
 * A defined property completes with one borrowed mln_buffer_view. An undefined
 * property completes successfully with no value.
 *
 * Paint and layout property names are unprefixed. The root keys filter,
 * source, source-layer, minzoom, and maxzoom use the layer's serialized values.
 * An unset filter, an empty source or source-layer, and an unbounded minzoom
 * or maxzoom complete successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, layer_id or
 *   property_name is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_BINDING(
  "execution=query;result=mln_buffer_view;nullable=true;"
  "encoding=json"
)
MLN_API mln_status mln_map_get_style_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_STYLE_H
