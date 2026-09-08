/**
 * @file maplibre_native_c/style.h
 * Public C API declarations for style sources, layers, and images.
 *
 * Every mutation is a command. It validates and copies all input bytes, arrays,
 * option structs, and image pixels before returning. Parsing and application
 * run later on the map worker, in runtime order. Its completion reports
 * committed, superseded, failed, or cancelled disposition. Committed
 * completions carry the map snapshot generation they published, so a caller can
 * fence a later mln_map_snapshot_get() on it.
 *
 * Every query is ordered. It copies its inputs before returning and observes
 * all commands accepted earlier by the runtime. Its completion borrows the
 * typed result for the duration of the callback.
 *
 * All declarations in this header are callable from any thread. A per-function
 * Returns list gives the statuses this call returns; a Completes with list
 * gives the statuses that reach the caller through the completion.
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

typedef uint64_t mln_geojson_source_data;

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
typedef enum mln_style_source_info_field : uint32_t {
  /** The source retains a URL. */
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

/** Borrowed image-stretch arrays available during a completion callback. */
typedef struct mln_style_image_stretches_result {
  uint32_t size;
  uint32_t reserved;
  const mln_image_stretch* stretch_x;
  size_t stretch_x_count;
  const mln_image_stretch* stretch_y;
  size_t stretch_y_count;
} mln_style_image_stretches_result;

/** Borrowed inline TileJSON tile URLs available during a completion callback.
 */
typedef struct mln_style_source_tile_urls_result {
  uint32_t size;
  uint32_t reserved;
  const mln_buffer_view* tile_urls;
  size_t tile_url_count;
} mln_style_source_tile_urls_result;

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

/** Field mask values for mln_custom_mvt_vector_source_options. */
typedef enum mln_custom_mvt_vector_source_option_field : uint32_t {
  MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM = 1U << 0U,
  MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM = 1U << 1U,
} mln_custom_mvt_vector_source_option_field;

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

/** Fixed source metadata included in mln_style_source_result. */
typedef struct mln_style_source_info {
  uint32_t size;
  /** One of mln_style_source_type. */
  uint32_t type;
  /** Bitwise combination of mln_style_source_info_field values. */
  uint32_t fields;
  /** Source ID byte length, excluding any null terminator. */
  size_t id_size;
  /** Whether the source is marked volatile. */
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

/** Complete source metadata borrowed for a completion callback. */
typedef struct mln_style_source_result {
  uint32_t size;
  uint32_t reserved;
  mln_style_source_info info;
  mln_buffer_view attribution;
  mln_buffer_view url;
  const mln_buffer_view* tile_urls;
  size_t tile_url_count;
} mln_style_source_result;

/** Fixed layer metadata included in mln_style_layer_result. */
typedef struct mln_style_layer_info {
  uint32_t size;
  uint32_t reserved;
  /** View of a static style-spec layer type string. It stays valid for the
     life of the process. */
  mln_buffer_view type;
  /** Lowest zoom at which the layer draws; -INFINITY with no lower bound. */
  double min_zoom;
  /** Highest zoom at which the layer draws; INFINITY with no upper bound. */
  double max_zoom;
  /** One of mln_style_layer_visibility. */
  uint32_t visibility;
} mln_style_layer_info;

/** Complete layer metadata borrowed for a completion callback. */
typedef struct mln_style_layer_result {
  uint32_t size;
  uint32_t reserved;
  mln_style_layer_info info;
  /** Source ID. Empty for a layer type that takes no source. */
  mln_buffer_view source_id;
  /** Source-layer ID. Empty when the layer sets none. */
  mln_buffer_view source_layer;
} mln_style_layer_result;

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

/** Canonical tile identity used by custom geometry and custom MVT vector source
 * callbacks. */
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
   * This callback must not destroy its map, because a release that style
   * processing drives may run inside MapLibre's dispatch. Free callback state
   * and return. A host that wants to destroy the map does so from its own call
   * after the release returns.
   */
  mln_custom_geometry_source_release_callback release_user_data;
} mln_custom_geometry_source_options;

/** Callback invoked for custom MVT vector source tile requests and cancels. */
typedef void (*mln_custom_mvt_vector_source_tile_callback)(
  void* user_data, mln_canonical_tile_id tile_id
);

/** Releases a custom MVT vector source's callback context. */
typedef void (*mln_custom_mvt_vector_source_release_callback)(void* user_data);

/** Options for custom MVT vector sources. */
typedef struct mln_custom_mvt_vector_source_options {
  uint32_t size;
  uint32_t fields;
  /** Required tile fetch callback. */
  mln_custom_mvt_vector_source_tile_callback fetch_tile;
  /** Optional best-effort tile cancel callback. */
  mln_custom_mvt_vector_source_tile_callback cancel_tile;
  /** Caller-owned callback context retained by pointer. */
  void* user_data;
  double min_zoom;
  double max_zoom;
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
   * This callback must not destroy its map, because a release that style
   * processing drives may run inside MapLibre's dispatch. Free callback state
   * and return. A host that wants to destroy the map does so from its own call
   * after the release returns.
   */
  mln_custom_mvt_vector_source_release_callback release_user_data;
} mln_custom_mvt_vector_source_options;

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
   * Interval counts for the stretchable axes.
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

/** Complete style image borrowed for a completion callback. */
typedef struct mln_style_image_result {
  uint32_t size;
  uint32_t reserved;
  mln_style_image_info info;
  mln_buffer_view pixels;
  const mln_image_stretch* stretch_x;
  size_t stretch_x_count;
  const mln_image_stretch* stretch_y;
  size_t stretch_y_count;
} mln_style_image_result;

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

/** Returns default custom MVT vector source options. */
MLN_API mln_custom_mvt_vector_source_options
mln_custom_mvt_vector_source_options_default(void) MLN_NOEXCEPT;

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
 * Adds one style source from a style-spec source JSON object.
 *
 * source_id and source_json are borrowed for the call and copied before
 * return. source_json is the object that appears under sources[source_id] in a
 * style document. Parsing and application run on the map worker.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, source_json is empty or invalid, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID, or the
 *   source JSON cannot be converted.
 */
MLN_API mln_status mln_map_add_style_source_json(
  mln_map map, mln_buffer_view source_id, mln_buffer_view source_json,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Removes one style source by ID.
 *
 * source_id is borrowed for the call. The command commits when a source with
 * that ID existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_STATE when the source exists but a layer still uses it.
 */
MLN_API mln_status mln_map_remove_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies complete metadata for one style source.
 *
 * A found source completes with one borrowed mln_style_source_result. A missing
 * source completes successfully with no value. The binding must copy strings
 * and tile URL views before the callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_source_info(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets whether one style source stores fetched tiles in the persistent cache.
 *
 * source_id is copied before acceptance. When is_volatile is true, source
 * implementations that fetch tiles stop storing them in persistent storage.
 * Other source types retain the value for inspection without changing how they
 * load. The change applies when the command commits and is visible through
 * mln_map_get_style_source_info as info.is_volatile.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 */
MLN_API mln_status mln_map_set_style_source_volatile(
  mln_map map, mln_buffer_view source_id, bool is_volatile,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one style source attribution string.
 *
 * A found attribution completes with one borrowed mln_buffer_view. A missing
 * source or attribution completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_source_attribution(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one style source URL.
 *
 * A found URL completes with one borrowed mln_buffer_view. A missing source or
 * URL completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_source_url(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one style source's inline TileJSON tile URLs.
 *
 * A found source completes with one borrowed mln_style_source_tile_urls_result
 * whose tile_urls array is empty for a URL-backed source or a source without
 * inline TileJSON. A missing source completes successfully with no value. The
 * binding must copy the views before the callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_source_tile_urls(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies style source IDs in style order.
 *
 * The completion borrows an array of mln_buffer_view values.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_list_style_source_ids(
  mln_map map, const mln_completion* completion
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id or url is empty, a source
 *   already has that ID, or the options cannot be converted.
 */
MLN_API mln_status mln_map_add_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geojson_source_options* options, const mln_completion* completion
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
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid, or data is null or not live.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id is empty or a source already
 *   has that ID.
 */
MLN_API mln_status mln_map_add_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Updates one GeoJSON source to load data from a URL.
 *
 * source_id and url are borrowed for the call. The source keeps the
 * mln_geojson_source_options it was added with.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a GeoJSON source.
 */
MLN_API mln_status mln_map_set_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion
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
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, data is null or not live, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a GeoJSON source, or
 *   the data was prepared with options that do not match the source's options.
 */
MLN_API mln_status mln_map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Overrides one GeoJSON source's synchronous tiling at runtime.
 *
 * source_id is borrowed for the call. While enabled is true, the source slices
 * requested tiles inline during the update pass, as if the source's options
 * had set synchronous_tiling; false restores the option the source was added
 * with. The override applies to update passes after the command commits. Hosts
 * enable it around high-frequency small updates, such as a tracked position,
 * so each installed update reaches the next rendered frame.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a GeoJSON source.
 */
MLN_API mln_status mln_map_set_geojson_source_synchronous_tiling(
  mln_map map, mln_buffer_view source_id, bool enabled,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a vector source with a TileJSON URL.
 *
 * source_id and url are borrowed for the call. options may be null for
 * defaults. For URL sources, min_zoom, max_zoom, and vector_encoding override
 * values from the loaded TileJSON when their field bits are set.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id or url is empty, or a source
 *   already has that ID.
 */
MLN_API mln_status mln_map_add_vector_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a vector source with inline tile URLs.
 *
 * source_id and tile URL views are borrowed for the call and copied before
 * return. options may be null for defaults.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid, tile URLs are null or invalid, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id is empty, the tile URL list or
 *   any tile URL is empty, a source already has that ID, or the tileset cannot
 *   be built.
 */
MLN_API mln_status mln_map_add_vector_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a raster source with a TileJSON URL.
 *
 * source_id and url are borrowed for the call. options may be null for
 * defaults. For URL sources, only tile_size is used when its field bit is set.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id or url is empty, or a source
 *   already has that ID.
 */
MLN_API mln_status mln_map_add_raster_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a raster source with inline tile URLs.
 *
 * source_id and tile URL views are borrowed for the call and copied before
 * return. options may be null for defaults.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid, tile URLs are null or invalid, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id is empty, the tile URL list or
 *   any tile URL is empty, a source already has that ID, or the tileset cannot
 *   be built.
 */
MLN_API mln_status mln_map_add_raster_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a raster DEM source with a TileJSON URL.
 *
 * source_id and url are borrowed for the call. options may be null for
 * defaults. For URL sources, tile_size and raster_encoding are used when their
 * field bits are set.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id or url is empty, or a source
 *   already has that ID.
 */
MLN_API mln_status mln_map_add_raster_dem_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a raster DEM source with inline tile URLs.
 *
 * source_id and tile URL views are borrowed for the call and copied before
 * return. options may be null for defaults.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid, tile URLs are null or invalid, or options is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id is empty, the tile URL list or
 *   any tile URL is empty, a source already has that ID, or the tileset cannot
 *   be built.
 */
MLN_API mln_status mln_map_add_raster_dem_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options,
  const mln_completion* completion
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
 * concurrent with runtime and map calls. The style command functions in this
 * header may be called directly from these callbacks. Callbacks must not block
 * waiting for command execution.
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
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid, options is null or invalid, or fetch_tile is null.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id is empty or a source already
 *   has that ID. This API runs release_user_data once, because the accepted
 *   command already referenced user_data.
 */
MLN_API mln_status mln_map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets custom geometry source data for one canonical tile.
 *
 * source_id and the UTF-8 GeoJSON data are borrowed for the call and copied
 * before return. Parsing and application run on the map worker.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile_id is invalid, data is empty, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the data is not valid GeoJSON, or the
 *   source is not a custom geometry source.
 */
MLN_API mln_status mln_map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Invalidates custom geometry source data for one canonical tile.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom geometry
 *   source.
 */
MLN_API mln_status mln_map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Invalidates custom geometry source data inside one geographic region.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, bounds is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom geometry
 *   source.
 */
MLN_API mln_status mln_map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds,
  const mln_completion* completion
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
 * inline JSON loads, style replacement completes before
 * mln_map_set_style_json() returns successfully.
 *
 * fetch_tile and cancel_tile may run on arbitrary native worker threads, may be
 * concurrent with runtime and map calls. The style command functions in this
 * header, including the tile-delivery commands below, may be called directly
 * from these callbacks. Callbacks must not block waiting for command
 * execution, and must not throw, panic, longjmp, or otherwise unwind through
 * the C ABI. cancel_tile is best-effort and may be repeated or race with
 * fetch_tile.
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid, options is null or invalid, or fetch_tile is null.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when source_id is empty or a source already
 *   has that ID. This API runs release_user_data once, because the accepted
 *   command already referenced user_data.
 */
MLN_API mln_status mln_map_add_custom_mvt_vector_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_mvt_vector_source_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets custom MVT vector source data for one canonical tile.
 *
 * source_id and the MVT protobuf bytes are borrowed for the call. The function
 * copies accepted bytes before return. A zero-length view, including a null
 * pointer with size 0, is an empty tile.
 *
 * MapLibre ignores the bytes when that tile is not awaiting a response after
 * fetch_tile, including after cancel_tile, and this call still returns
 * MLN_STATUS_OK after validation.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile_id is invalid, data is a null pointer with a
 *   nonzero size, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom MVT vector
 *   source.
 */
MLN_API mln_status mln_map_set_custom_mvt_vector_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Reports a custom MVT vector source error for one canonical tile.
 *
 * source_id and the UTF-8 diagnostic message are borrowed for the call. The
 * function copies accepted bytes before return. An empty message is accepted.
 *
 * MapLibre ignores the error when that tile is not awaiting a response after
 * fetch_tile, including after cancel_tile, and this call still returns
 * MLN_STATUS_OK after validation.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile_id is invalid, message is a null pointer with a
 *   nonzero size, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom MVT vector
 *   source.
 */
MLN_API mln_status mln_map_set_custom_mvt_vector_source_tile_error(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Invalidates custom MVT vector source data for one canonical tile.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, tile_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not a custom MVT vector
 *   source.
 */
MLN_API mln_status mln_map_invalidate_custom_mvt_vector_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets one runtime style image.
 *
 * image_id, image, and image pixels are borrowed for the call and copied
 * before return. If image_id already exists, the native image is replaced when
 * the command commits.
 *
 * Runtime style images belong to the current style. Loading another style URL
 * or JSON document drops images that were added to the previous style.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, image or options is invalid, image pixels are null, image
 *   dimensions or stride are invalid, or image byte_length is too small.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Removes one runtime style image by ID.
 *
 * image_id is borrowed for the call. The command commits when an image with
 * that ID existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no runtime style image has that ID.
 */
MLN_API mln_status mln_map_remove_style_image(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one complete runtime style image.
 *
 * A found image completes with one borrowed mln_style_image_result containing
 * metadata, pixels, and stretch intervals. A missing image completes
 * successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_image_info(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one runtime style image as tightly packed premultiplied RGBA8 pixels.
 *
 * A found image completes with one borrowed mln_buffer_view. A missing image
 * completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_image_premultiplied_rgba8(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one runtime style image's stretchable intervals.
 *
 * A found image completes with one borrowed mln_style_image_stretches_result.
 * A missing image completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, image_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_image_stretches(
  mln_map map, mln_buffer_view image_id, const mln_completion* completion
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, coordinates is null or invalid, or coordinate_count
 *   is not 4.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID.
 */
MLN_API mln_status mln_map_add_image_source_url(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_buffer_view url, const mln_completion* completion
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, coordinates is null or invalid, coordinate_count is not
 *   4, image is invalid, image pixels are null, image dimensions or stride are
 *   invalid, or image byte_length is too small.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when a source already has that ID.
 */
MLN_API mln_status mln_map_add_image_source_image(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Updates an image source to load its image from a URL.
 *
 * source_id and url are borrowed for the call. Later URL load or decode
 * failures are reported through runtime events.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id or url
 *   is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not an image source.
 */
MLN_API mln_status mln_map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Updates an image source with inline image pixels.
 *
 * source_id, image, and image pixels are borrowed for the call and copied
 * before return.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, image is invalid, image pixels are null, image dimensions
 *   or stride are invalid, image byte_length is too small, the source does not
 *   exist, or the source is not an image source.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image, const mln_completion* completion
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, coordinates is null or invalid, coordinate_count is not
 *   4, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the source is not an image source.
 */
MLN_API mln_status mln_map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies image source coordinates.
 *
 * A found image source completes with four borrowed mln_lat_lng values. A
 * missing source completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, source_id is
 *   invalid or empty, completion is invalid, or the source exists and is not
 *   an image source.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a hillshade layer for a raster DEM source.
 *
 * layer_id, source_id, and before_layer_id are borrowed for the call. Passing
 * an empty before_layer_id appends the layer; otherwise the layer is inserted
 * before that existing layer.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   source_id is invalid or empty, before_layer_id is invalid, or completion
 *   is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has source_id, or before_layer_id
 *   is non-empty and no style layer has it.
 * - MLN_STATUS_INVALID_ARGUMENT when a layer already has layer_id, or source_id
 *   is not a raster DEM source.
 */
MLN_API mln_status mln_map_add_hillshade_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id, const mln_completion* completion
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
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   source_id is invalid or empty, before_layer_id is invalid, or completion
 *   is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style source has source_id, or before_layer_id
 *   is non-empty and no style layer has it.
 * - MLN_STATUS_INVALID_ARGUMENT when a layer already has layer_id, or source_id
 *   is not a raster DEM source.
 */
MLN_API mln_status mln_map_add_color_relief_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Adds a source-free location indicator layer.
 *
 * layer_id and before_layer_id are borrowed for the call. Passing an empty
 * before_layer_id appends the layer; otherwise the layer is inserted before
 * that existing layer.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, before_layer_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when before_layer_id is non-empty and no style layer
 *   has it.
 * - MLN_STATUS_INVALID_ARGUMENT when a layer already has layer_id.
 */
MLN_API mln_status mln_map_add_location_indicator_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets a location indicator layer location.
 *
 * coordinate uses normal C API latitude/longitude order. The underlying style
 * property is written as [latitude, longitude, altitude], matching the order
 * the renderer reads it back in.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, coordinate or altitude is invalid, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer is not a location indicator
 *   layer.
 */
MLN_API mln_status mln_map_set_location_indicator_location(
  mln_map map, mln_buffer_view layer_id, mln_lat_lng coordinate,
  double altitude, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets a location indicator layer bearing in degrees.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, bearing is not finite float32, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer is not a location indicator
 *   layer.
 */
MLN_API mln_status mln_map_set_location_indicator_bearing(
  mln_map map, mln_buffer_view layer_id, double bearing,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets a location indicator layer accuracy radius in meters.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, radius is negative or not finite float32, or completion
 *   is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer is not a location indicator
 *   layer.
 */
MLN_API mln_status mln_map_set_location_indicator_accuracy_radius(
  mln_map map, mln_buffer_view layer_id, double radius,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets one location indicator image-name property.
 *
 * image_id is borrowed for the call and copied into native style storage. The
 * named style image does not need to exist when this function is called.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   image_id is invalid or empty, image_kind is invalid, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer is not a location indicator
 *   layer.
 */
MLN_API mln_status mln_map_set_location_indicator_image_name(
  mln_map map, mln_buffer_view layer_id, uint32_t image_kind,
  mln_buffer_view image_id, const mln_completion* completion
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
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_json is
 *   empty or invalid, before_layer_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when before_layer_id is non-empty and no style layer
 *   has it, or the layer requires a source no style source has.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer JSON cannot be converted, or a
 *   layer already has the JSON's id.
 */
MLN_API mln_status mln_map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json, mln_buffer_view before_layer_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Removes one style layer by ID.
 *
 * layer_id is borrowed for the call. The command commits when a layer with
 * that ID existed and was removed.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_remove_style_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies complete metadata for one style layer.
 *
 * A found layer completes with one borrowed mln_style_layer_result. A missing
 * layer completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_layer_info(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies style layer IDs in style order.
 *
 * The completion borrows an array of mln_buffer_view values.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_list_style_layer_ids(
  mln_map map, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Moves one style layer before another layer or to the top.
 *
 * layer_id and before_layer_id are borrowed for the call. Passing an empty
 * before_layer_id moves the layer to the top of the style order.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, before_layer_id is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has layer_id, or before_layer_id
 *   is non-empty and no style layer has it.
 */
MLN_API mln_status mln_map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Serializes one style layer as a full style-spec layer JSON object.
 *
 * A found layer completes with one borrowed mln_buffer_view. A missing layer
 * completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_layer_json(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets the style light from a style-spec light JSON object.
 *
 * light_json is borrowed for the call and copied before return. Parsing and
 * application run on the map worker.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, light_json is
 *   empty or invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the light JSON cannot be converted.
 */
MLN_API mln_status mln_map_set_style_light_json(
  mln_map map, mln_buffer_view light_json, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets one style light property using its MapLibre style-spec property name.
 *
 * property_name and value are borrowed for the call and copied before return.
 * value is a style-spec JSON value. Parsing and application into MapLibre
 * Native's typed light property storage run on the map worker.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, property_name is
 *   invalid or empty, value is empty or invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when the style has no light.
 * - MLN_STATUS_INVALID_ARGUMENT when the property name is unknown, or the
 *   value cannot be converted for that property.
 */
MLN_API mln_status mln_map_set_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer_view value,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Serializes one style light property as a style-spec JSON value.
 *
 * A defined property completes with one borrowed mln_buffer_view. An undefined
 * property completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, property_name is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_light_property(
  mln_map map, mln_buffer_view property_name, const mln_completion* completion
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
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, options is null
 *   or undersized, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when options->fields contains unknown bits, or
 *   an enabled duration or delay is negative, non-finite, or out of the native
 *   duration range. A rejected command changes nothing.
 * - MLN_STATUS_NATIVE_ERROR when applying the options throws on the map worker.
 */
MLN_API mln_status mln_map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Reads the style's global transition options.
 *
 * The completion borrows the last-known transition configuration. Duration
 * and delay report through their field-mask bits, because MapLibre
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
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_style_transition_options(
  mln_map map, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets one layer property using its MapLibre style-spec property name.
 *
 * layer_id, property_name, and value are borrowed for the call and copied
 * before return. value is a style-spec JSON value; expressions use style-spec
 * expression JSON arrays. Parsing and application into MapLibre Native's typed
 * style property storage run on the map worker.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   property_name is invalid or empty, value is empty or invalid, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the property name is unknown for that
 *   layer, or the value cannot be converted for that property.
 */
MLN_API mln_status mln_map_set_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Serializes one layer property as a style-spec JSON value.
 *
 * A defined property completes with one borrowed mln_buffer_view. An undefined
 * property completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id or
 *   property_name is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_get_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets or clears one layer filter.
 *
 * layer_id and filter are borrowed for the call and copied before return.
 * Passing null for filter clears the layer filter. Non-null filters use the
 * MapLibre style-spec filter JSON representation. Parsing and application into
 * MapLibre Native's typed filter expression storage run on the map worker.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, filter is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the filter cannot be converted.
 */
MLN_API mln_status mln_map_set_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_buffer_view* filter,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Serializes one layer filter as a style-spec JSON value.
 *
 * A present filter completes with one borrowed mln_buffer_view. A missing
 * filter completes successfully with no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_get_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets one layer's source-layer ID.
 *
 * layer_id and source_layer are borrowed for the call and copied before
 * return. Passing an empty source_layer clears it.
 *
 * Only layer types that require a source carry a source-layer; this rejects the
 * others, such as background and custom.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, source_layer is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer's type does not take a source.
 */
MLN_API mln_status mln_map_set_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_layer,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one layer's source-layer ID.
 *
 * The completion borrows one mln_buffer_view, which is empty when the layer
 * carries no source-layer ID.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_copy_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets one layer's source ID.
 *
 * layer_id and source_id are borrowed for the call and copied before return.
 * This does not require the named source to exist yet; MapLibre reports an
 * unresolved source through style events.
 *
 * Only layer types that require a source carry a source ID; this rejects the
 * others, such as background and custom.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, source_id is invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 * - MLN_STATUS_INVALID_ARGUMENT when the layer's type does not take a source.
 */
MLN_API mln_status mln_map_set_layer_source_id(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies one layer's source ID.
 *
 * The completion borrows one mln_buffer_view, which is empty when the layer
 * carries no source ID.
 *
 * Returns:
 * - MLN_STATUS_OK when the query was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_copy_layer_source_id(
  mln_map map, mln_buffer_view layer_id, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets the lowest zoom at which one layer draws.
 *
 * Pass -INFINITY for no lower bound. MapLibre Native stores the zoom range as
 * single-precision floats, so this narrows the value.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, min_zoom is NaN, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_set_layer_min_zoom(
  mln_map map, mln_buffer_view layer_id, double min_zoom,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets the highest zoom at which one layer draws.
 *
 * Pass INFINITY for no upper bound. MapLibre Native stores the zoom range as
 * single-precision floats, so this narrows the value.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, max_zoom is NaN, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_set_layer_max_zoom(
  mln_map map, mln_buffer_view layer_id, double max_zoom,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets whether one layer draws.
 *
 * visibility is an mln_style_layer_visibility value.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, layer_id is
 *   invalid or empty, visibility is not an mln_style_layer_visibility value,
 *   or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when no style layer has that ID.
 */
MLN_API mln_status mln_map_set_layer_visibility(
  mln_map map, mln_buffer_view layer_id, uint32_t visibility,
  const mln_completion* completion
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_STYLE_H
