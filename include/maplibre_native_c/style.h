/**
 * @file maplibre_native_c/style.h
 * Read and change the sources, layers, and images of a map's style.
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
 * Sets a value that the style's `global-state`
 * [expressions](https://maplibre.org/maplibre-style-spec/expressions/) read.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE until the map has parsed its style, which
 *   MLN_RUNTIME_EVENT_MAP_STYLE_LOADED reports, and again while it loads a new
 *   style URL.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_global_state_property(
  mln_map map, mln_buffer_view property_name,
  mln_buffer_view value MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/** Reads the global state of the map's style as a JSON object. */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_map_get_global_state(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** The type of a style source. */
typedef enum mln_style_source_type : uint32_t {
  MLN_STYLE_SOURCE_TYPE_UNKNOWN = 0,
  MLN_STYLE_SOURCE_TYPE_VECTOR = 1,
  MLN_STYLE_SOURCE_TYPE_RASTER = 2,
  MLN_STYLE_SOURCE_TYPE_RASTER_DEM = 3,
  MLN_STYLE_SOURCE_TYPE_GEO_JSON = 4,
  MLN_STYLE_SOURCE_TYPE_IMAGE = 5,
  MLN_STYLE_SOURCE_TYPE_VIDEO = 6,
  MLN_STYLE_SOURCE_TYPE_ANNOTATIONS = 7,
  MLN_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY = 8,
  MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR = 9,
} mln_style_source_type;

/** The fields that mln_style_source_info sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_source_info_field : uint32_t {
  MLN_STYLE_SOURCE_INFO_TILE_JSON = 1U << 0U,
  MLN_STYLE_SOURCE_INFO_BOUNDS = 1U << 1U,
  MLN_STYLE_SOURCE_INFO_TILE_SIZE = 1U << 2U,
  MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING = 1U << 3U,
  MLN_STYLE_SOURCE_INFO_RASTER_ENCODING = 1U << 4U,
} mln_style_source_info_field;

/** A set of mln_style_source_info_field bits. */
typedef uint32_t mln_style_source_info_fields;

/**
 * Whether a tileset counts tile rows from the north (XYZ) or the south (TMS).
 */
typedef enum mln_style_tile_scheme : uint32_t {
  MLN_STYLE_TILE_SCHEME_XYZ = 0,
  MLN_STYLE_TILE_SCHEME_TMS = 1,
} mln_style_tile_scheme;

/** The format of vector tiles. */
typedef enum mln_style_vector_tile_encoding : uint32_t {
  MLN_STYLE_VECTOR_TILE_ENCODING_MVT = 0,
  MLN_STYLE_VECTOR_TILE_ENCODING_MLT = 1,
} mln_style_vector_tile_encoding;

/** How a raster DEM tile encodes elevation in its colors. */
typedef enum mln_style_raster_dem_encoding : uint32_t {
  MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX = 0,
  MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM = 1,
} mln_style_raster_dem_encoding;

/** A range of image pixels, from `from` to `to`, that stretches to fit text. */
typedef struct mln_image_stretch {
  float from;
  float to;
} mln_image_stretch;

/** The area of an image, in image pixels, that text fits inside. */
typedef struct mln_image_content {
  float left;
  float top;
  float right;
  float bottom;
} mln_image_content;

/** How an image stretches to fit text along one axis. */
typedef enum mln_style_image_text_fit : uint32_t {
  MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK = 0,
  MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY = 1,
  MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL = 2,
} mln_style_image_text_fit;

/** Whether a layer shows. */
typedef enum mln_style_layer_visibility : uint32_t {
  MLN_STYLE_LAYER_VISIBILITY_VISIBLE = 0,
  MLN_STYLE_LAYER_VISIBILITY_NONE = 1,
} mln_style_layer_visibility;

/** The fields that mln_geo_json_source_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_geo_json_source_option_field : uint32_t {
  MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
  MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE = 1U << 2U,
  MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM = 1U << 3U,
  MLN_GEO_JSON_SOURCE_OPTION_BUFFER = 1U << 4U,
  MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS = 1U << 5U,
  MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS = 1U << 6U,
  MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS = 1U << 7U,
  MLN_GEO_JSON_SOURCE_OPTION_CLUSTER = 1U << 8U,
  MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING = 1U << 9U,
} mln_geo_json_source_option_field;

/** A set of mln_geo_json_source_option_field bits. */
typedef uint32_t mln_geo_json_source_option_fields;

/** The fields that mln_custom_geometry_source_options sets. */
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

/** The fields that mln_custom_vector_source_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_custom_vector_source_option_field : uint32_t {
  MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
} mln_custom_vector_source_option_field;

/** A set of mln_custom_vector_source_option_field bits. */
typedef uint32_t mln_custom_vector_source_option_fields;

/** The fields that mln_style_image_options sets. */
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
  MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED = 1U << 7U,
} mln_style_image_option_field;

/** A set of mln_style_image_option_field bits. */
typedef uint32_t mln_style_image_option_fields;

/** The fields that mln_style_image_info sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_style_image_info_field : uint32_t {
  MLN_STYLE_IMAGE_INFO_CONTENT = 1U << 0U,
  MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH = 1U << 1U,
  MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT = 1U << 2U,
} mln_style_image_info_field;

/** A set of mln_style_image_info_field bits. */
typedef uint32_t mln_style_image_info_fields;

/** The fields that mln_style_transition_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_transition_option_field : uint32_t {
  MLN_STYLE_TRANSITION_OPTION_DURATION = 1U << 0U,
  MLN_STYLE_TRANSITION_OPTION_DELAY = 1U << 1U,
  MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS = 1U << 2U,
} mln_style_transition_option_field;

/** A set of mln_style_transition_option_field bits. */
typedef uint32_t mln_style_transition_option_fields;

/** The tiles of a source, as the style lists them. */
typedef struct mln_style_source_tile_info {
  const mln_buffer_view* tile_urls MLN_BINDING("length=tile_url_count");
  size_t tile_url_count;
  double min_zoom;
  double max_zoom;
  mln_style_tile_scheme scheme;
} mln_style_source_tile_info;

/** A source in the map's style. */
typedef struct mln_style_source_info {
  mln_buffer_view id;
  mln_style_source_type type;
  mln_style_source_info_fields fields;
  bool is_volatile;
  mln_buffer_view attribution;
  mln_buffer_view url;
  /**
   * The tiles that the style lists in the source itself, instead of at a URL.
   */
  mln_style_source_tile_info tile_json
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_TILE_JSON");
  mln_lat_lng_bounds bounds
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_BOUNDS");
  uint32_t tile_size
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_TILE_SIZE");
  mln_style_vector_tile_encoding vector_encoding
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING");
  mln_style_raster_dem_encoding raster_encoding
    MLN_BINDING("mask=fields;bit=MLN_STYLE_SOURCE_INFO_RASTER_ENCODING");
} mln_style_source_info;

/** A layer in the map's style. */
typedef struct mln_style_layer_info {
  mln_buffer_view id;
  /**
   * The layer's [type](https://maplibre.org/maplibre-style-spec/layers/#type),
   * such as `fill`.
   */
  mln_buffer_view type;
  mln_buffer_view source_id;
  mln_buffer_view source_layer;
  /**
   * The zoom level where the layer starts to show, or negative infinity when
   * the layer doesn't set one.
   */
  double min_zoom;
  /**
   * The zoom level where the layer stops showing, or infinity when the layer
   * doesn't set one.
   */
  double max_zoom;
  mln_style_layer_visibility visibility;
} mln_style_layer_info;

/**
 * Options for a GeoJSON
 * [source](https://maplibre.org/maplibre-style-spec/sources/#geojson).
 *
 * Zoom levels are whole numbers.
 */
typedef struct mln_geo_json_source_options {
  uint32_t size;
  mln_geo_json_source_option_fields fields;
  double min_zoom
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM");
  double max_zoom
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM");
  double tolerance
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE");
  double cluster_max_zoom
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM");
  /** The `clusterProperties` object, or empty for none. */
  mln_buffer_view cluster_properties MLN_BINDING("encoding=json");
  uint32_t buffer
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_BUFFER");
  uint32_t cluster_radius
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS");
  uint32_t cluster_min_points MLN_BINDING(
    "mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS"
  );
  bool line_metrics
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS");
  bool cluster
    MLN_BINDING("mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_CLUSTER");
  /**
   * Builds tiles while the map renders instead of in the background, so that
   * new data shows in the next frame.
   */
  bool synchronous_tiling MLN_BINDING(
    "mask=fields;bit=MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING"
  );
} mln_geo_json_source_options;

/**
 * A tile's zoom level `z`, column `x`, and row `y`, with row 0 at the north.
 */
typedef struct mln_canonical_tile_id {
  uint32_t z;
  uint32_t x;
  uint32_t y;
} mln_canonical_tile_id;

/**
 * Asks for the data of a tile in a custom source.
 *
 * Return quickly. Once the data is ready, pass it to
 * mln_map_set_custom_geometry_source_tile_data() or
 * mln_map_set_custom_vector_source_tile_data(). If it fails to load, report
 * that with mln_map_set_custom_geometry_source_tile_error() or
 * mln_map_set_custom_vector_source_tile_error().
 */
typedef void (*mln_custom_source_fetch_tile_callback)(
  void* user_data, mln_canonical_tile_id tile_id
);

/** Reports that a custom source no longer needs a tile. */
typedef void (*mln_custom_source_cancel_tile_callback)(
  void* user_data, mln_canonical_tile_id tile_id
);

/**
 * The callbacks and options of a custom geometry source.
 *
 * Zoom levels are whole numbers from 0 to 32.
 */
typedef struct mln_custom_geometry_source_options {
  uint32_t size;
  mln_custom_geometry_source_option_fields fields;
  mln_custom_source_fetch_tile_callback fetch_tile;
  mln_custom_source_cancel_tile_callback cancel_tile
    MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  double min_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM");
  double max_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM");
  /** How far, in pixels, simplified geometry can stray from the original. */
  double tolerance
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE");
  /** The width of a tile in pixels, a power of two no larger than 8192. */
  uint32_t tile_size
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE");
  /**
   * The width in pixels of the margin around a tile that holds geometry from
   * beyond its edges.
   */
  uint32_t buffer
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER");
  /** Whether to cut geometry at the outer edge of a tile's margin. */
  bool clip
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP");
  /** Whether to wrap geometry that crosses the antimeridian. */
  bool wrap
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_custom_geometry_source_options MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * The callbacks and options of a custom vector source.
 *
 * Zoom levels are whole numbers from 0 to 32.
 */
typedef struct mln_custom_vector_source_options {
  uint32_t size;
  mln_custom_vector_source_option_fields fields;
  mln_custom_source_fetch_tile_callback fetch_tile;
  mln_custom_source_cancel_tile_callback cancel_tile
    MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  double min_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM");
  double max_zoom
    MLN_BINDING("mask=fields;bit=MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_custom_vector_source_options MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** An image of 8-bit RGBA pixels whose colors are multiplied by their alpha. */
typedef struct mln_premultiplied_rgba8_image {
  uint32_t size;
  uint32_t width MLN_BINDING("required=true");
  uint32_t height MLN_BINDING("required=true");
  /**
   * The number of bytes from the start of one row to the start of the next, at
   * least `width` * 4.
   */
  uint32_t stride MLN_BINDING("required=true");
  /** The rows of pixels, `stride` bytes apart, from the top row down. */
  const uint8_t* pixels
    MLN_BINDING("length=byte_length;encoding=bytes;required=true");
  size_t byte_length;
} mln_premultiplied_rgba8_image;

/**
 * Options for a style image, as the
 * [sprite](https://maplibre.org/maplibre-style-spec/sprite/) spec describes
 * them.
 */
typedef struct mln_style_image_options {
  uint32_t size;
  mln_style_image_option_fields fields;
  /**
   * The ranges of columns that stretch, in increasing order without overlap.
   */
  const mln_image_stretch* stretch_x MLN_BINDING(
    "length=stretch_x_count;mask=fields;"
    "bit=MLN_STYLE_IMAGE_OPTION_STRETCH_X"
  );
  size_t stretch_x_count;
  /** The ranges of rows that stretch, in increasing order without overlap. */
  const mln_image_stretch* stretch_y MLN_BINDING(
    "length=stretch_y_count;mask=fields;"
    "bit=MLN_STYLE_IMAGE_OPTION_STRETCH_Y"
  );
  size_t stretch_y_count;
  mln_image_content content
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_CONTENT");
  mln_style_image_text_fit text_fit_width
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH");
  mln_style_image_text_fit text_fit_height
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT");
  float pixel_ratio
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO");
  bool sdf MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_SDF");
  /**
   * Keeps the image after no layer uses it. Without this, the map can remove
   * an image that it asked for through
   * MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING.
   */
  bool keep_unused
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED");
} mln_style_image_options;

/** An image in the map's style. */
typedef struct mln_style_image_info {
  uint32_t width;
  uint32_t height;
  mln_style_image_info_fields fields;
  /**
   * The premultiplied RGBA pixels, in rows of `width` * 4 bytes, from the top
   * row down.
   */
  mln_buffer_view pixels MLN_BINDING("encoding=bytes");
  const mln_image_stretch* stretch_x MLN_BINDING("length=stretch_x_count");
  size_t stretch_x_count;
  const mln_image_stretch* stretch_y MLN_BINDING("length=stretch_y_count");
  size_t stretch_y_count;
  mln_image_content content
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_INFO_CONTENT");
  mln_style_image_text_fit text_fit_width
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH");
  mln_style_image_text_fit text_fit_height
    MLN_BINDING("mask=fields;bit=MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT");
  float pixel_ratio;
  bool sdf;
  bool keep_unused;
} mln_style_image_info;

/**
 * The default
 * [transition](https://maplibre.org/maplibre-style-spec/transition/) for
 * changes to a style's paint properties.
 */
typedef struct mln_style_transition_options {
  uint32_t size;
  mln_style_transition_option_fields fields;
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_STYLE_TRANSITION_OPTION_DURATION");
  double delay_ms
    MLN_BINDING("mask=fields;bit=MLN_STYLE_TRANSITION_OPTION_DELAY");
  /** Whether symbols fade in and out as the map places them. */
  bool enable_placement_transitions MLN_BINDING(
    "mask=fields;bit=MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS"
  );
} mln_style_transition_options;

/**
 * Adds a source to the map's style.
 *
 * `source_json` is a
 * [source](https://maplibre.org/maplibre-style-spec/sources/) object.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a source named
 *   `source_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_style_source_json(
  mln_map map, mln_buffer_view source_id,
  mln_buffer_view source_json MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes a source from the map's style.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_STATE when a layer still uses the source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads a source of the map's style.
 *
 * Completes with no value when the style has no source named `source_id`.
 */
MLN_BINDING("execution=query;result=mln_style_source_info;nullable=true")
MLN_API mln_status mln_map_get_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets whether a source is volatile.
 *
 * The map doesn't store the tiles of a volatile source in the persistent cache.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_source_volatile(
  mln_map map, mln_buffer_view source_id, bool is_volatile,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Lists the sources of the map's style. */
MLN_BINDING("execution=query;result=mln_style_source_info;shape=array")
MLN_API mln_status mln_map_list_style_sources(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds a GeoJSON source that loads its data from `url`.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a source named
 *   `source_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_geo_json_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geo_json_source_options* options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Prepares GeoJSON data for GeoJSON sources.
 *
 * `data` is a GeoJSON feature collection, feature, or geometry. Preparing it
 * can be slow, but adding the result to a source is quick, and several sources
 * can share it.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `data` isn't valid GeoJSON, or when
 *   `options` turns on clustering and `data` isn't a feature collection of
 *   points.
 */
MLN_API mln_status mln_geo_json_source_data_create(
  mln_buffer_view data MLN_BINDING("encoding=json"),
  const mln_geo_json_source_options* options MLN_BINDING("nullable=true"),
  mln_geo_json_source_data* out_data MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases prepared GeoJSON data.
 *
 * Sources that use the data keep it.
 */
MLN_API void mln_geo_json_source_data_release(
  mln_geo_json_source_data data
) MLN_NOEXCEPT;

/**
 * Adds a GeoJSON source with prepared data.
 *
 * The source uses the options that `data` was prepared with. `data` can be
 * released once this function returns.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a source named
 *   `source_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_geo_json_source_data(
  mln_map map, mln_buffer_view source_id, mln_geo_json_source_data data,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Makes a GeoJSON source load its data from `url`.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   GeoJSON source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_geo_json_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the data of a GeoJSON source.
 *
 * `data` must have been prepared with the same options as the source. It can be
 * released once this function returns.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   GeoJSON source, or `data` was prepared with other options.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_geo_json_source_data(
  mln_map map, mln_buffer_view source_id, mln_geo_json_source_data data,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets whether a GeoJSON source builds its tiles synchronously.
 *
 * This works like the synchronous tiling option of mln_geo_json_source_options,
 * but `enabled` can't turn off synchronous tiling that the source's options
 * turned on.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   GeoJSON source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_geo_json_source_synchronous_tiling(
  mln_map map, mln_buffer_view source_id, bool enabled,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds a source whose GeoJSON tiles come from callbacks.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a source named
 *   `source_id`.
 *
 * Lifetimes:
 * When this function returns an error, the caller keeps `user_data` and the
 * library doesn't call `release_user_data`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets the data of a tile in a custom geometry source.
 *
 * `data` is a GeoJSON feature collection. The source ignores data for a tile
 * that it hasn't asked for, or has since cancelled.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   custom geometry source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reports that a tile of a custom geometry source failed to load.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   custom geometry source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_geometry_source_tile_error(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Makes a custom geometry source ask for a tile again when the map needs it.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   custom geometry source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Makes a custom geometry source ask again for the tiles in `bounds` that the
 * map still needs.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   custom geometry source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds a source whose vector tiles come from callbacks.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a source named
 *   `source_id`.
 *
 * Lifetimes:
 * When this function returns an error, the caller keeps `user_data` and the
 * library doesn't call `release_user_data`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_custom_vector_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_vector_source_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets the data of a tile in a custom vector source.
 *
 * `data` is the tile, encoded as `encoding`. The source ignores data for a tile
 * that it hasn't asked for, or has since cancelled.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   custom vector source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_vector_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_style_vector_tile_encoding encoding,
  mln_buffer_view data MLN_BINDING("encoding=bytes"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reports that a tile of a custom vector source failed to load.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   custom vector source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_custom_vector_source_tile_error(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Makes a custom vector source ask for a tile again when the map needs it.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't a
 *   custom vector source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_invalidate_custom_vector_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Adds an image to the map's style, or replaces the image named `image_id`. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes an image from the map's style.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no image named `image_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_style_image(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads an image of the map's style.
 *
 * Completes with no value when the style has no image named `image_id`.
 */
MLN_BINDING("execution=query;result=mln_style_image_info;nullable=true")
MLN_API mln_status mln_map_get_style_image(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds an image source that loads its image from `url`.
 *
 * `coordinates` are the image's four corners, clockwise from the top left.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a source named
 *   `source_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_image_source_url(
  mln_map map, mln_buffer_view source_id,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds an image source that shows `image`.
 *
 * `coordinates` are the image's four corners, clockwise from the top left.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a source named
 *   `source_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Makes an image source load its image from `url`.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't an
 *   image source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the image of an image source.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't an
 *   image source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves the image of an image source.
 *
 * `coordinates` are the image's four corners, clockwise from the top left.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't an
 *   image source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the corners of an image source's image, clockwise from the top left.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when the source named `source_id` isn't an
 *   image source.
 */
MLN_BINDING("execution=query;result=mln_lat_lng;shape=array")
MLN_API mln_status mln_map_get_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds a layer to the map's style.
 *
 * `layer_json` is a [layer](https://maplibre.org/maplibre-style-spec/layers/)
 * object. The layer goes below the layer named `before_layer_id`, or above
 * every layer when `before_layer_id` is empty.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no layer named `before_layer_id`,
 *   or no source with the ID that the layer uses.
 * - MLN_STATUS_INVALID_ARGUMENT when the style already has a layer with the new
 *   layer's ID.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json MLN_BINDING("encoding=json"),
  mln_buffer_view before_layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes a layer from the map's style.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no layer named `layer_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_style_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads a layer of the map's style.
 *
 * Completes with no value when the style has no layer named `layer_id`.
 */
MLN_BINDING("execution=query;result=mln_style_layer_info;nullable=true")
MLN_API mln_status mln_map_get_style_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Lists the layers of the map's style, from bottom to top. */
MLN_BINDING("execution=query;result=mln_style_layer_info;shape=array")
MLN_API mln_status mln_map_list_style_layers(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves a layer of the map's style below the layer named `before_layer_id`, or
 * above every layer when `before_layer_id` is empty.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no layer named `layer_id` or
 *   `before_layer_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads a layer of the map's style as a
 * [layer](https://maplibre.org/maplibre-style-spec/layers/) object.
 *
 * Completes with no value when the style has no layer named `layer_id`.
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
 * Replaces the [light](https://maplibre.org/maplibre-style-spec/light/) of the
 * map's style.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_light_json(
  mln_map map, mln_buffer_view light_json MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Sets a property of the light of the map's style. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_light_property(
  mln_map map, mln_buffer_view property_name,
  mln_buffer_view value MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads a property of the light of the map's style.
 *
 * Completes with no value when the light doesn't set the property.
 */
MLN_BINDING(
  "execution=query;result=mln_buffer_view;nullable=true;"
  "encoding=json"
)
MLN_API mln_status mln_map_get_style_light_property(
  mln_map map, mln_buffer_view property_name, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Sets the default transition of the map's style. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Reads the default transition of the map's style. */
MLN_BINDING("execution=query;result=mln_style_transition_options")
MLN_API mln_status mln_map_get_style_transition_options(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets a property of a layer in the map's style.
 *
 * `property_name` is one of the layer's layout or paint
 * [properties](https://maplibre.org/maplibre-style-spec/layers/), or `filter`,
 * `minzoom`, `maxzoom`, `source`, or `source-layer`. A null `value` resets any
 * of these but `source` and `source-layer` to its default.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no layer named `layer_id`.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads a property of a layer in the map's style.
 *
 * Completes with no value when the layer doesn't set the property.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no layer named `layer_id`.
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
