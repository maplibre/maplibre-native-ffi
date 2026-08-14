/**
 * @file maplibre_native_c/style.h
 * Public C API declarations for style sources, layers, and images.
 */

#ifndef MAPLIBRE_NATIVE_C_STYLE_H
#define MAPLIBRE_NATIVE_C_STYLE_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t mln_style_id_list;
typedef uint64_t mln_style_string_list;
typedef uint64_t mln_geojson_source_data;

/** Style source type values returned by mln_map_get_style_source_type(). */
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
typedef enum mln_style_source_info_field : uint32_t {
  /** The source retains a URL. Copy it with mln_map_copy_style_source_url(). */
  MLN_STYLE_SOURCE_INFO_URL = 1U << 0U,
  /** The tile source was defined with an inline TileJSON description. */
  MLN_STYLE_SOURCE_INFO_TILEJSON = 1U << 1U,
  /** The inline TileJSON description contains geographic bounds. */
  MLN_STYLE_SOURCE_INFO_BOUNDS = 1U << 2U,
  /** The source exposes a tile size. */
  MLN_STYLE_SOURCE_INFO_TILE_SIZE = 1U << 3U,
  /** The source exposes a vector tile encoding. */
  MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING = 1U << 4U,
  /** The source exposes a DEM raster encoding. */
  MLN_STYLE_SOURCE_INFO_RASTER_ENCODING = 1U << 5U,
} mln_style_source_info_field;

/** Field mask values for mln_style_tile_source_options. */
typedef enum mln_style_tile_source_option_field : uint32_t {
  MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
  MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION = 1U << 2U,
  MLN_STYLE_TILE_SOURCE_OPTION_SCHEME = 1U << 3U,
  MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS = 1U << 4U,
  MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE = 1U << 5U,
  MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING = 1U << 6U,
  MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING = 1U << 7U,
} mln_style_tile_source_option_field;

/** Tile URL coordinate scheme values used by mln_style_tile_source_options. */
typedef enum mln_style_tile_scheme : uint32_t {
  MLN_STYLE_TILE_SCHEME_XYZ = 0,
  MLN_STYLE_TILE_SCHEME_TMS = 1,
} mln_style_tile_scheme;

/** Vector tile encoding values used by mln_style_tile_source_options. */
typedef enum mln_style_vector_tile_encoding : uint32_t {
  MLN_STYLE_VECTOR_TILE_ENCODING_MVT = 0,
  MLN_STYLE_VECTOR_TILE_ENCODING_MLT = 1,
} mln_style_vector_tile_encoding;

/** DEM raster encoding values used by mln_style_tile_source_options. */
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

/** Layer visibility values used by the layer visibility accessors. */
typedef enum mln_style_layer_visibility : uint32_t {
  MLN_STYLE_LAYER_VISIBILITY_VISIBLE = 0,
  MLN_STYLE_LAYER_VISIBILITY_NONE = 1,
} mln_style_layer_visibility;

/** Field mask values for mln_geojson_source_options. */
typedef enum mln_geojson_source_option_field : uint32_t {
  MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
  MLN_GEOJSON_SOURCE_OPTION_TOLERANCE = 1U << 2U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM = 1U << 3U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES = 1U << 4U,
  MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE = 1U << 5U,
  MLN_GEOJSON_SOURCE_OPTION_BUFFER = 1U << 6U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS = 1U << 7U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS = 1U << 8U,
  MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS = 1U << 9U,
  MLN_GEOJSON_SOURCE_OPTION_CLUSTER = 1U << 10U,
  MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING = 1U << 11U,
} mln_geojson_source_option_field;

/** Field mask values for mln_custom_geometry_source_options. */
typedef enum mln_custom_geometry_source_option_field : uint32_t {
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE = 1U << 2U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE = 1U << 3U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER = 1U << 4U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP = 1U << 5U,
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP = 1U << 6U,
} mln_custom_geometry_source_option_field;

/** Field mask values for mln_style_image_options. */
typedef enum mln_style_image_option_field : uint32_t {
  MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO = 1U << 0U,
  MLN_STYLE_IMAGE_OPTION_SDF = 1U << 1U,
  MLN_STYLE_IMAGE_OPTION_STRETCH_X = 1U << 2U,
  MLN_STYLE_IMAGE_OPTION_STRETCH_Y = 1U << 3U,
  MLN_STYLE_IMAGE_OPTION_CONTENT = 1U << 4U,
  MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH = 1U << 5U,
  MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT = 1U << 6U,
} mln_style_image_option_field;

/** Field mask values for mln_style_transition_options. */
typedef enum mln_style_transition_option_field : uint32_t {
  MLN_STYLE_TRANSITION_OPTION_DURATION = 1U << 0U,
  MLN_STYLE_TRANSITION_OPTION_DELAY = 1U << 1U,
  MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS = 1U << 2U,
} mln_style_transition_option_field;

/** Location indicator image-name properties. */
typedef enum mln_location_indicator_image_kind : uint32_t {
  MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP = 0,
  MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING = 1,
  MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW = 2,
} mln_location_indicator_image_kind;

/** Fixed source metadata returned by mln_map_get_style_source_info(). */
typedef struct mln_style_source_info {
  uint32_t size;
  /** One of mln_style_source_type. */
  uint32_t type;
  /** Bitwise combination of mln_style_source_info_field values. */
  uint32_t fields;
  /** Source ID byte length, excluding any null terminator. */
  size_t id_size;
  bool is_volatile;
  bool has_attribution;
  /** Attribution byte length, excluding any null terminator. */
  size_t attribution_size;
  /** URL byte length, meaningful when fields contains URL. */
  size_t url_size;
  /** Inline tile URL count, meaningful when fields contains TILEJSON. */
  size_t tile_count;
  /** Minimum zoom, meaningful when fields contains TILEJSON. */
  double min_zoom;
  /** Maximum zoom, meaningful when fields contains TILEJSON. */
  double max_zoom;
  /** One of mln_style_tile_scheme, meaningful when fields contains TILEJSON. */
  uint32_t scheme;
  /** Geographic bounds, meaningful when fields contains BOUNDS. */
  mln_lat_lng_bounds bounds;
  /** Tile size in pixels, meaningful when fields contains TILE_SIZE. */
  uint32_t tile_size;
  /** Vector encoding, meaningful when fields contains VECTOR_ENCODING. */
  uint32_t vector_encoding;
  /** DEM encoding, meaningful when fields contains RASTER_ENCODING. */
  uint32_t raster_encoding;
} mln_style_source_info;

/** Options for vector and raster tile sources. */
typedef struct mln_style_tile_source_options {
  uint32_t size;
  uint32_t fields;
  double min_zoom;
  double max_zoom;
  mln_buffer_view attribution;
  /** One of mln_style_tile_scheme. Defaults to MLN_STYLE_TILE_SCHEME_XYZ. */
  uint32_t scheme;
  mln_lat_lng_bounds bounds;
  /** Raster tile size in pixels. Defaults to 512. */
  uint32_t tile_size;
  /** One of mln_style_vector_tile_encoding. Defaults to MVT. */
  uint32_t vector_encoding;
  /** One of mln_style_raster_dem_encoding. Defaults to Mapbox. */
  uint32_t raster_encoding;
} mln_style_tile_source_options;

/**
 * Options for GeoJSON sources.
 *
 * MapLibre Native fixes these options when the source is created, so
 * mln_map_set_geojson_source_url() keeps the options the source was added
 * with, and mln_map_set_geojson_source_data() requires data prepared with
 * matching options.
 */
typedef struct mln_geojson_source_options {
  uint32_t size;
  uint32_t fields;
  /** Minimum tiling zoom. Defaults to 0. */
  double min_zoom;
  /** Maximum tiling zoom. Defaults to 18. */
  double max_zoom;
  /** Douglas-Peucker simplification tolerance. Defaults to 0.375. */
  double tolerance;
  /** Highest zoom that clusters points. Defaults to 17. */
  double cluster_max_zoom;
  /**
   * Cluster aggregation expressions keyed by property name, as a JSON object
   * whose members follow the MapLibre Style Spec clusterProperties form. The
   * UTF-8 bytes are borrowed for the call.
   */
  mln_buffer_view cluster_properties;
  /** Tile extent in pixels. Defaults to 512. */
  uint32_t tile_size;
  /** Tile buffer in pixels. Defaults to 128. */
  uint32_t buffer;
  /** Cluster radius in pixels. Defaults to 50. */
  uint32_t cluster_radius;
  /** Points required to form a cluster. Defaults to 2. */
  uint32_t cluster_min_points;
  /** Adds line distance metrics to line features. Defaults to false. */
  bool line_metrics;
  /**
   * Clusters point features. Defaults to false.
   *
   * Clustering applies to feature collections whose every feature carries point
   * geometry. MapLibre Native clusters feature collections only, so
   * mln_geojson_source_data_create() rejects a bare geometry or a single
   * feature, along with a feature collection that mixes in other geometry. An
   * empty feature collection stays accepted and carries nothing to cluster.
   */
  bool cluster;
  /**
   * Slices requested tiles inline during the update pass. Defaults to false.
   *
   * MapLibre Native normally slices tiles out of the prepared data index on a
   * worker and shows them in a later frame. When this is set, slicing runs
   * inline, so data installed through mln_map_set_geojson_source_data() reaches
   * the next rendered frame at the cost of that work running on the update
   * thread. mln_map_set_geojson_source_synchronous_tiling() overrides this at
   * runtime.
   */
  bool synchronous_tiling;
} mln_geojson_source_options;

/** Canonical tile identity used by custom geometry source callbacks. */
typedef struct mln_canonical_tile_id {
  uint32_t z;
  uint32_t x;
  uint32_t y;
} mln_canonical_tile_id;

/** Callback invoked for custom geometry source tile requests and cancels. */
typedef void (*mln_custom_geometry_source_tile_callback)(
  void* user_data, mln_canonical_tile_id tile_id
);

/** Releases a custom geometry source's callback context. */
typedef void (*mln_custom_geometry_source_release_callback)(void* user_data);

/** Options for custom geometry sources. */
typedef struct mln_custom_geometry_source_options {
  uint32_t size;
  uint32_t fields;
  /** Required tile fetch callback. */
  mln_custom_geometry_source_tile_callback fetch_tile;
  /** Optional best-effort tile cancel callback. */
  mln_custom_geometry_source_tile_callback cancel_tile;
  /** Caller-owned callback context retained by pointer. */
  void* user_data;
  double min_zoom;
  double max_zoom;
  double tolerance;
  uint32_t tile_size;
  uint32_t buffer;
  bool clip;
  bool wrap;
  /**
   * Optional. Invoked once when this API stops referencing user_data.
   *
   * The call runs on the map owner thread when the source is removed
   * explicitly, when a style load replaces the style that held the source, or
   * when the map is destroyed. It runs at most once, and it does not run when
   * adding the source failed. A host frees its callback state here. Null means
   * the host needs no release.
   *
   * This callback must not destroy its map, because a release that a style load
   * drives runs inside MapLibre's style-load dispatch. Free callback state and
   * return. A host that wants to destroy the map does so from its own call
   * after the pump that reported the load returns.
   */
  mln_custom_geometry_source_release_callback release_user_data;
} mln_custom_geometry_source_options;

/** Caller-owned premultiplied RGBA8 image pixels. */
typedef struct mln_premultiplied_rgba8_image {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  /** Bytes per image row. Must be at least width * 4. */
  uint32_t stride;
  /** Premultiplied RGBA8 pixels. Must not be null for a non-empty image. */
  const uint8_t* pixels;
  /** Available bytes at pixels. */
  size_t byte_length;
} mln_premultiplied_rgba8_image;

/** Options for runtime style images. */
typedef struct mln_style_image_options {
  uint32_t size;
  uint32_t fields;
  /**
   * Horizontally stretchable intervals. Borrowed for the call and copied before
   * return. May be null only when stretch_x_count is 0.
   */
  const mln_image_stretch* stretch_x;
  size_t stretch_x_count;
  /**
   * Vertically stretchable intervals. Borrowed for the call and copied before
   * return. May be null only when stretch_y_count is 0.
   */
  const mln_image_stretch* stretch_y;
  size_t stretch_y_count;
  /** Content box used when icon-text-fit applies. */
  mln_image_content content;
  /** One of mln_style_image_text_fit. Defaults to STRETCH_OR_SHRINK. */
  uint32_t text_fit_width;
  /** One of mln_style_image_text_fit. Defaults to STRETCH_OR_SHRINK. */
  uint32_t text_fit_height;
  /** Sprite pixel ratio. Defaults to 1. */
  float pixel_ratio;
  /** Whether the image is a signed distance field icon. Defaults to false. */
  bool sdf;
} mln_style_image_options;

/** Fixed metadata for one runtime style image. */
typedef struct mln_style_image_info {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  /** Native copied images are exposed as tightly packed premultiplied RGBA8. */
  uint32_t stride;
  size_t byte_length;
  /**
   * Interval counts for the stretchable axes. Read the intervals themselves
   * with mln_map_copy_style_image_stretches().
   */
  size_t stretch_x_count;
  size_t stretch_y_count;
  /** Content box, meaningful only when has_content is true. */
  mln_image_content content;
  /** One of mln_style_image_text_fit, meaningful only when its flag is true. */
  uint32_t text_fit_width;
  /** One of mln_style_image_text_fit, meaningful only when its flag is true. */
  uint32_t text_fit_height;
  float pixel_ratio;
  bool sdf;
  bool has_content;
  bool has_text_fit_width;
  bool has_text_fit_height;
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
  uint32_t fields;
  /**
   * Transition duration in milliseconds. Must be finite and non-negative.
   * Values that would overflow MapLibre Native's internal duration are invalid.
   *
   * When this field is omitted, paint property changes apply instantly, and
   * MapLibre Native's own 300 millisecond default governs the symbol placement
   * cross-fade that enable_placement_transitions gates. A continuous-mode map
   * applies that same default to the pattern cross-fade across integer zoom
   * levels, while a still-mode map fades those patterns instantly instead.
   *
   * A still-mode map ignores this field and delay_ms for paint property
   * transitions, and renders each still image at a fixed time point, so a
   * property that takes its transition from these reaches its final value in
   * the next still image whatever they hold.
   */
  double duration_ms;
  /**
   * Transition delay in milliseconds. Must be finite and non-negative. Values
   * that would overflow MapLibre Native's internal duration are invalid.
   *
   * When this field is omitted, transitions start without delay.
   */
  double delay_ms;
  /**
   * Whether symbol placement changes cross-fade.
   *
   * Clearing this makes symbol placement changes apply to the next rendered
   * frame, in every map mode. Hosts that move symbol-backed features at pointer
   * frequency clear it for the duration of the interaction.
   *
   * When this field is omitted, the cross-fade stays on, which is MapLibre
   * Native's own default. A style carries no equivalent, so loading a style
   * leaves the cross-fade on.
   */
  bool enable_placement_transitions;
} mln_style_transition_options;

/** Returns default tile source options. */
MLN_API mln_style_tile_source_options
mln_style_tile_source_options_default(void) MLN_NOEXCEPT;

/** Returns default GeoJSON source options. */
MLN_API mln_geojson_source_options
mln_geojson_source_options_default(void) MLN_NOEXCEPT;

/** Returns default custom geometry source options. */
MLN_API mln_custom_geometry_source_options
mln_custom_geometry_source_options_default(void) MLN_NOEXCEPT;

/** Returns a default premultiplied RGBA8 image descriptor. */
MLN_API mln_premultiplied_rgba8_image
mln_premultiplied_rgba8_image_default(void) MLN_NOEXCEPT;

/** Returns default runtime style image options. */
MLN_API mln_style_image_options
mln_style_image_options_default(void) MLN_NOEXCEPT;

/** Returns default runtime style image metadata. */
MLN_API mln_style_image_info mln_style_image_info_default(void) MLN_NOEXCEPT;

/** Returns default global style transition options. */
MLN_API mln_style_transition_options
mln_style_transition_options_default(void) MLN_NOEXCEPT;

/**
 * Gets the number of IDs in a style ID list handle.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, or out_count is
 *   null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_style_id_list_count(mln_style_id_list list, size_t* out_count) MLN_NOEXCEPT;

/**
 * Borrows one ID from a style ID list handle.
 *
 * On success, out_id receives a view into list-owned storage. The view remains
 * valid until the list is destroyed.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, index is out of
 *   range, or out_id is null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_style_id_list_get(
  mln_style_id_list list, size_t index, mln_buffer_view* out_id
) MLN_NOEXCEPT;

/** Destroys a style ID list handle. Null is accepted as a no-op. */
MLN_API void mln_style_id_list_destroy(mln_style_id_list list) MLN_NOEXCEPT;

/**
 * Gets the number of strings in a style string list handle.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, or out_count is
 *   null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_style_string_list_count(
  mln_style_string_list list, size_t* out_count
) MLN_NOEXCEPT;

/**
 * Borrows one string from a style string list handle.
 *
 * On success, out_value receives a view into list-owned storage. The view
 * remains valid until the list is destroyed.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, index is out of
 *   range, or out_value is null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_style_string_list_get(
  mln_style_string_list list, size_t index, mln_buffer_view* out_value
) MLN_NOEXCEPT;

/** Destroys a style string list handle. Null is accepted as a no-op. */
MLN_API void mln_style_string_list_destroy(
  mln_style_string_list list
) MLN_NOEXCEPT;

/**
 * Adds one style source from a style-spec source JSON object.
 *
 * source_id and source_json are borrowed for the call. source_json is the
 * object that appears under sources[source_id] in a style document. The
 * function parses and copies the accepted source into MapLibre Native before
 * return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, source_json is empty or invalid, the source ID already
 *   exists, or the source JSON cannot be converted.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_style_source_json(
  mln_map map, mln_buffer_view source_id, mln_buffer_view source_json
) MLN_NOEXCEPT;

/**
 * Removes one style source by ID.
 *
 * source_id is borrowed for the call. On success, out_removed reports whether a
 * source existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or out_removed is null.
 * - MLN_STATUS_INVALID_STATE when the source exists but a layer still uses it.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_remove_style_source(
  mln_map map, mln_buffer_view source_id, bool* out_removed
) MLN_NOEXCEPT;

/**
 * Reports whether a style source ID exists.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or out_exists is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_style_source_exists(
  mln_map map, mln_buffer_view source_id, bool* out_exists
) MLN_NOEXCEPT;

/**
 * Gets one style source type.
 *
 * On success, out_found reports whether source_id exists. When found,
 * out_source_type receives one of mln_style_source_type.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, out_source_type is null, or out_found is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_source_type(
  mln_map map, mln_buffer_view source_id, uint32_t* out_source_type,
  bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies fixed metadata for one style source.
 *
 * The returned struct contains string lengths and fixed inline TileJSON
 * fields, not string contents. Use
 * mln_map_copy_style_source_attribution() and
 * mln_map_copy_style_source_url() to copy individual strings, and
 * mln_map_get_style_source_tile_urls() to copy inline tile URLs. The source ID
 * is the lookup key and is also available through style source ID lists.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, out_info is null, out_info->size is too small, or
 *   out_found is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_source_info(
  mln_map map, mln_buffer_view source_id, mln_style_source_info* out_info,
  bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies one style source attribution string into caller-owned memory.
 *
 * source_id is borrowed for the call. On success, out_attribution_size receives
 * the byte length of the attribution, excluding any null terminator. When
 * out_found is false or the source has no attribution, out_attribution_size
 * receives 0.
 *
 * Passing null for out_attribution with a capacity of 0 is a size probe: it
 * reports the required byte length and succeeds, so a caller can size a buffer
 * without treating the result as a failure. With a non-null out_attribution, a
 * capacity smaller than the required length still reports that length and
 * returns MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, out_attribution is null with non-zero capacity,
 *   attribution_capacity is too small for a non-null buffer,
 *   out_attribution_size is null, or out_found is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_source_attribution(
  mln_map map, mln_buffer_view source_id, char* out_attribution,
  size_t attribution_capacity, size_t* out_attribution_size, bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies one style source URL into caller-owned memory.
 *
 * source_id is borrowed for the call. On success, out_url_size receives the URL
 * byte length, excluding any null terminator. When out_found is false or the
 * source has no URL, out_url_size receives 0.
 *
 * Passing null for out_url with a capacity of 0 is a size probe. It reports the
 * required byte length and succeeds. With a non-null out_url, a capacity
 * smaller than the required length still reports that length and returns
 * MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, out_url is null with non-zero capacity, url_capacity is
 *   too small for a non-null buffer, out_url_size is null, or out_found is
 *   null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_source_url(
  mln_map map, mln_buffer_view source_id, char* out_url, size_t url_capacity,
  size_t* out_url_size, bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies one style source's inline TileJSON tile URLs into an owned list.
 *
 * On success, out_found reports whether source_id exists. When found,
 * *out_tile_urls receives an owned list. A URL-backed tile source and every
 * source without inline TileJSON return an empty list. Loading a URL-backed
 * source does not change this result. Destroy the list with
 * mln_style_string_list_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, out_tile_urls is null, *out_tile_urls is not null, or
 *   out_found is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_source_tile_urls(
  mln_map map, mln_buffer_view source_id, mln_style_string_list* out_tile_urls,
  bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies style source IDs in style order.
 *
 * On success, *out_source_ids receives an owned list handle. Destroy it with
 * mln_style_id_list_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, out_source_ids is
 *   null, or *out_source_ids is not null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_list_style_source_ids(
  mln_map map, mln_style_id_list* out_source_ids
) MLN_NOEXCEPT;

/**
 * Adds a GeoJSON source with URL data.
 *
 * source_id, url, and options are borrowed for the call. The source loads
 * GeoJSON from url through MapLibre Native's resource system. options may be
 * null for defaults, and the options are fixed for the lifetime of the source.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, options is invalid, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geojson_source_options* options
) MLN_NOEXCEPT;

/**
 * Prepares GeoJSON source data for installation on a map.
 *
 * data and options are borrowed for the call. The UTF-8 GeoJSON bytes are
 * parsed and tiled (or clustered) into the index a GeoJSON source consumes,
 * which is the expensive part of a data update. options may be null for
 * defaults; the options are baked into the prepared data and must match the
 * options of every source the data is installed on.
 *
 * When options enable clustering, the data must be a feature collection whose
 * every feature carries point geometry. Data that does not is rejected, and
 * the thread-local diagnostic names the constraint.
 *
 * This entry point is callable from any thread and touches no runtime or map,
 * so a host prepares data on a worker thread and installs it on the map owner
 * thread. The prepared data is immutable; create, read, and destroy may each
 * happen on different threads.
 *
 * *out_data must be MLN_HANDLE_NULL on entry. On success it receives an owned
 * handle the host releases with mln_geojson_source_data_destroy(). Installing
 * the data borrows the handle, so one prepared handle may be installed on any
 * number of sources and destroyed at any time afterward.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when data is empty or invalid, options is
 *   invalid, out_data is null or *out_data is not MLN_HANDLE_NULL, or
 *   clustering is enabled and the data is a bare geometry or a single feature,
 *   or a feature carries geometry other than a point.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_geojson_source_data_create(
  mln_buffer_view data, const mln_geojson_source_options* options,
  mln_geojson_source_data* out_data
) MLN_NOEXCEPT;

/**
 * Releases prepared GeoJSON source data.
 *
 * Callable from any thread. A null or already-released handle is a no-op.
 * Sources the data was installed on keep their own reference, so destroying
 * the handle never invalidates a source.
 */
MLN_API void mln_geojson_source_data_destroy(
  mln_geojson_source_data data
) MLN_NOEXCEPT;

/**
 * Adds a GeoJSON source with prepared inline data.
 *
 * source_id is borrowed for the call. data names a live handle from
 * mln_geojson_source_data_create(); the call borrows the handle and retains
 * the prepared index, and the source adopts the options the data was prepared
 * with, fixed for the lifetime of the source.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, data is null or not live, or the source ID already
 *   exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data
) MLN_NOEXCEPT;

/**
 * Updates one GeoJSON source to load data from a URL.
 *
 * source_id and url are borrowed for the call. The source keeps the
 * mln_geojson_source_options it was added with.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, the source does not exist, or the source is not a
 *   GeoJSON source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url
) MLN_NOEXCEPT;

/**
 * Updates one GeoJSON source with prepared inline data.
 *
 * source_id is borrowed for the call. data names a live handle from
 * mln_geojson_source_data_create(); the call borrows the handle and retains
 * the prepared index, which makes this a cheap install: the expensive parse
 * and tiling already happened when the data was prepared. Requested tiles are
 * still sliced out of the index on a worker unless synchronous tiling is on.
 *
 * The data must have been prepared with options equal to the options the
 * source was added with. Cluster aggregation expressions compare by parsed
 * equality, so equivalent cluster_properties JSON matches regardless of
 * formatting. A mismatch is rejected, because MapLibre Native fixes a source's
 * options at creation and data prepared under different options would tile
 * inconsistently with them.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, data is null or not live, the source does not exist, the
 *   source is not a GeoJSON source, or the data was prepared with options that
 *   do not match the source's options.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data
) MLN_NOEXCEPT;

/**
 * Overrides one GeoJSON source's synchronous tiling at runtime.
 *
 * source_id is borrowed for the call. While enabled is true, the source slices
 * requested tiles inline during the update pass, as if the source's options
 * had set synchronous_tiling; false restores the option the source was added
 * with. The override applies to update passes after this call returns. Hosts
 * enable it around high-frequency small updates, such as a tracked position,
 * so each installed update reaches the next rendered frame.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, the source does not exist, or the source is not a
 *   GeoJSON source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_geojson_source_synchronous_tiling(
  mln_map map, mln_buffer_view source_id, bool enabled
) MLN_NOEXCEPT;

/**
 * Adds a vector source with a TileJSON URL.
 *
 * source_id and url are borrowed for the call. options may be null for
 * defaults. For URL sources, min_zoom, max_zoom, and vector_encoding override
 * values from the loaded TileJSON when their field bits are set.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, options is invalid, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_vector_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) MLN_NOEXCEPT;

/**
 * Adds a vector source with inline tile URLs.
 *
 * source_id and tile URL views are borrowed for the call. The function copies
 * accepted strings into MapLibre Native before return. options may be null for
 * defaults.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile URLs are null, empty, or invalid, options is
 *   invalid, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_vector_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) MLN_NOEXCEPT;

/**
 * Adds a raster source with a TileJSON URL.
 *
 * source_id and url are borrowed for the call. options may be null for
 * defaults. For URL sources, only tile_size is used when its field bit is set.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, options is invalid, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_raster_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) MLN_NOEXCEPT;

/**
 * Adds a raster source with inline tile URLs.
 *
 * source_id and tile URL views are borrowed for the call. The function copies
 * accepted strings into MapLibre Native before return. options may be null for
 * defaults.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile URLs are null, empty, or invalid, options is
 *   invalid, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_raster_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) MLN_NOEXCEPT;

/**
 * Adds a raster DEM source with a TileJSON URL.
 *
 * source_id and url are borrowed for the call. options may be null for
 * defaults. For URL sources, tile_size and raster_encoding are used when their
 * field bits are set.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, options is invalid, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_raster_dem_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) MLN_NOEXCEPT;

/**
 * Adds a raster DEM source with inline tile URLs.
 *
 * source_id and tile URL views are borrowed for the call. The function copies
 * accepted strings into MapLibre Native before return. options may be null for
 * defaults.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile URLs are null, empty, or invalid, options is
 * invalid, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_raster_dem_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
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
 * inline JSON loads, style replacement completes before
 * mln_map_set_style_json() returns successfully.
 *
 * fetch_tile and cancel_tile may run on arbitrary native worker threads, may be
 * concurrent with owner-thread map calls, and must not call thread-affine map
 * APIs directly. Queue work back to the map owner thread before calling
 * mln_map_set_custom_geometry_source_tile_data() or invalidation functions.
 * Callbacks must not throw, panic, longjmp, or otherwise unwind through the C
 * ABI. cancel_tile is best-effort and may be repeated or race with fetch_tile.
 *
 * Custom geometry sources belong to the current style. Replacing the style
 * drops sources that were added to the previous style.
 *
 * A host that owns callback state frees it in options.release_user_data, which
 * this API invokes once after it stops referencing user_data. See
 * mln_custom_geometry_source_options.release_user_data.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, options is null or invalid, fetch_tile is null, or the
 *   source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options
) MLN_NOEXCEPT;

/**
 * Sets custom geometry source data for one canonical tile.
 *
 * source_id and UTF-8 GeoJSON data are borrowed for the call and parsed before
 * return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile_id is invalid, data is empty or invalid, the source
 *   does not exist, or the source is not a custom geometry source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data
) MLN_NOEXCEPT;

/**
 * Invalidates custom geometry source data for one canonical tile.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile_id is invalid, the source does not exist, or the
 *   source is not a custom geometry source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id
) MLN_NOEXCEPT;

/**
 * Invalidates custom geometry source data inside one geographic region.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, bounds is invalid, the source does not exist, or the
 *   source is not a custom geometry source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds
) MLN_NOEXCEPT;

/**
 * Sets one runtime style image.
 *
 * image_id, image, and image pixels are borrowed for the call. The function
 * copies accepted pixel bytes into the current style before return. If image_id
 * already exists, the native image is replaced.
 *
 * Runtime style images belong to the current style. Loading another style URL
 * or JSON document drops images that were added to the previous style.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, image or options is invalid, image pixels are null, image
 *   dimensions or stride are invalid, or image byte_length is too small.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) MLN_NOEXCEPT;

/**
 * Removes one runtime style image by ID.
 *
 * image_id is borrowed for the call. On success, out_removed reports whether an
 * image existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, or out_removed is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_remove_style_image(
  mln_map map, mln_buffer_view image_id, bool* out_removed
) MLN_NOEXCEPT;

/**
 * Reports whether a runtime style image ID exists.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, or out_exists is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_style_image_exists(
  mln_map map, mln_buffer_view image_id, bool* out_exists
) MLN_NOEXCEPT;

/**
 * Copies fixed metadata for one runtime style image.
 *
 * On success, out_found reports whether image_id exists. When not found,
 * out_info receives default image metadata.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, out_info is null, out_info->size is too small, or
 *   out_found is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_image_info(
  mln_map map, mln_buffer_view image_id, mln_style_image_info* out_info,
  bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies one runtime style image as tightly packed premultiplied RGBA8 pixels.
 *
 * image_id is borrowed for the call. On success, out_byte_length receives the
 * required byte length. When out_found is false, out_byte_length receives 0.
 *
 * Passing null for out_pixels with a capacity of 0 is a size probe: it reports
 * the required byte length and succeeds, so a caller can size a buffer without
 * treating the result as a failure. With a non-null out_pixels, a capacity
 * smaller than the required length still reports that length and returns
 * MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, out_pixels is null with non-zero capacity, pixel_capacity
 *   is too small for a non-null buffer, out_byte_length is null, or out_found
 *   is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_image_premultiplied_rgba8(
  mln_map map, mln_buffer_view image_id, uint8_t* out_pixels,
  size_t pixel_capacity, size_t* out_byte_length, bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies one runtime style image's stretchable intervals.
 *
 * image_id is borrowed for the call. Each output array may be null only when
 * its capacity is 0. On success, out_stretch_x_count and out_stretch_y_count
 * receive the interval counts, and both receive 0 when out_found is false.
 *
 * Passing null for both arrays with both capacities 0 is a size probe: it
 * reports the required counts and succeeds. With a non-null array, a capacity
 * smaller than that axis's count still reports the counts and returns
 * MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, an output array is null with non-zero capacity, a
 * capacity is too small for a non-null array, or an output count or out_found
 * is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_image_stretches(
  mln_map map, mln_buffer_view image_id, mln_image_stretch* out_stretch_x,
  size_t stretch_x_capacity, size_t* out_stretch_x_count,
  mln_image_stretch* out_stretch_y, size_t stretch_y_capacity,
  size_t* out_stretch_y_count, bool* out_found
) MLN_NOEXCEPT;

/**
 * Adds an image source that loads its image from a URL.
 *
 * source_id, coordinates, and url are borrowed for the call. coordinates must
 * contain exactly four coordinates in top-left, top-right, bottom-right,
 * bottom-left order. The function copies accepted strings and coordinates into
 * the current style before return. Later URL load or decode failures are
 * reported through runtime events.
 *
 * Image sources belong to the current style. Loading another style URL or JSON
 * document drops sources that were added to the previous style.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, coordinates is null or invalid, coordinate_count is
 * not 4, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_image_source_url(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_buffer_view url
) MLN_NOEXCEPT;

/**
 * Adds an image source with inline image pixels.
 *
 * source_id, coordinates, image, and image pixels are borrowed for the call.
 * coordinates must contain exactly four coordinates in top-left, top-right,
 * bottom-right, bottom-left order. The function copies accepted coordinates and
 * pixels into the current style before return.
 *
 * Image sources belong to the current style. Loading another style URL or JSON
 * document drops sources that were added to the previous style.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, coordinates is null or invalid, coordinate_count is not
 * 4, image is invalid, image pixels are null, image dimensions or stride are
 *   invalid, image byte_length is too small, or the source ID already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_image_source_image(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image
) MLN_NOEXCEPT;

/**
 * Updates an image source to load its image from a URL.
 *
 * source_id and url are borrowed for the call. Later URL load or decode
 * failures are reported through runtime events.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, the source does not exist, or the source is not an
 *   image source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url
) MLN_NOEXCEPT;

/**
 * Updates an image source with inline image pixels.
 *
 * source_id, image, and image pixels are borrowed for the call. The function
 * copies accepted pixels into MapLibre Native before return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, image is invalid, image pixels are null, image dimensions
 *   or stride are invalid, image byte_length is too small, the source does not
 *   exist, or the source is not an image source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image
) MLN_NOEXCEPT;

/**
 * Updates image source coordinates.
 *
 * coordinates is borrowed for the call and must contain exactly four
 * coordinates in top-left, top-right, bottom-right, bottom-left order. The
 * function copies accepted coordinates into MapLibre Native before return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, coordinates is null or invalid, coordinate_count is not
 * 4, the source does not exist, or the source is not an image source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count
) MLN_NOEXCEPT;

/**
 * Copies image source coordinates.
 *
 * On success, out_found reports whether source_id exists. When found,
 * out_coordinate_count receives 4. If coordinate_capacity is less than 4,
 * out_coordinate_count still receives 4 and the function returns
 * MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, out_coordinates is null with non-zero capacity,
 *   coordinate_capacity is too small for a found source, out_coordinate_count
 * is null, out_found is null, or the source exists and is not an image source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, mln_lat_lng* out_coordinates,
  size_t coordinate_capacity, size_t* out_coordinate_count, bool* out_found
) MLN_NOEXCEPT;

/**
 * Adds a hillshade layer for a raster DEM source.
 *
 * layer_id, source_id, and before_layer_id are borrowed for the call. Passing
 * an empty before_layer_id appends the layer; otherwise the layer is inserted
 * before that existing layer.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   source_id is invalid or empty, before_layer_id is invalid or does not
 * exist, layer_id already exists, source_id does not exist, or source_id is not
 * a raster DEM source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_hillshade_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) MLN_NOEXCEPT;

/**
 * Adds a color-relief layer for a raster DEM source.
 *
 * layer_id, source_id, and before_layer_id are borrowed for the call. Passing
 * an empty before_layer_id appends the layer; otherwise the layer is inserted
 * before that existing layer. Use mln_map_set_layer_property() with
 * color-relief-color to set the color ramp expression.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   source_id is invalid or empty, before_layer_id is invalid or does not
 * exist, layer_id already exists, source_id does not exist, or source_id is not
 * a raster DEM source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_color_relief_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) MLN_NOEXCEPT;

/**
 * Adds a source-free location indicator layer.
 *
 * layer_id and before_layer_id are borrowed for the call. Passing an empty
 * before_layer_id appends the layer; otherwise the layer is inserted before
 * that existing layer.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, before_layer_id is invalid or does not exist, or layer_id
 *   already exists.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_location_indicator_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) MLN_NOEXCEPT;

/**
 * Sets a location indicator layer location.
 *
 * coordinate uses normal C API latitude/longitude order. The underlying style
 * property is written as [latitude, longitude, altitude], matching the order
 * the renderer reads it back in.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, coordinate or altitude is invalid, the layer does not
 *   exist, or the layer is not a location indicator layer.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_location_indicator_location(
  mln_map map, mln_buffer_view layer_id, mln_lat_lng coordinate, double altitude
) MLN_NOEXCEPT;

/**
 * Sets a location indicator layer bearing in degrees.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, bearing is not finite float32, the layer does not exist,
 *   or the layer is not a location indicator layer.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_location_indicator_bearing(
  mln_map map, mln_buffer_view layer_id, double bearing
) MLN_NOEXCEPT;

/**
 * Sets a location indicator layer accuracy radius in logical pixels.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, radius is negative or not finite float32, the layer does
 *   not exist, or the layer is not a location indicator layer.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_location_indicator_accuracy_radius(
  mln_map map, mln_buffer_view layer_id, double radius
) MLN_NOEXCEPT;

/**
 * Sets one location indicator image-name property.
 *
 * image_id is borrowed for the call and copied into native style storage. The
 * named style image does not need to exist when this function is called.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   image_id is invalid or empty, image_kind is invalid, the layer does not
 *   exist, or the layer is not a location indicator layer.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_location_indicator_image_name(
  mln_map map, mln_buffer_view layer_id, uint32_t image_kind,
  mln_buffer_view image_id
) MLN_NOEXCEPT;

/**
 * Adds one style layer from a full style-spec layer JSON object.
 *
 * This is the insertion path for every style-spec layer type. The typed adders
 * above exist for reasons beyond construction: mln_map_add_hillshade_layer()
 * and mln_map_add_color_relief_layer() validate that the source is a raster DEM
 * source, and mln_map_add_location_indicator_layer() pairs with typed per-frame
 * setters that take coordinates in C API order.
 *
 * layer_json and before_layer_id are borrowed for the call. layer_json must
 * contain id and type members. Passing an empty before_layer_id appends the
 * layer; otherwise the layer is inserted before that existing layer.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_json is
 *   empty or invalid, the layer ID already exists, before_layer_id is invalid
 * or does not exist, or the layer JSON cannot be converted.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json, mln_buffer_view before_layer_id
) MLN_NOEXCEPT;

/**
 * Removes one style layer by ID.
 *
 * layer_id is borrowed for the call. On success, out_removed reports whether a
 * layer existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or out_removed is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_remove_style_layer(
  mln_map map, mln_buffer_view layer_id, bool* out_removed
) MLN_NOEXCEPT;

/**
 * Reports whether a style layer ID exists.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or out_exists is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_style_layer_exists(
  mln_map map, mln_buffer_view layer_id, bool* out_exists
) MLN_NOEXCEPT;

/**
 * Borrows one style layer type string.
 *
 * On success, out_found reports whether layer_id exists. When found,
 * out_layer_type receives a view of a static style-spec layer type string.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_layer_type is null, or out_found is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_layer_type(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view* out_layer_type,
  bool* out_found
) MLN_NOEXCEPT;

/**
 * Copies style layer IDs in style order.
 *
 * On success, *out_layer_ids receives an owned list handle. Destroy it with
 * mln_style_id_list_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, out_layer_ids is
 *   null, or *out_layer_ids is not null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_list_style_layer_ids(
  mln_map map, mln_style_id_list* out_layer_ids
) MLN_NOEXCEPT;

/**
 * Moves one style layer before another layer or to the top.
 *
 * layer_id and before_layer_id are borrowed for the call. Passing an empty
 * before_layer_id moves the layer to the top of the style order.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, before_layer_id is invalid, layer_id does not exist, or
 *   before_layer_id is non-empty and does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) MLN_NOEXCEPT;

/**
 * Serializes one style layer as a full style-spec layer JSON object.
 *
 * On success, out_found reports whether layer_id exists. When found,
 * *out_layer receives an owned UTF-8 JSON buffer. Destroy it with
 * mln_buffer_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_layer is null, *out_layer is not null, or out_found
 *   is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_layer_json(
  mln_map map, mln_buffer_view layer_id, mln_buffer* out_layer, bool* out_found
) MLN_NOEXCEPT;

/**
 * Sets the style light from a style-spec light JSON object.
 *
 * light_json is borrowed for the call. The function parses and copies the
 * accepted light into MapLibre Native before return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, light_json is
 *   empty or invalid, or the light JSON cannot be converted.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_style_light_json(
  mln_map map, mln_buffer_view light_json
) MLN_NOEXCEPT;

/**
 * Sets one style light property using its MapLibre style-spec property name.
 *
 * property_name and value are borrowed for the call. value is a style-spec JSON
 * value. The function parses and copies the accepted value into
 * MapLibre Native's typed light property storage before return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, property_name is
 *   invalid or empty, value is empty or invalid, the property name is unknown,
 *   or the property value cannot be converted for that property.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer_view value
) MLN_NOEXCEPT;

/**
 * Serializes one style light property as a style-spec JSON value.
 *
 * On success, *out_value receives an owned UTF-8 JSON buffer. Destroy it
 * with mln_buffer_destroy(). Undefined native style light properties
 * return null buffers.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, property_name is
 *   invalid or empty, out_value is null, or *out_value is not null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer* out_value
) MLN_NOEXCEPT;

/**
 * Sets the style's global transition options.
 *
 * options is borrowed for the call and copied into MapLibre Native before
 * return. Omitted duration and delay fields clear the style-wide override, so
 * this call replaces the whole transition configuration rather than merging
 * into it.
 *
 * Loading a style replaces these options with the ones that style declares, so
 * a host that overrides them applies the override after the style loads.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, options is null
 *   or undersized, options->fields contains unknown bits, or an enabled
 *   duration or delay is negative, non-finite, or out of the native duration
 *   range.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options
) MLN_NOEXCEPT;

/**
 * Reads the style's global transition options.
 *
 * On success, *out_options receives the last-known transition configuration.
 * Duration and delay report through their field-mask bits, because MapLibre
 * Native leaves either one unset until a style or a host sets it.
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
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, or out_options is
 *   null or undersized.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_transition_options(
  mln_map map, mln_style_transition_options* out_options
) MLN_NOEXCEPT;

/**
 * Sets one layer property using its MapLibre style-spec property name.
 *
 * layer_id, property_name, and value are borrowed for the call. value is a
 * style-spec JSON value. Expressions use style-spec expression JSON
 * arrays. The function parses and copies the accepted value into MapLibre
 * Native's typed style property storage before return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   property_name is invalid or empty, value is empty or invalid, the layer
 * does not exist, the property name is unknown for that layer, or the property
 *   value cannot be converted for that property.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value
) MLN_NOEXCEPT;

/**
 * Serializes one layer property as a style-spec JSON value.
 *
 * On success, *out_value receives an owned UTF-8 JSON buffer. Destroy it
 * with mln_buffer_destroy(). Undefined native style properties return
 * null buffers.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   property_name is invalid or empty, out_value is null, *out_value is not
 *   null, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer* out_value
) MLN_NOEXCEPT;

/**
 * Sets or clears one layer filter.
 *
 * layer_id and filter are borrowed for the call. Passing null for filter clears
 * the layer filter. Non-null filters use the MapLibre style-spec filter JSON
 * representation. The function parses and copies the accepted filter into
 * MapLibre Native's typed filter expression storage before return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, filter is invalid, the layer does not exist, or the
 *   filter cannot be converted.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_buffer_view* filter
) MLN_NOEXCEPT;

/**
 * Serializes one layer filter as a style-spec JSON value.
 *
 * On success, *out_filter receives an owned UTF-8 JSON buffer. Destroy it
 * with mln_buffer_destroy(). Missing filters return null buffers.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_filter is null, *out_filter is not null, or the layer
 *   does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_layer_filter(
  mln_map map, mln_buffer_view layer_id, mln_buffer* out_filter
) MLN_NOEXCEPT;

/**
 * Sets one layer's source-layer ID.
 *
 * layer_id and source_layer are borrowed for the call and copied into MapLibre
 * Native's layer storage before return. Passing an empty source_layer clears
 * it.
 *
 * Only layer types that require a source carry a source-layer; this rejects the
 * others, such as background and custom.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, source_layer is invalid, the layer does not exist, or the
 *   layer's type does not take a source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_layer
) MLN_NOEXCEPT;

/**
 * Copies one layer's source-layer ID into caller-owned memory.
 *
 * layer_id is borrowed for the call. On success, out_source_layer_size receives
 * the byte length excluding any null terminator, and 0 when the layer carries
 * no source-layer.
 *
 * Passing null for out_source_layer with a capacity of 0 is a size probe: it
 * reports the required byte length and succeeds, so a caller can size a buffer
 * without treating the result as a failure. With a non-null out_source_layer, a
 * capacity smaller than the required length still reports that length and
 * returns MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_source_layer is null with non-zero capacity,
 *   source_layer_capacity is too small for a non-null buffer,
 *   out_source_layer_size is null, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, char* out_source_layer,
  size_t source_layer_capacity, size_t* out_source_layer_size
) MLN_NOEXCEPT;

/**
 * Sets one layer's source ID.
 *
 * layer_id and source_id are borrowed for the call and copied into MapLibre
 * Native's layer storage before return. This does not require the named source
 * to exist yet; MapLibre reports an unresolved source through style events.
 *
 * Only layer types that require a source carry a source ID; this rejects the
 * others, such as background and custom.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, source_id is invalid or empty, the layer does not exist,
 *   or the layer's type does not take a source.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_layer_source_id(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id
) MLN_NOEXCEPT;

/**
 * Copies one layer's source ID into caller-owned memory.
 *
 * layer_id is borrowed for the call. On success, out_source_id_size receives
 * the byte length excluding any null terminator, and 0 when the layer carries
 * no source.
 *
 * Passing null for out_source_id with a capacity of 0 is a size probe: it
 * reports the required byte length and succeeds, so a caller can size a buffer
 * without treating the result as a failure. With a non-null out_source_id, a
 * capacity smaller than the required length still reports that length and
 * returns MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_source_id is null with non-zero capacity,
 *   source_id_capacity is too small for a non-null buffer, out_source_id_size
 * is null, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_layer_source_id(
  mln_map map, mln_buffer_view layer_id, char* out_source_id,
  size_t source_id_capacity, size_t* out_source_id_size
) MLN_NOEXCEPT;

/**
 * Sets the lowest zoom at which one layer draws.
 *
 * Pass -INFINITY for no lower bound. MapLibre Native stores the zoom range as
 * single-precision floats, so this narrows the value.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, min_zoom is NaN, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_layer_min_zoom(
  mln_map map, mln_buffer_view layer_id, double min_zoom
) MLN_NOEXCEPT;

/**
 * Reads the lowest zoom at which one layer draws.
 *
 * A layer with no lower bound reports -INFINITY.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_min_zoom is null, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_layer_min_zoom(
  mln_map map, mln_buffer_view layer_id, double* out_min_zoom
) MLN_NOEXCEPT;

/**
 * Sets the highest zoom at which one layer draws.
 *
 * Pass INFINITY for no upper bound. MapLibre Native stores the zoom range as
 * single-precision floats, so this narrows the value.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, max_zoom is NaN, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_layer_max_zoom(
  mln_map map, mln_buffer_view layer_id, double max_zoom
) MLN_NOEXCEPT;

/**
 * Reads the highest zoom at which one layer draws.
 *
 * A layer with no upper bound reports INFINITY.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_max_zoom is null, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_layer_max_zoom(
  mln_map map, mln_buffer_view layer_id, double* out_max_zoom
) MLN_NOEXCEPT;

/**
 * Sets whether one layer draws.
 *
 * visibility is an mln_style_layer_visibility value.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, visibility is not an mln_style_layer_visibility value, or
 *   the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_layer_visibility(
  mln_map map, mln_buffer_view layer_id, uint32_t visibility
) MLN_NOEXCEPT;

/**
 * Reads whether one layer draws.
 *
 * On success, *out_visibility receives an mln_style_layer_visibility value.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, out_visibility is null, or the layer does not exist.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_layer_visibility(
  mln_map map, mln_buffer_view layer_id, uint32_t* out_visibility
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_STYLE_H
