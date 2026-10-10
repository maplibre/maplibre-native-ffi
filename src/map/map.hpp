#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <mln/style/image.hpp>

#include "completion/completion_result.hpp"
#include "maplibre_native_c.h"

namespace mln {
class Map;
class RendererObserver;
class TransformState;
class UpdateParameters;
}  // namespace mln

namespace mln::core {

struct GeoJsonSourceDataObject;
struct MapObject;
struct FeatureStateSnapshot;

auto map_options_default() noexcept -> mln_map_options;
auto camera_options_default() noexcept -> mln_camera_options;
auto animation_options_default() noexcept -> mln_animation_options;
auto camera_delta_default() noexcept -> mln_camera_delta;
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
auto custom_mvt_vector_source_options_default() noexcept
  -> mln_custom_mvt_vector_source_options;
auto premultiplied_rgba8_image_default() noexcept
  -> mln_premultiplied_rgba8_image;
auto style_image_options_default() noexcept -> mln_style_image_options;
auto style_transition_options_default() noexcept
  -> mln_style_transition_options;

// A style source's metadata. info holds every scalar member; the strings back
// its views when the result is delivered.
struct StyleSourceRecord {
  mln_style_source_info info{};
  std::string id;
  std::string attribution;
  std::string url;
  std::vector<std::string> tile_urls;
};

// A style layer's metadata. type views the layer type's static string.
struct StyleLayerRecord {
  std::string id;
  std::string_view type;
  std::string source_id;
  std::string source_layer;
  double min_zoom = 0;
  double max_zoom = 0;
  uint32_t visibility = MLN_STYLE_LAYER_VISIBILITY_VISIBLE;
};

// A runtime style image. image shares the style's immutable image data, so the
// record outlives later style changes without copying pixels.
struct StyleImageRecord {
  std::optional<mln::style::Image> image;
  uint32_t fields = 0;
  std::vector<mln_image_stretch> stretch_x;
  std::vector<mln_image_stretch> stretch_y;
};

struct StyleOperationResult {
  bool found = false;
  StyleSourceRecord source;
  std::vector<StyleSourceRecord> sources;
  StyleLayerRecord layer;
  std::vector<StyleLayerRecord> layers;
  StyleImageRecord image;
  mln_style_transition_options transition_options{};
  std::string bytes;
  std::vector<mln_lat_lng> coordinates;
};

using StyleWork = std::function<mln_status(MapObject&, StyleOperationResult&)>;
using StyleDelivery =
  void (*)(const mln_completion&, const StyleOperationResult&) noexcept;

// Views a found image record as the C value. The pixel and stretch views
// borrow from record.
auto style_image_info(const StyleImageRecord& record) noexcept
  -> mln_style_image_info;

// Presents a style read's result as the C value that Function delivers. The
// generated result table picks the arm, so the value type, its shape, and
// whether a missing object completes without a value all come from the
// function's header annotations.
template <auto Function>
auto deliver_style_result(
  const mln_completion& descriptor, const StyleOperationResult& result
) noexcept -> void {
  using Value = CompletionValue<Function>;
  using Type = typename Value::Type;
  const auto view = [](const std::string& text) -> mln_buffer_view {
    return {.data = text.data(), .size = text.size()};
  };
  const auto views = [&view](const std::vector<std::string>& strings) {
    auto result = std::vector<mln_buffer_view>{};
    result.reserve(strings.size());
    for (const auto& string : strings) result.push_back(view(string));
    return result;
  };
  if constexpr (Value::nullable) {
    if (!result.found) {
      Value::deliver_absent(descriptor);
      return;
    }
  }
  if constexpr (std::is_same_v<Type, mln_buffer_view>) {
    Value::deliver(descriptor, view(result.bytes));
  } else if constexpr (std::is_same_v<Type, mln_style_source_info>) {
    // Each info points into the tile URL views it is given, which must outlive
    // the delivery.
    const auto present = [&view](
                           const StyleSourceRecord& source,
                           const std::vector<mln_buffer_view>& tile_urls
                         ) -> mln_style_source_info {
      auto info = source.info;
      info.size = sizeof(mln_style_source_info);
      info.id = view(source.id);
      info.attribution = view(source.attribution);
      info.url = view(source.url);
      info.tilejson.tile_urls = tile_urls.data();
      info.tilejson.tile_url_count = tile_urls.size();
      return info;
    };
    if constexpr (Value::array) {
      auto tile_urls = std::vector<std::vector<mln_buffer_view>>{};
      tile_urls.reserve(result.sources.size());
      auto sources = std::vector<mln_style_source_info>{};
      sources.reserve(result.sources.size());
      for (const auto& source : result.sources) {
        sources.push_back(
          present(source, tile_urls.emplace_back(views(source.tile_urls)))
        );
      }
      Value::deliver(descriptor, sources);
    } else {
      const auto tile_urls = views(result.source.tile_urls);
      Value::deliver(descriptor, present(result.source, tile_urls));
    }
  } else if constexpr (std::is_same_v<Type, mln_style_layer_entry>) {
    auto layers = std::vector<mln_style_layer_entry>{};
    layers.reserve(result.layers.size());
    for (const auto& entry : result.layers) {
      layers.push_back(
        {.size = sizeof(mln_style_layer_entry),
         .id = view(entry.id),
         .type = {.data = entry.type.data(), .size = entry.type.size()},
         .source_id = view(entry.source_id),
         .source_layer = view(entry.source_layer)}
      );
    }
    Value::deliver(descriptor, layers);
  } else if constexpr (std::is_same_v<Type, mln_style_layer_info>) {
    const auto& layer = result.layer;
    Value::deliver(
      descriptor,
      {.size = sizeof(mln_style_layer_info),
       .visibility = layer.visibility,
       .type = {.data = layer.type.data(), .size = layer.type.size()},
       .min_zoom = layer.min_zoom,
       .max_zoom = layer.max_zoom,
       .source_id = view(layer.source_id),
       .source_layer = view(layer.source_layer)}
    );
  } else if constexpr (std::is_same_v<Type, mln_style_image_info>) {
    Value::deliver(descriptor, style_image_info(result.image));
  } else if constexpr (std::is_same_v<Type, mln_style_transition_options>) {
    Value::deliver(descriptor, result.transition_options);
  } else if constexpr (std::is_same_v<Type, mln_lat_lng> && Value::array) {
    Value::deliver(descriptor, result.coordinates);
  } else {
    static_assert(
      sizeof(Type) == 0, "style operation result type has no presentation"
    );
  }
}

auto submit_map_command(
  mln_map map, std::function<mln_status(MapObject&)> work,
  const mln_completion* completion
) -> mln_status;
auto start_style_operation(
  mln_map map, StyleDelivery delivery, StyleWork work,
  const mln_completion* completion
) -> mln_status;

// Runs work on the map's runtime worker and completes with Function's result.
template <auto Function>
auto start_style_operation(
  mln_map map, StyleWork work, const mln_completion* completion
) -> mln_status {
  return start_style_operation(
    map, &deliver_style_result<Function>, std::move(work), completion
  );
}
auto validate_geojson_command_options(const mln_geojson_source_options* options)
  -> mln_status;
enum class TileSourceOptionKind : uint8_t { Vector, Raster, RasterDEM };
auto validate_tile_command_options(
  const mln_style_tile_source_options* options, TileSourceOptionKind kind
) -> mln_status;
auto validate_custom_geometry_command_options(
  const mln_custom_geometry_source_options* options
) -> mln_status;
auto validate_custom_mvt_command_options(
  const mln_custom_mvt_vector_source_options* options
) -> mln_status;
auto validate_style_image_command_input(
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) -> mln_status;
auto validate_image_source_command_coordinates(
  const mln_lat_lng* coordinates, size_t coordinate_count
) -> mln_status;
auto validate_location_indicator_location_command(
  mln_lat_lng coordinate, double altitude
) -> mln_status;
auto validate_location_indicator_bearing_command(double bearing) -> mln_status;
auto validate_location_indicator_accuracy_radius_command(double radius)
  -> mln_status;
auto validate_location_indicator_image_kind(uint32_t image_kind) -> mln_status;

auto create_map_start(
  mln_runtime runtime, const mln_map_options* options,
  const mln_completion* completion
) -> mln_status;
auto dispose_map(mln_map map) -> mln_status;
auto release_map(mln_map map, const mln_completion* completion) -> mln_status;
auto map_snapshot_get(mln_map map, mln_map_snapshot* out_snapshot)
  -> mln_status;
auto map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion
) -> mln_status;
auto map_request_repaint(mln_map map, const mln_completion* completion)
  -> mln_status;
auto map_request_still_image_start(
  mln_map map, const mln_completion* completion
) -> mln_status;
auto map_set_feature_state(
  MapObject& live, const mln_feature_state_selector* selector,
  mln_buffer_view state
) -> mln_status;
auto map_get_feature_state_start(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion
) -> mln_status;
auto map_remove_feature_state(
  MapObject& live, const mln_feature_state_selector* selector
) -> mln_status;
auto map_set_style_url(MapObject& live, const char* url) -> mln_status;
auto map_set_style_json(MapObject& live, mln_buffer_view json) -> mln_status;
auto map_loaded_style_json_start(mln_map map, const mln_completion* completion)
  -> mln_status;
auto map_style_url_start(mln_map map, const mln_completion* completion)
  -> mln_status;
auto map_set_event_mask(
  mln_map map, uint64_t mask, const mln_completion* completion
) -> mln_status;
auto map_add_style_source_json(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view source_json
) -> mln_status;
auto map_remove_style_source(MapObject& live, mln_buffer_view source_id)
  -> mln_status;
auto map_get_style_source(
  MapObject& live, mln_buffer_view source_id, StyleSourceRecord& out_source
) -> bool;
auto map_set_style_source_volatile(
  MapObject& live, mln_buffer_view source_id, bool is_volatile
) -> mln_status;
auto map_list_style_sources(
  MapObject& live, std::vector<StyleSourceRecord>& out_sources
) -> mln_status;
auto map_add_geojson_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geojson_source_options* options
) -> mln_status;
auto map_add_geojson_source_data(
  MapObject& live, mln_buffer_view source_id,
  const std::shared_ptr<const GeoJsonSourceDataObject>& data
) -> mln_status;
auto map_set_geojson_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status;
auto map_set_geojson_source_data(
  MapObject& live, mln_buffer_view source_id,
  const std::shared_ptr<const GeoJsonSourceDataObject>& data
) -> mln_status;
auto map_set_geojson_source_synchronous_tiling(
  MapObject& live, mln_buffer_view source_id, bool enabled
) -> mln_status;
auto map_add_vector_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_vector_source_tiles(
  MapObject& live, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_source_tiles(
  MapObject& live, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_dem_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_raster_dem_source_tiles(
  MapObject& live, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status;
auto map_add_custom_geometry_source(
  MapObject& live, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options
) -> mln_status;
auto map_set_custom_geometry_source_tile_data(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data
) -> mln_status;
auto map_invalidate_custom_geometry_source_tile(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id
) -> mln_status;
auto map_invalidate_custom_geometry_source_region(
  MapObject& live, mln_buffer_view source_id, mln_lat_lng_bounds bounds
) -> mln_status;
auto map_add_custom_mvt_vector_source(
  MapObject& live, mln_buffer_view source_id,
  const mln_custom_mvt_vector_source_options* options
) -> mln_status;
auto map_set_custom_mvt_vector_source_tile_data(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data
) -> mln_status;
auto map_set_custom_mvt_vector_source_tile_error(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message
) -> mln_status;
auto map_invalidate_custom_mvt_vector_source_tile(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id
) -> mln_status;
auto map_set_style_image(
  MapObject& live, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) -> mln_status;
auto map_remove_style_image(MapObject& live, mln_buffer_view image_id)
  -> mln_status;
auto map_get_style_image(
  MapObject& live, mln_buffer_view image_id, StyleImageRecord& out_image
) -> bool;
auto map_add_image_source_url(
  MapObject& live, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_buffer_view url
) -> mln_status;
auto map_add_image_source_image(
  MapObject& live, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image
) -> mln_status;
auto map_set_image_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status;
auto map_set_image_source_image(
  MapObject& live, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image
) -> mln_status;
auto map_set_image_source_coordinates(
  MapObject& live, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count
) -> mln_status;
auto map_get_image_source_coordinates(
  MapObject& live, mln_buffer_view source_id,
  std::vector<mln_lat_lng>& out_coordinates, bool* out_found
) -> mln_status;
auto map_add_hillshade_layer(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) -> mln_status;
auto map_add_color_relief_layer(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) -> mln_status;
auto map_add_location_indicator_layer(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) -> mln_status;
auto map_set_location_indicator_location(
  MapObject& live, mln_buffer_view layer_id, mln_lat_lng coordinate,
  double altitude
) -> mln_status;
auto map_set_location_indicator_bearing(
  MapObject& live, mln_buffer_view layer_id, double bearing
) -> mln_status;
auto map_set_location_indicator_accuracy_radius(
  MapObject& live, mln_buffer_view layer_id, double radius
) -> mln_status;
auto map_set_location_indicator_image_name(
  MapObject& live, mln_buffer_view layer_id, uint32_t image_kind,
  mln_buffer_view image_id
) -> mln_status;
auto map_add_style_layer_json(
  MapObject& live, mln_buffer_view layer_json, mln_buffer_view before_layer_id
) -> mln_status;
auto map_remove_style_layer(MapObject& live, mln_buffer_view layer_id)
  -> mln_status;
auto map_get_style_layer(
  MapObject& live, mln_buffer_view layer_id, StyleLayerRecord& out_layer
) -> bool;
auto map_move_style_layer(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) -> mln_status;
auto map_get_style_layer_json(
  MapObject& live, mln_buffer_view layer_id, mln_buffer* out_layer,
  bool* out_found
) -> mln_status;
auto map_set_global_state_property(
  MapObject& live, mln_buffer_view property_name, mln_buffer_view value
) -> mln_status;
auto map_get_global_state(MapObject& live, mln_buffer* out_state) -> mln_status;

auto map_set_style_light_json(MapObject& live, mln_buffer_view light_json)
  -> mln_status;
auto map_set_style_light_property(
  MapObject& live, mln_buffer_view property_name, mln_buffer_view value
) -> mln_status;
auto map_get_style_light_property(
  MapObject& live, mln_buffer_view property_name, mln_buffer* out_value
) -> mln_status;
auto map_set_style_transition_options(
  MapObject& live, const mln_style_transition_options* options
) -> mln_status;
auto map_get_style_transition_options(
  MapObject& live, mln_style_transition_options* out_options
) -> mln_status;
auto map_set_layer_property(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value
) -> mln_status;
auto map_get_layer_property(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer* out_value
) -> mln_status;
auto map_set_layer_filter(
  MapObject& live, mln_buffer_view layer_id, const mln_buffer_view* filter
) -> mln_status;
auto map_get_layer_filter(
  MapObject& live, mln_buffer_view layer_id, mln_buffer* out_filter
) -> mln_status;
auto map_set_layer_source_layer(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view source_layer
) -> mln_status;
auto map_set_layer_source_id(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view source_id
) -> mln_status;
auto map_set_layer_min_zoom(
  MapObject& live, mln_buffer_view layer_id, double min_zoom
) -> mln_status;
auto map_set_layer_max_zoom(
  MapObject& live, mln_buffer_view layer_id, double max_zoom
) -> mln_status;
auto map_set_layer_visibility(
  MapObject& live, mln_buffer_view layer_id, uint32_t visibility
) -> mln_status;
auto map_camera_snapshot_get(
  mln_map map, mln_camera_options* out_camera, uint64_t* out_generation
) -> mln_status;
auto map_update_camera(
  mln_map map, const mln_camera_update* update, const mln_completion* completion
) -> mln_status;
auto map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion
) -> mln_status;
auto map_cancel_transitions(mln_map map, const mln_completion* completion)
  -> mln_status;
auto map_camera_query_start(mln_map map, const mln_completion* completion)
  -> mln_status;
auto map_set_debug_options(MapObject& live, uint32_t options) -> mln_status;
auto map_set_rendering_stats_view_enabled(MapObject& live, bool enabled)
  -> mln_status;
auto map_dump_debug_logs(MapObject& live) -> mln_status;
auto map_set_viewport_options(
  MapObject& live, const mln_map_viewport_options* options
) -> mln_status;
auto map_set_tile_options(MapObject& live, const mln_map_tile_options* options)
  -> mln_status;
auto map_pixel_for_lat_lng_start(
  mln_map map, mln_lat_lng coordinate, const mln_completion* completion
) -> mln_status;
auto map_lat_lng_for_pixel_start(
  mln_map map, mln_screen_point point, const mln_completion* completion
) -> mln_status;
auto map_lat_lng_for_pixel_unwrapped_start(
  mln_map map, mln_screen_point point, const mln_completion* completion
) -> mln_status;
auto map_pixels_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_completion* completion
) -> mln_status;
auto map_lat_lngs_for_pixels_start(
  mln_map map, const mln_screen_point* points, size_t point_count,
  const mln_completion* completion
) -> mln_status;
auto map_lat_lngs_for_pixels_unwrapped_start(
  mln_map map, const mln_screen_point* points, size_t point_count,
  const mln_completion* completion
) -> mln_status;
auto map_camera_for_lat_lng_bounds_start(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) -> mln_status;
auto map_camera_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) -> mln_status;
auto map_camera_for_geometry_start(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) -> mln_status;
auto map_lat_lng_bounds_for_camera_start(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) -> mln_status;
auto map_lat_lng_bounds_for_camera_unwrapped_start(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) -> mln_status;
auto map_projection_create_from_transform(
  const mln::TransformState& transform, mln_map_projection* out_projection
) -> mln_status;

auto map_projection_create_start(mln_map map, const mln_completion* completion)
  -> mln_status;
auto map_projection_close(mln_map_projection projection) -> mln_status;
auto map_projection_get_camera(
  mln_map_projection projection, mln_camera_options* out_camera
) -> mln_status;
auto map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera
) -> mln_status;
auto map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding
) -> mln_status;
auto map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding
) -> mln_status;
auto map_projection_pixel_for_lat_lng(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_screen_point* out_point
) -> mln_status;
auto map_projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate
) -> mln_status;
auto map_projection_lat_lng_for_pixel_unwrapped(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate
) -> mln_status;
auto map_meters_per_pixel_at_latitude(
  mln_map map, double latitude, const mln_completion* completion
) -> mln_status;
auto map_list_style_layers(
  MapObject& live, std::vector<StyleLayerRecord>& layers
) -> mln_status;

auto map_projection_meters_per_pixel_at_latitude(
  mln_map_projection projection, double latitude, double* out_meters_per_pixel
) -> mln_status;
auto projected_meters_for_lat_lng(
  mln_lat_lng coordinate, mln_projected_meters* out_meters
) -> mln_status;
auto lat_lng_for_projected_meters(
  mln_projected_meters meters, mln_lat_lng* out_coordinate
) -> mln_status;

auto map_camera_for_lat_lng_bounds(
  MapObject& live, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status;
auto map_camera_for_lat_lngs(
  MapObject& live, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status;
auto map_camera_for_geometry(
  MapObject& live, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status;
auto map_lat_lng_bounds_for_camera(
  MapObject& live, const mln_camera_options* camera,
  mln_lat_lng_bounds* out_bounds
) -> mln_status;
auto map_lat_lng_bounds_for_camera_unwrapped(
  MapObject& live, const mln_camera_options* camera,
  mln_lat_lng_bounds* out_bounds
) -> mln_status;
auto map_set_bounds(MapObject& live, const mln_bound_options* options)
  -> mln_status;
auto map_set_free_camera_options(
  MapObject& live, const mln_free_camera_options* options
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
  mln_map map, const mln_projection_mode* mode, const mln_completion* completion
) -> mln_status;
// Validates that a map handle is non-null and live.
auto validate_map_live(mln_map map, MapObject*& out_map) -> mln_status;
auto map_scale_factor(mln_map map) -> double;
// Returns worker-owned native state. Callers must already run on the runtime
// worker or use the posting helpers below.
auto map_native(MapObject& map) -> mln::Map&;

auto map_post_resize(mln_map map, mln_logical_extent extent) -> mln_status;
auto map_post_trigger_repaint(mln_map map) -> mln_status;
auto map_latest_update(mln_map map) -> std::shared_ptr<mln::UpdateParameters>;
auto map_latest_update_generation(mln_map map) noexcept -> uint64_t;
auto map_latest_update_snapshot(mln_map map, uint64_t& out_generation)
  -> std::shared_ptr<mln::UpdateParameters>;
auto map_set_render_session_publish_callback(
  mln_map map, std::function<void()> callback
) -> mln_status;
// Copies the map's coalesced feature-state snapshot. Callable from a render
// session's owner thread while the map is attached to that session.
auto map_feature_state_snapshot(mln_map map)
  -> std::shared_ptr<const FeatureStateSnapshot>;
auto map_renderer_observer(mln_map map) -> mln::RendererObserver*;
// Tells the map which update generation the session is about to render, so a
// frame of an update older than a pending still image request cannot complete
// it. Called on the rendering thread.
auto map_begin_render(mln_map map, uint64_t update_generation) noexcept -> void;
auto map_run_render_jobs(mln_map map) -> void;
// Blocks until the map's queued and running tile-worker jobs drain. Abandon
// uses it so a host may destroy its graphics device as soon as abandon
// returns. Leases the map for the wait, so the caller need only hold a handle
// that was live when the call began. Must not be called from a MapLibre worker
// thread.
auto map_quiesce_render_workers(mln_map map) -> void;
auto map_attach_render_target_session(mln_map map, void* session) -> mln_status;
auto map_detach_render_target_session(mln_map map, void* session) -> mln_status;

}  // namespace mln::core
