#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "maplibre_native_c.h"

namespace mbgl {
class Map;
class RendererObserver;
class UpdateParameters;
}  // namespace mbgl

namespace mln::core {

struct MapObject;

auto map_options_default() noexcept -> mln_map_options;
auto camera_options_default() noexcept -> mln_camera_options;
auto animation_options_default() noexcept -> mln_animation_options;
auto camera_update_default() noexcept -> mln_camera_update;
auto camera_fit_options_default() noexcept -> mln_camera_fit_options;
auto bound_options_default() noexcept -> mln_bound_options;
auto free_camera_options_default() noexcept -> mln_free_camera_options;
auto projection_mode_default() noexcept -> mln_projection_mode;
auto map_viewport_options_default() noexcept -> mln_map_viewport_options;
auto map_tile_options_default() noexcept -> mln_map_tile_options;
auto style_tile_source_options_default() noexcept
  -> mln_style_tile_source_options;
auto geojson_source_options_default() noexcept -> mln_geojson_source_options;
auto custom_geometry_source_options_default() noexcept
  -> mln_custom_geometry_source_options;
auto premultiplied_rgba8_image_default() noexcept
  -> mln_premultiplied_rgba8_image;
auto style_image_options_default() noexcept -> mln_style_image_options;
auto style_image_info_default() noexcept -> mln_style_image_info;
auto style_transition_options_default() noexcept
  -> mln_style_transition_options;

enum class StyleOperationKind : uint32_t {
  RemoveSource = 0x5301,
  SourceExists,
  SourceType,
  SourceInfo,
  SourceAttribution,
  SourceUrl,
  SourceTileUrls,
  SourceIds,
  RemoveImage,
  ImageExists,
  ImageInfo,
  ImageStretches,
  ImagePixels,
  ImageCoordinates,
  RemoveLayer,
  LayerExists,
  LayerType,
  LayerIds,
  LayerJson,
  LightProperty,
  TransitionOptions,
  LayerProperty,
  LayerFilter,
  LayerSourceLayer,
  LayerSourceId,
  LayerMinZoom,
  LayerMaxZoom,
  LayerVisibility
};

struct StyleOperationResult {
  bool flag = false;
  bool found = false;
  uint32_t value_u32 = 0;
  double value_double = 0;
  mln_style_source_info source_info{};
  mln_style_image_info image_info{};
  mln_style_transition_options transition_options{};
  mln_buffer buffer = MLN_HANDLE_NULL;
  mln_style_id_list id_list = MLN_HANDLE_NULL;
  mln_style_string_list string_list = MLN_HANDLE_NULL;
  std::vector<mln_image_stretch> stretch_x;
  std::vector<mln_image_stretch> stretch_y;
  std::vector<mln_lat_lng> coordinates;

  StyleOperationResult() = default;
  StyleOperationResult(const StyleOperationResult&) = delete;
  StyleOperationResult(StyleOperationResult&& other) noexcept;
  auto operator=(const StyleOperationResult&) -> StyleOperationResult& = delete;
  auto operator=(StyleOperationResult&& other) noexcept
    -> StyleOperationResult&;
  ~StyleOperationResult();
};

using StyleWork = std::function<mln_status(StyleOperationResult&)>;

auto submit_map_command(
  mln_map map, std::function<mln_status()> work, uint64_t* out_command_id
) -> mln_status;
auto start_style_operation(
  mln_map map, StyleOperationKind kind, StyleWork work,
  mln_operation* out_operation
) -> mln_status;
auto take_style_operation(
  mln_operation operation, StyleOperationKind kind,
  std::function<mln_status(StyleOperationResult&)> transfer
) -> mln_status;
auto validate_geojson_command_options(const mln_geojson_source_options* options)
  -> mln_status;
auto validate_tile_command_options(
  const mln_style_tile_source_options* options, uint32_t kind
) -> mln_status;
auto validate_custom_geometry_command_options(
  const mln_custom_geometry_source_options* options
) -> mln_status;
auto validate_style_image_command_input(
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) -> mln_status;
auto validate_image_source_command_coordinates(
  const mln_lat_lng* coordinates, size_t coordinate_count
) -> mln_status;

enum class GeometryOperationKind : uint32_t {
  CameraForBounds = 0x4701,
  CameraForCoordinates,
  CameraForGeometry,
  BoundsForCamera,
  UnwrappedBoundsForCamera,
  PixelForCoordinate,
  CoordinateForPixel,
  PixelsForCoordinates,
  CoordinatesForPixels,
  DebugOptions,
  RenderingStats,
  FullyLoaded,
  ViewportOptions,
  TileOptions,
  Bounds,
  FreeCamera
};

struct GeometryOperationResult {
  mln_camera_options camera{};
  mln_lat_lng_bounds bounds{};
  mln_screen_point point{};
  mln_lat_lng coordinate{};
  std::vector<mln_screen_point> points;
  std::vector<mln_lat_lng> coordinates;
  uint32_t value_u32 = 0;
  bool flag = false;
  mln_map_viewport_options viewport{};
  mln_map_tile_options tile{};
  mln_bound_options bound{};
  mln_free_camera_options free_camera{};
};

using GeometryWork = std::function<mln_status(GeometryOperationResult&)>;

auto start_geometry_operation(
  mln_map map, GeometryOperationKind kind, GeometryWork work,
  mln_operation* out_operation
) -> mln_status;
auto map_get_debug_options_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_get_rendering_stats_view_enabled_start(
  mln_map map, mln_operation* out_operation
) -> mln_status;
auto map_is_fully_loaded_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_get_viewport_options_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_get_tile_options_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_get_bounds_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_get_free_camera_options_start(
  mln_map map, mln_operation* out_operation
) -> mln_status;
auto take_geometry_operation(
  mln_operation operation, GeometryOperationKind kind,
  std::function<mln_status(GeometryOperationResult&)> transfer
) -> mln_status;
auto create_map_start(
  mln_runtime runtime, const mln_map_options* options,
  mln_operation* out_operation
) -> mln_status;
auto create_map_take_result(mln_operation operation, mln_map* out_map)
  -> mln_status;
auto map_close_start(mln_map map, mln_operation* out_operation) -> mln_status;
auto map_snapshot_get(mln_map map, mln_map_snapshot* out_snapshot)
  -> mln_status;
auto map_resize(
  mln_map map, mln_logical_extent extent, uint64_t* out_command_id
) -> mln_status;
auto map_request_repaint(mln_map map, uint64_t* out_command_id) -> mln_status;
auto map_request_still_image_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_set_style_url(mln_map map, const char* url) -> mln_status;
auto map_set_style_json(mln_map map, mln_buffer_view json) -> mln_status;
auto map_loaded_style_json_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_loaded_style_json_take_result(
  mln_operation operation, mln_buffer* out_json
) -> mln_status;
auto map_style_url_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_style_url_take_result(mln_operation operation, mln_buffer* out_url)
  -> mln_status;
auto map_set_event_mask(mln_map map, uint64_t mask, uint64_t* out_command_id)
  -> mln_status;
auto style_id_list_count(mln_style_id_list list, size_t* out_count)
  -> mln_status;
auto style_id_list_get(
  mln_style_id_list list, size_t index, mln_buffer_view* out_id
) -> mln_status;
auto style_id_list_destroy(mln_style_id_list list) -> void;
auto style_string_list_count(mln_style_string_list list, size_t* out_count)
  -> mln_status;
auto style_string_list_get(
  mln_style_string_list list, size_t index, mln_buffer_view* out_value
) -> mln_status;
auto style_string_list_destroy(mln_style_string_list list) -> void;
auto map_add_style_source_json(
  mln_map map, mln_buffer_view source_id, mln_buffer_view source_json
) -> mln_status;
auto map_remove_style_source(
  mln_map map, mln_buffer_view source_id, bool* out_removed
) -> mln_status;
auto map_style_source_exists(
  mln_map map, mln_buffer_view source_id, bool* out_exists
) -> mln_status;
auto map_get_style_source_type(
  mln_map map, mln_buffer_view source_id, uint32_t* out_source_type,
  bool* out_found
) -> mln_status;
auto map_get_style_source_info(
  mln_map map, mln_buffer_view source_id, mln_style_source_info* out_info,
  bool* out_found
) -> mln_status;
auto map_copy_style_source_attribution(
  mln_map map, mln_buffer_view source_id, char* out_attribution,
  size_t attribution_capacity, size_t* out_attribution_size, bool* out_found
) -> mln_status;
auto map_copy_style_source_url(
  mln_map map, mln_buffer_view source_id, char* out_url, size_t url_capacity,
  size_t* out_url_size, bool* out_found
) -> mln_status;
auto map_get_style_source_tile_urls(
  mln_map map, mln_buffer_view source_id, mln_style_string_list* out_tile_urls,
  bool* out_found
) -> mln_status;
auto map_list_style_source_ids(mln_map map, mln_style_id_list* out_source_ids)
  -> mln_status;
auto map_add_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geojson_source_options* options
) -> mln_status;
auto map_add_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_buffer_view data,
  const mln_geojson_source_options* options
) -> mln_status;
auto map_set_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status;
auto map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_buffer_view data
) -> mln_status;
auto map_add_vector_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_vector_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_dem_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_dem_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options
) -> mln_status;
auto map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data
) -> mln_status;
auto map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id
) -> mln_status;
auto map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds
) -> mln_status;
auto map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) -> mln_status;
auto map_remove_style_image(
  mln_map map, mln_buffer_view image_id, bool* out_removed
) -> mln_status;
auto map_style_image_exists(
  mln_map map, mln_buffer_view image_id, bool* out_exists
) -> mln_status;
auto map_get_style_image_info(
  mln_map map, mln_buffer_view image_id, mln_style_image_info* out_info,
  bool* out_found
) -> mln_status;
auto map_copy_style_image_stretches(
  mln_map map, mln_buffer_view image_id, mln_image_stretch* out_stretch_x,
  size_t stretch_x_capacity, size_t* out_stretch_x_count,
  mln_image_stretch* out_stretch_y, size_t stretch_y_capacity,
  size_t* out_stretch_y_count, bool* out_found
) -> mln_status;
auto map_copy_style_image_premultiplied_rgba8(
  mln_map map, mln_buffer_view image_id, uint8_t* out_pixels,
  size_t pixel_capacity, size_t* out_byte_length, bool* out_found
) -> mln_status;
auto map_add_image_source_url(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_buffer_view url
) -> mln_status;
auto map_add_image_source_image(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image
) -> mln_status;
auto map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status;
auto map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image
) -> mln_status;
auto map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count
) -> mln_status;
auto map_get_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, mln_lat_lng* out_coordinates,
  size_t coordinate_capacity, size_t* out_coordinate_count, bool* out_found
) -> mln_status;
auto map_add_hillshade_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) -> mln_status;
auto map_add_color_relief_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) -> mln_status;
auto map_add_location_indicator_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) -> mln_status;
auto map_set_location_indicator_location(
  mln_map map, mln_buffer_view layer_id, mln_lat_lng coordinate, double altitude
) -> mln_status;
auto map_set_location_indicator_bearing(
  mln_map map, mln_buffer_view layer_id, double bearing
) -> mln_status;
auto map_set_location_indicator_accuracy_radius(
  mln_map map, mln_buffer_view layer_id, double radius
) -> mln_status;
auto map_set_location_indicator_image_name(
  mln_map map, mln_buffer_view layer_id, uint32_t image_kind,
  mln_buffer_view image_id
) -> mln_status;
auto map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json, mln_buffer_view before_layer_id
) -> mln_status;
auto map_remove_style_layer(
  mln_map map, mln_buffer_view layer_id, bool* out_removed
) -> mln_status;
auto map_style_layer_exists(
  mln_map map, mln_buffer_view layer_id, bool* out_exists
) -> mln_status;
auto map_get_style_layer_type(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view* out_layer_type,
  bool* out_found
) -> mln_status;
auto map_list_style_layer_ids(mln_map map, mln_style_id_list* out_layer_ids)
  -> mln_status;
auto map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) -> mln_status;
auto map_get_style_layer_json(
  mln_map map, mln_buffer_view layer_id, mln_buffer* out_layer, bool* out_found
) -> mln_status;
auto map_set_style_light_json(mln_map map, mln_buffer_view light_json)
  -> mln_status;
auto map_set_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer_view value
) -> mln_status;
auto map_get_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer* out_value
) -> mln_status;
auto map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options
) -> mln_status;
auto map_get_style_transition_options(
  mln_map map, mln_style_transition_options* out_options
) -> mln_status;
auto map_set_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value
) -> mln_status;
auto map_get_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer* out_value
) -> mln_status;
auto map_set_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_buffer_view* filter
) -> mln_status;
auto map_get_layer_filter(
  mln_map map, mln_buffer_view layer_id, mln_buffer* out_filter
) -> mln_status;
auto map_set_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_layer
) -> mln_status;
auto map_copy_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, char* out_source_layer,
  size_t source_layer_capacity, size_t* out_source_layer_size
) -> mln_status;
auto map_set_layer_source_id(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id
) -> mln_status;
auto map_copy_layer_source_id(
  mln_map map, mln_buffer_view layer_id, char* out_source_id,
  size_t source_id_capacity, size_t* out_source_id_size
) -> mln_status;
auto map_set_layer_min_zoom(
  mln_map map, mln_buffer_view layer_id, double min_zoom
) -> mln_status;
auto map_get_layer_min_zoom(
  mln_map map, mln_buffer_view layer_id, double* out_min_zoom
) -> mln_status;
auto map_set_layer_max_zoom(
  mln_map map, mln_buffer_view layer_id, double max_zoom
) -> mln_status;
auto map_get_layer_max_zoom(
  mln_map map, mln_buffer_view layer_id, double* out_max_zoom
) -> mln_status;
auto map_set_layer_visibility(
  mln_map map, mln_buffer_view layer_id, uint32_t visibility
) -> mln_status;
auto map_get_layer_visibility(
  mln_map map, mln_buffer_view layer_id, uint32_t* out_visibility
) -> mln_status;
auto map_camera_snapshot_get(
  mln_map map, mln_camera_options* out_camera, uint64_t* out_generation
) -> mln_status;
auto map_update_camera(
  mln_map map, const mln_camera_update* update, uint64_t* out_command_id
) -> mln_status;
auto map_camera_query_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_camera_query_take_result(
  mln_operation operation, mln_camera_query_result* out_result
) -> mln_status;
auto map_set_debug_options(mln_map map, uint32_t options) -> mln_status;
auto map_get_debug_options(mln_map map, uint32_t* out_options) -> mln_status;
auto map_set_rendering_stats_view_enabled(mln_map map, bool enabled)
  -> mln_status;
auto map_get_rendering_stats_view_enabled(mln_map map, bool* out_enabled)
  -> mln_status;
auto map_is_fully_loaded(mln_map map, bool* out_loaded) -> mln_status;
auto map_dump_debug_logs(mln_map map) -> mln_status;
auto map_get_size(
  mln_map map, uint32_t* out_width, uint32_t* out_height,
  double* out_scale_factor
) -> mln_status;
auto map_get_viewport_options(
  mln_map map, mln_map_viewport_options* out_options
) -> mln_status;
auto map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options
) -> mln_status;
auto map_get_tile_options(mln_map map, mln_map_tile_options* out_options)
  -> mln_status;
auto map_set_tile_options(mln_map map, const mln_map_tile_options* options)
  -> mln_status;
auto map_pixel_for_lat_lng_start(
  mln_map map, mln_lat_lng coordinate, mln_operation* out_operation
) -> mln_status;
auto map_pixel_for_lat_lng_take_result(
  mln_operation operation, mln_screen_point* out_point
) -> mln_status;
auto map_lat_lng_for_pixel_start(
  mln_map map, mln_screen_point point, mln_operation* out_operation
) -> mln_status;
auto map_lat_lng_for_pixel_take_result(
  mln_operation operation, mln_lat_lng* out_coordinate
) -> mln_status;
auto map_pixels_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  mln_operation* out_operation
) -> mln_status;
auto map_pixels_for_lat_lngs_take_result(
  mln_operation operation, mln_screen_point* out_points, size_t point_capacity,
  size_t* out_point_count
) -> mln_status;
auto map_lat_lngs_for_pixels_start(
  mln_map map, const mln_screen_point* points, size_t point_count,
  mln_operation* out_operation
) -> mln_status;
auto map_lat_lngs_for_pixels_take_result(
  mln_operation operation, mln_lat_lng* out_coordinates,
  size_t coordinate_capacity, size_t* out_coordinate_count
) -> mln_status;
auto map_camera_for_lat_lng_bounds_start(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) -> mln_status;
auto map_camera_for_lat_lng_bounds_take_result(
  mln_operation operation, mln_camera_options* out_camera
) -> mln_status;
auto map_camera_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) -> mln_status;
auto map_camera_for_lat_lngs_take_result(
  mln_operation operation, mln_camera_options* out_camera
) -> mln_status;
auto map_camera_for_geometry_start(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) -> mln_status;
auto map_camera_for_geometry_take_result(
  mln_operation operation, mln_camera_options* out_camera
) -> mln_status;
auto map_lat_lng_bounds_for_camera_start(
  mln_map map, const mln_camera_options* camera, mln_operation* out_operation
) -> mln_status;
auto map_lat_lng_bounds_for_camera_take_result(
  mln_operation operation, mln_lat_lng_bounds* out_bounds
) -> mln_status;
auto map_lat_lng_bounds_for_camera_unwrapped_start(
  mln_map map, const mln_camera_options* camera, mln_operation* out_operation
) -> mln_status;
auto map_lat_lng_bounds_for_camera_unwrapped_take_result(
  mln_operation operation, mln_lat_lng_bounds* out_bounds
) -> mln_status;
auto map_projection_create_start(mln_map map, mln_operation* out_operation)
  -> mln_status;
auto map_projection_create_take_result(
  mln_operation operation, mln_map_projection* out_projection
) -> mln_status;
auto map_projection_close_start(
  mln_map_projection projection, mln_operation* out_operation
) -> mln_status;
auto map_projection_get_camera_start(
  mln_map_projection projection, mln_operation* out_operation
) -> mln_status;
auto map_projection_get_camera_take_result(
  mln_operation operation, mln_camera_options* out_camera
) -> mln_status;
auto map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera,
  uint64_t* out_command_id
) -> mln_status;
auto map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding, uint64_t* out_command_id
) -> mln_status;
auto map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding, uint64_t* out_command_id
) -> mln_status;
auto map_projection_pixel_for_lat_lng_start(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_operation* out_operation
) -> mln_status;
auto map_projection_pixel_for_lat_lng_take_result(
  mln_operation operation, mln_screen_point* out_point
) -> mln_status;
auto map_projection_lat_lng_for_pixel_start(
  mln_map_projection projection, mln_screen_point point,
  mln_operation* out_operation
) -> mln_status;
auto map_projection_lat_lng_for_pixel_take_result(
  mln_operation operation, mln_lat_lng* out_coordinate
) -> mln_status;
auto projected_meters_for_lat_lng(
  mln_lat_lng coordinate, mln_projected_meters* out_meters
) -> mln_status;
auto lat_lng_for_projected_meters(
  mln_projected_meters meters, mln_lat_lng* out_coordinate
) -> mln_status;

auto map_camera_for_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status;
auto map_camera_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status;
auto map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status;
auto map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera, mln_lat_lng_bounds* out_bounds
) -> mln_status;
auto map_lat_lng_bounds_for_camera_unwrapped(
  mln_map map, const mln_camera_options* camera, mln_lat_lng_bounds* out_bounds
) -> mln_status;
auto map_get_bounds(mln_map map, mln_bound_options* out_options) -> mln_status;
auto map_set_bounds(mln_map map, const mln_bound_options* options)
  -> mln_status;
auto map_get_free_camera_options(
  mln_map map, mln_free_camera_options* out_options
) -> mln_status;
auto map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options
) -> mln_status;
auto validate_debug_options_input(uint32_t options) -> mln_status;
auto validate_viewport_options_input(const mln_map_viewport_options* options)
  -> mln_status;
auto validate_tile_options_input(const mln_map_tile_options* options)
  -> mln_status;
auto validate_bound_options_input(const mln_bound_options* options)
  -> mln_status;
auto validate_free_camera_options_input(const mln_free_camera_options* options)
  -> mln_status;
auto map_set_projection_mode(
  mln_map map, const mln_projection_mode* mode, uint64_t* out_command_id
) -> mln_status;
// Validates that a map handle is non-null and live.
auto validate_map_live(mln_map map, MapObject*& out_map) -> mln_status;
auto validate_map(mln_map map, MapObject*& out_map) -> mln_status;
auto map_scale_factor(mln_map map) -> double;
// Returns worker-owned native state. Callers must already run on the runtime
// worker or use the posting helpers below.
auto map_native(MapObject* map) -> mbgl::Map*;

auto map_post_resize(mln_map map, mln_logical_extent extent) -> mln_status;
auto map_post_trigger_repaint(mln_map map) -> mln_status;
auto map_latest_update(mln_map map) -> std::shared_ptr<mbgl::UpdateParameters>;
auto map_renderer_observer(mln_map map) -> mbgl::RendererObserver*;
auto map_run_render_jobs(mln_map map) -> void;
auto map_attach_render_target_session(mln_map map, void* session) -> mln_status;
auto map_detach_render_target_session(mln_map map, void* session) -> mln_status;

}  // namespace mln::core
