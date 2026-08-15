#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <ratio>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <mbgl/actor/actor_ref.hpp>
#include <mbgl/actor/mailbox.hpp>
#include <mbgl/actor/scheduler.hpp>
#include <mbgl/gfx/rendering_stats.hpp>
#include <mbgl/map/bound_options.hpp>
#include <mbgl/map/camera.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/map/map_projection.hpp>
#include <mbgl/map/mode.hpp>
#include <mbgl/map/projection_mode.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/renderer/update_parameters.hpp>
#include <mbgl/style/conversion.hpp>
#include <mbgl/style/conversion/geojson_options.hpp>  // IWYU pragma: keep
#include <mbgl/style/conversion/json.hpp>
#include <mbgl/style/conversion/layer.hpp>   // IWYU pragma: keep
#include <mbgl/style/conversion/light.hpp>   // IWYU pragma: keep
#include <mbgl/style/conversion/source.hpp>  // IWYU pragma: keep
#include <mbgl/style/conversion_impl.hpp>
#include <mbgl/style/image.hpp>
#include <mbgl/style/layer.hpp>
#include <mbgl/style/layers/color_relief_layer.hpp>
#include <mbgl/style/layers/hillshade_layer.hpp>
#include <mbgl/style/layers/location_indicator_layer.hpp>
#include <mbgl/style/light.hpp>
#include <mbgl/style/rapidjson_conversion.hpp>
#include <mbgl/style/source.hpp>
#include <mbgl/style/sources/custom_geometry_source.hpp>
#include <mbgl/style/sources/geojson_source.hpp>
#include <mbgl/style/sources/image_source.hpp>
#include <mbgl/style/sources/raster_dem_source.hpp>
#include <mbgl/style/sources/raster_source.hpp>
#include <mbgl/style/sources/vector_source.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/style/style_property.hpp>
#include <mbgl/style/transition_options.hpp>
#include <mbgl/style/types.hpp>
#include <mbgl/tile/tile_id.hpp>
#include <mbgl/tile/tile_operation.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/immutable.hpp>
#include <mbgl/util/projection.hpp>
#include <mbgl/util/range.hpp>
#include <mbgl/util/size.hpp>
#include <mbgl/util/tileset.hpp>
#include <mbgl/util/vectors.hpp>

#include "bytes/buffer.hpp"
#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"
#include "style/style_value.hpp"

namespace mln::core {

struct StyleIdListObject {
  std::vector<std::string> ids;
};

template <>
struct HandleTraits<StyleIdListObject> {
  static constexpr auto kind = HandleKind::StyleIdList;
  static constexpr auto leasable = false;
};

struct StyleStringListObject {
  std::vector<std::string> values;
};

template <>
struct HandleTraits<StyleStringListObject> {
  static constexpr auto kind = HandleKind::StyleStringList;
  static constexpr auto leasable = false;
};

auto validate_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln_status;
auto validate_lat_lng_array(
  const mln_lat_lng* coordinates, size_t coordinate_count, bool allow_empty
) -> mln_status;
auto to_native_lat_lng(mln_lat_lng coordinate) -> mbgl::LatLng;
auto from_native_lat_lng(const mbgl::LatLng& coordinate) -> mln_lat_lng;
auto to_native_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mbgl::LatLngBounds;
auto from_native_lat_lng_bounds(const mbgl::LatLngBounds& bounds)
  -> mln_lat_lng_bounds;

}  // namespace mln::core

namespace {

auto buffer_view_from_string(const std::string& value) -> mln_buffer_view {
  return {
    .data = value.data(),
    .size = value.size(),
  };
}

enum class TileSourceOptionKind : uint8_t { Vector, Raster, RasterDEM };

auto validate_string_view(mln_buffer_view string, const char* name) -> bool {
  if (string.size > 0 && string.data == nullptr) {
    auto message = std::string{name} + " data must not be null";
    mln::core::set_thread_error(message.c_str());
    return false;
  }
  return true;
}

auto string_from_view(mln_buffer_view string) -> std::string {
  if (string.size == 0) {
    return {};
  }
  return std::string{static_cast<const char*>(string.data), string.size};
}

auto string_view_from_string(const std::string& string) -> mln_buffer_view {
  return mln_buffer_view{.data = string.data(), .size = string.size()};
}

auto string_view_from_literal(const char* string) -> mln_buffer_view {
  return mln_buffer_view{.data = string, .size = std::strlen(string)};
}

auto to_c_source_type(mbgl::style::SourceType type) -> uint32_t {
  switch (type) {
    case mbgl::style::SourceType::Vector:
      return MLN_STYLE_SOURCE_TYPE_VECTOR;
    case mbgl::style::SourceType::Raster:
      return MLN_STYLE_SOURCE_TYPE_RASTER;
    case mbgl::style::SourceType::RasterDEM:
      return MLN_STYLE_SOURCE_TYPE_RASTER_DEM;
    case mbgl::style::SourceType::GeoJSON:
      return MLN_STYLE_SOURCE_TYPE_GEOJSON;
    case mbgl::style::SourceType::Video:
      return MLN_STYLE_SOURCE_TYPE_VIDEO;
    case mbgl::style::SourceType::Annotations:
      return MLN_STYLE_SOURCE_TYPE_ANNOTATIONS;
    case mbgl::style::SourceType::Image:
      return MLN_STYLE_SOURCE_TYPE_IMAGE;
    case mbgl::style::SourceType::CustomVector:
      return MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR;
    case mbgl::style::SourceType::CustomMVTVector:
      return MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR;
  }
  assert(false);
  return MLN_STYLE_SOURCE_TYPE_UNKNOWN;
}

auto to_c_tile_scheme(mbgl::Tileset::Scheme scheme) -> uint32_t {
  switch (scheme) {
    case mbgl::Tileset::Scheme::XYZ:
      return MLN_STYLE_TILE_SCHEME_XYZ;
    case mbgl::Tileset::Scheme::TMS:
      return MLN_STYLE_TILE_SCHEME_TMS;
  }
  assert(false);
  return MLN_STYLE_TILE_SCHEME_XYZ;
}

auto to_c_vector_encoding(mbgl::Tileset::VectorEncoding encoding) -> uint32_t {
  switch (encoding) {
    case mbgl::Tileset::VectorEncoding::Mapbox:
      return MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
    case mbgl::Tileset::VectorEncoding::MLT:
      return MLN_STYLE_VECTOR_TILE_ENCODING_MLT;
  }
  assert(false);
  return MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
}

auto to_c_raster_encoding(mbgl::Tileset::RasterEncoding encoding) -> uint32_t {
  switch (encoding) {
    case mbgl::Tileset::RasterEncoding::Mapbox:
      return MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
    case mbgl::Tileset::RasterEncoding::Terrarium:
      return MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM;
  }
  assert(false);
  return MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
}

auto tile_source_from_source(const mbgl::style::Source& source)
  -> const mbgl::style::TileSource* {
  switch (source.getType()) {
    case mbgl::style::SourceType::Vector:
      return source.as<mbgl::style::VectorSource>();
    case mbgl::style::SourceType::Raster:
      return source.as<mbgl::style::RasterSource>();
    case mbgl::style::SourceType::RasterDEM:
      return source.as<mbgl::style::RasterDEMSource>();
    default:
      return nullptr;
  }
}

auto inline_tileset(const mbgl::style::TileSource& source)
  -> const mbgl::Tileset* {
  const auto& url_or_tileset = source.getURLOrTileset();
  return url_or_tileset.is<mbgl::Tileset>()
           ? &url_or_tileset.get<mbgl::Tileset>()
           : nullptr;
}

auto source_url(const mbgl::style::Source& source)
  -> std::optional<std::string> {
  if (const auto* tile_source = tile_source_from_source(source)) {
    return tile_source->getURL();
  }
  if (const auto* geojson = source.as<mbgl::style::GeoJSONSource>()) {
    return geojson->getURL();
  }
  if (const auto* image = source.as<mbgl::style::ImageSource>()) {
    return image->getURL();
  }
  return std::nullopt;
}

auto has_tile_source_option(
  const mln_style_tile_source_options& options, uint32_t field
) -> bool {
  return (options.fields & field) != 0U;
}

auto validate_zoom_option(double zoom, const char* name) -> mln_status {
  if (!std::isfinite(zoom) || zoom < 0.0 || zoom > 255.0) {
    auto message = std::string{name} + " must be finite and within [0, 255]";
    mln::core::set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_tile_source_option_header(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (options.size < sizeof(mln_style_tile_source_options)) {
    mln::core::set_thread_error(
      "mln_style_tile_source_options.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM) |
    MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM |
    MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION |
    MLN_STYLE_TILE_SOURCE_OPTION_SCHEME | MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS |
    MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE |
    MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING |
    MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  if ((options.fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_style_tile_source_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_tile_source_zoom_options(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM)) {
    const auto status = validate_zoom_option(options.min_zoom, "min_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM)) {
    const auto status = validate_zoom_option(options.max_zoom, "max_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM) &&
    has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM) &&
    options.min_zoom > options.max_zoom
  ) {
    mln::core::set_thread_error(
      "min_zoom must be less than or equal to max_zoom"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_tile_source_attribution_option(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (!has_tile_source_option(
        options, MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
      )) {
    return MLN_STATUS_OK;
  }
  return validate_string_view(options.attribution, "attribution")
           ? MLN_STATUS_OK
           : MLN_STATUS_INVALID_ARGUMENT;
}

auto validate_tile_source_scheme_option(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (!has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_SCHEME)) {
    return MLN_STATUS_OK;
  }
  switch (options.scheme) {
    case MLN_STYLE_TILE_SCHEME_XYZ:
    case MLN_STYLE_TILE_SCHEME_TMS:
      return MLN_STATUS_OK;
    default:
      mln::core::set_thread_error("scheme is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
}

auto validate_tile_source_bounds_option(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (!has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS)) {
    return MLN_STATUS_OK;
  }
  return mln::core::validate_lat_lng_bounds(options.bounds);
}

auto validate_tile_source_tile_size_option(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (!has_tile_source_option(
        options, MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
      )) {
    return MLN_STATUS_OK;
  }
  if (options.tile_size == 0 || options.tile_size > 65535U) {
    mln::core::set_thread_error("tile_size must be within [1, 65535]");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_tile_source_vector_encoding_option(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (!has_tile_source_option(
        options, MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
      )) {
    return MLN_STATUS_OK;
  }
  switch (options.vector_encoding) {
    case MLN_STYLE_VECTOR_TILE_ENCODING_MVT:
    case MLN_STYLE_VECTOR_TILE_ENCODING_MLT:
      return MLN_STATUS_OK;
    default:
      mln::core::set_thread_error("vector_encoding is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
}

auto validate_tile_source_raster_encoding_option(
  const mln_style_tile_source_options& options
) -> mln_status {
  if (!has_tile_source_option(
        options, MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
      )) {
    return MLN_STATUS_OK;
  }
  switch (options.raster_encoding) {
    case MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX:
    case MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM:
      return MLN_STATUS_OK;
    default:
      mln::core::set_thread_error("raster_encoding is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
}

auto validate_tile_source_option_kind(
  const mln_style_tile_source_options& options, TileSourceOptionKind kind
) -> mln_status {
  if (
    kind != TileSourceOptionKind::Vector &&
    has_tile_source_option(
      options, MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
    )
  ) {
    mln::core::set_thread_error(
      "vector_encoding is only valid for vector sources"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    kind != TileSourceOptionKind::RasterDEM &&
    has_tile_source_option(
      options, MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
    )
  ) {
    mln::core::set_thread_error(
      "raster_encoding is only valid for raster DEM sources"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_tile_source_options(
  const mln_style_tile_source_options* options, TileSourceOptionKind kind
) -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  for (const auto validator : {
         validate_tile_source_option_header,
         validate_tile_source_zoom_options,
         validate_tile_source_attribution_option,
         validate_tile_source_scheme_option,
         validate_tile_source_bounds_option,
         validate_tile_source_tile_size_option,
         validate_tile_source_vector_encoding_option,
         validate_tile_source_raster_encoding_option,
       }) {
    const auto status = validator(*options);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  return validate_tile_source_option_kind(*options, kind);
}

auto effective_tile_source_options(const mln_style_tile_source_options* options)
  -> mln_style_tile_source_options {
  auto result = mln::core::style_tile_source_options_default();
  if (options == nullptr) {
    return result;
  }

  result.fields = options->fields;
  if (has_tile_source_option(*options, MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM)) {
    result.min_zoom = options->min_zoom;
  }
  if (has_tile_source_option(*options, MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM)) {
    result.max_zoom = options->max_zoom;
  }
  if (
    has_tile_source_option(*options, MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION)
  ) {
    result.attribution = options->attribution;
  }
  if (has_tile_source_option(*options, MLN_STYLE_TILE_SOURCE_OPTION_SCHEME)) {
    result.scheme = options->scheme;
  }
  if (has_tile_source_option(*options, MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS)) {
    result.bounds = options->bounds;
  }
  if (
    has_tile_source_option(*options, MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE)
  ) {
    result.tile_size = options->tile_size;
  }
  if (
    has_tile_source_option(
      *options, MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
    )
  ) {
    result.vector_encoding = options->vector_encoding;
  }
  if (
    has_tile_source_option(
      *options, MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
    )
  ) {
    result.raster_encoding = options->raster_encoding;
  }
  return result;
}

auto to_native_tile_scheme(uint32_t scheme) -> mbgl::Tileset::Scheme {
  return scheme == MLN_STYLE_TILE_SCHEME_TMS ? mbgl::Tileset::Scheme::TMS
                                             : mbgl::Tileset::Scheme::XYZ;
}

auto to_native_vector_encoding(uint32_t encoding)
  -> mbgl::Tileset::VectorEncoding {
  return encoding == MLN_STYLE_VECTOR_TILE_ENCODING_MLT
           ? mbgl::Tileset::VectorEncoding::MLT
           : mbgl::Tileset::VectorEncoding::Mapbox;
}

auto to_native_raster_encoding(uint32_t encoding)
  -> mbgl::Tileset::RasterEncoding {
  return encoding == MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM
           ? mbgl::Tileset::RasterEncoding::Terrarium
           : mbgl::Tileset::RasterEncoding::Mapbox;
}

auto validate_tile_urls(const mln_buffer_view* tiles, size_t tile_count)
  -> mln_status {
  if (tile_count == 0) {
    mln::core::set_thread_error("tile_count must be greater than 0");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (tiles == nullptr) {
    mln::core::set_thread_error("tiles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  for (const auto tile : std::span<const mln_buffer_view>{tiles, tile_count}) {
    if (!validate_string_view(tile, "tile URL")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (tile.size == 0) {
      mln::core::set_thread_error("tile URLs must not be empty");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  return MLN_STATUS_OK;
}

auto to_native_tile_urls(const mln_buffer_view* tiles, size_t tile_count)
  -> std::vector<std::string> {
  auto result = std::vector<std::string>{};
  result.reserve(tile_count);
  for (const auto tile : std::span<const mln_buffer_view>{tiles, tile_count}) {
    result.push_back(string_from_view(tile));
  }
  return result;
}

auto to_native_tileset(
  const mln_buffer_view* tiles, size_t tile_count,
  const mln_style_tile_source_options& options, bool vector_source
) -> std::optional<mbgl::Tileset> {
  if (options.min_zoom > options.max_zoom) {
    mln::core::set_thread_error(
      "effective min_zoom must be less than or equal to max_zoom"
    );
    return std::nullopt;
  }

  auto tileset = mbgl::Tileset{
    to_native_tile_urls(tiles, tile_count),
    mbgl::Range<uint8_t>{
      static_cast<uint8_t>(options.min_zoom),
      static_cast<uint8_t>(options.max_zoom)
    },
    string_from_view(options.attribution),
    to_native_tile_scheme(options.scheme),
    std::nullopt,
    vector_source
      ? std::optional<mbgl::Tileset::VectorEncoding>{to_native_vector_encoding(
          options.vector_encoding
        )}
      : std::nullopt
  };
  if (has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS)) {
    tileset.bounds = mln::core::to_native_lat_lng_bounds(options.bounds);
  }
  return tileset;
}

auto has_geojson_source_option(
  const mln_geojson_source_options& options, uint32_t field
) -> bool {
  return (options.fields & field) != 0U;
}

auto effective_geojson_source_options(const mln_geojson_source_options* options)
  -> mln_geojson_source_options {
  auto result = mln::core::geojson_source_options_default();
  if (options == nullptr) {
    return result;
  }

  result.fields = options->fields;
  if (has_geojson_source_option(*options, MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM)) {
    result.min_zoom = options->min_zoom;
  }
  if (has_geojson_source_option(*options, MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM)) {
    result.max_zoom = options->max_zoom;
  }
  if (
    has_geojson_source_option(*options, MLN_GEOJSON_SOURCE_OPTION_TOLERANCE)
  ) {
    result.tolerance = options->tolerance;
  }
  if (
    has_geojson_source_option(
      *options, MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
    )
  ) {
    result.cluster_max_zoom = options->cluster_max_zoom;
  }
  if (
    has_geojson_source_option(
      *options, MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
    )
  ) {
    result.cluster_properties = options->cluster_properties;
  }
  if (
    has_geojson_source_option(*options, MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE)
  ) {
    result.tile_size = options->tile_size;
  }
  if (has_geojson_source_option(*options, MLN_GEOJSON_SOURCE_OPTION_BUFFER)) {
    result.buffer = options->buffer;
  }
  if (
    has_geojson_source_option(
      *options, MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
    )
  ) {
    result.cluster_radius = options->cluster_radius;
  }
  if (
    has_geojson_source_option(
      *options, MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
    )
  ) {
    result.cluster_min_points = options->cluster_min_points;
  }
  if (
    has_geojson_source_option(*options, MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS)
  ) {
    result.line_metrics = options->line_metrics;
  }
  if (has_geojson_source_option(*options, MLN_GEOJSON_SOURCE_OPTION_CLUSTER)) {
    result.cluster = options->cluster;
  }
  if (
    has_geojson_source_option(
      *options, MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE
    )
  ) {
    result.synchronous_update = options->synchronous_update;
  }
  return result;
}

auto validate_geojson_source_options(const mln_geojson_source_options* options)
  -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_geojson_source_options)) {
    mln::core::set_thread_error("mln_geojson_source_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM) |
    MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM | MLN_GEOJSON_SOURCE_OPTION_TOLERANCE |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES |
    MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE | MLN_GEOJSON_SOURCE_OPTION_BUFFER |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
    MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS | MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
    MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_geojson_source_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto effective = effective_geojson_source_options(options);
  for (const auto& [zoom, name] : {
         std::pair{effective.min_zoom, "min_zoom"},
         std::pair{effective.max_zoom, "max_zoom"},
         std::pair{effective.cluster_max_zoom, "cluster_max_zoom"},
       }) {
    if (
      !std::isfinite(zoom) || zoom < 0.0 || zoom > 255.0 ||
      std::floor(zoom) != zoom
    ) {
      auto message = std::string{name} + " must be an integer within [0, 255]";
      mln::core::set_thread_error(message.c_str());
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if (effective.min_zoom > effective.max_zoom) {
    mln::core::set_thread_error(
      "min_zoom must be less than or equal to max_zoom"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!std::isfinite(effective.tolerance) || effective.tolerance < 0.0) {
    mln::core::set_thread_error("tolerance must be finite and non-negative");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.tile_size == 0 || effective.tile_size > 65535U) {
    mln::core::set_thread_error("tile_size must be within [1, 65535]");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.buffer > 65535U) {
    mln::core::set_thread_error("buffer must be at most 65535");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.cluster_radius > 65535U) {
    mln::core::set_thread_error("cluster_radius must be at most 65535");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    has_geojson_source_option(
      *options, MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
    ) &&
    effective.cluster_properties.size == 0
  ) {
    mln::core::set_thread_error(
      "cluster_properties must not be empty when its field is present"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

// Cluster properties reach Converter<GeoJSONOptions> as a one-member object,
// which is what parses the aggregation expressions.
auto to_native_geojson_source_options(const mln_geojson_source_options& options)
  -> std::optional<mbgl::Immutable<mbgl::style::GeoJSONOptions>> {
  auto native = mbgl::style::GeoJSONOptions{};
  native.minzoom = static_cast<uint8_t>(options.min_zoom);
  native.maxzoom = static_cast<uint8_t>(options.max_zoom);
  native.tileSize = static_cast<uint16_t>(options.tile_size);
  native.buffer = static_cast<uint16_t>(options.buffer);
  native.tolerance = options.tolerance;
  native.lineMetrics = options.line_metrics;
  native.cluster = options.cluster;
  native.clusterRadius = static_cast<uint16_t>(options.cluster_radius);
  native.clusterMaxZoom = static_cast<uint8_t>(options.cluster_max_zoom);
  native.clusterMinPoints = options.cluster_min_points;
  native.synchronousUpdate = options.synchronous_update;

  if (options.cluster_properties.size != 0) {
    auto document = mbgl::JSDocument{};
    if (!mln::core::parse_json_document(
          options.cluster_properties, "cluster_properties", document
        )) {
      return std::nullopt;
    }
    if (!document.IsObject()) {
      mln::core::set_thread_error(
        "cluster_properties must contain a JSON object"
      );
      return std::nullopt;
    }
    auto wrapper = std::string{"{\"clusterProperties\":"};
    wrapper.append(
      static_cast<const char*>(options.cluster_properties.data),
      options.cluster_properties.size
    );
    wrapper.push_back('}');
    auto error = mbgl::style::conversion::Error{};
    auto converted =
      mbgl::style::conversion::convertJSON<mbgl::style::GeoJSONOptions>(
        wrapper, error
      );
    if (!converted) {
      mln::core::set_style_conversion_error("GeoJSON source options", error);
      return std::nullopt;
    }
    native.clusterProperties = std::move(converted->clusterProperties);
  }

  return mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(native));
}

auto geojson_geometry_type_name(const mbgl::Geometry<double>& geometry)
  -> std::string_view {
  return geometry.match(
    [](const mbgl::EmptyGeometry&) -> std::string_view { return "empty"; },
    [](const mbgl::Point<double>&) -> std::string_view { return "point"; },
    [](const mbgl::LineString<double>&) -> std::string_view {
      return "line string";
    },
    [](const mbgl::Polygon<double>&) -> std::string_view { return "polygon"; },
    [](const mbgl::MultiPoint<double>&) -> std::string_view {
      return "multi-point";
    },
    [](const mbgl::MultiLineString<double>&) -> std::string_view {
      return "multi-line string";
    },
    [](const mbgl::MultiPolygon<double>&) -> std::string_view {
      return "multi-polygon";
    },
    [](const mapbox::geometry::geometry_collection<double>&)
      -> std::string_view { return "geometry collection"; }
  );
}

auto geojson_alternative_name(const mbgl::GeoJSON& geojson)
  -> std::string_view {
  return geojson.match(
    [](const mbgl::Geometry<double>&) -> std::string_view {
      return "a bare geometry";
    },
    [](const mbgl::GeoJSONFeature&) -> std::string_view {
      return "a single feature";
    },
    [](const mbgl::FeatureCollection&) -> std::string_view {
      return "a feature collection";
    }
  );
}

// Clustering requires a feature collection whose every feature has point
// geometry; anything else raises a variant access error inside supercluster
// while the index is built. An empty collection is accepted.
auto validate_clustered_geojson(
  const std::string& source_id, const mbgl::GeoJSON& geojson
) -> bool {
  if (!geojson.is<mbgl::FeatureCollection>()) {
    const auto message = "clustered GeoJSON source \"" + source_id +
                         "\" requires a feature collection; the data is " +
                         std::string{geojson_alternative_name(geojson)};
    mln::core::set_thread_error(message.c_str());
    return false;
  }

  const auto& features = geojson.get<mbgl::FeatureCollection>();
  for (std::size_t index = 0; index < features.size(); ++index) {
    const auto& geometry = features.at(index).geometry;
    if (geometry.is<mbgl::Point<double>>()) {
      continue;
    }
    const auto message =
      "clustered GeoJSON source \"" + source_id +
      "\" requires point geometry on every feature; feature " +
      std::to_string(index) + " has " +
      std::string{geojson_geometry_type_name(geometry)} + " geometry";
    mln::core::set_thread_error(message.c_str());
    return false;
  }
  return true;
}

auto has_custom_geometry_source_option(
  const mln_custom_geometry_source_options& options, uint32_t field
) -> bool {
  return (options.fields & field) != 0U;
}

auto validate_custom_geometry_zoom(double zoom, const char* name)
  -> mln_status {
  if (
    !std::isfinite(zoom) || zoom < 0.0 || zoom > 32.0 ||
    std::floor(zoom) != zoom
  ) {
    auto message = std::string{name} + " must be an integer within [0, 32]";
    mln::core::set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto effective_custom_geometry_source_options(
  const mln_custom_geometry_source_options& options
) -> mln_custom_geometry_source_options;

auto validate_custom_geometry_source_options(
  const mln_custom_geometry_source_options* options
) -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_custom_geometry_source_options)) {
    mln::core::set_thread_error(
      "mln_custom_geometry_source_options.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM) |
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM |
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE |
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE |
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER |
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP |
    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_custom_geometry_source_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->fetch_tile == nullptr) {
    mln::core::set_thread_error("fetch_tile must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    has_custom_geometry_source_option(
      *options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
    )
  ) {
    const auto status =
      validate_custom_geometry_zoom(options->min_zoom, "min_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    has_custom_geometry_source_option(
      *options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
    )
  ) {
    const auto status =
      validate_custom_geometry_zoom(options->max_zoom, "max_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  const auto effective = effective_custom_geometry_source_options(*options);
  if (effective.min_zoom > effective.max_zoom) {
    mln::core::set_thread_error(
      "min_zoom must be less than or equal to max_zoom"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!std::isfinite(effective.tolerance) || effective.tolerance < 0.0) {
    mln::core::set_thread_error("tolerance must be finite and non-negative");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.tile_size == 0 || effective.tile_size > 65535U) {
    mln::core::set_thread_error("tile_size must be within [1, 65535]");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.buffer > 65535U) {
    mln::core::set_thread_error("buffer must be at most 65535");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto effective_custom_geometry_source_options(
  const mln_custom_geometry_source_options& options
) -> mln_custom_geometry_source_options {
  auto result = mln::core::custom_geometry_source_options_default();
  result.fields = options.fields;
  result.fetch_tile = options.fetch_tile;
  result.cancel_tile = options.cancel_tile;
  result.user_data = options.user_data;
  result.release_user_data = options.release_user_data;
  if (
    has_custom_geometry_source_option(
      options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
    )
  ) {
    result.min_zoom = options.min_zoom;
  }
  if (
    has_custom_geometry_source_option(
      options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
    )
  ) {
    result.max_zoom = options.max_zoom;
  }
  if (
    has_custom_geometry_source_option(
      options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
    )
  ) {
    result.tolerance = options.tolerance;
  }
  if (
    has_custom_geometry_source_option(
      options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
    )
  ) {
    result.tile_size = options.tile_size;
  }
  if (
    has_custom_geometry_source_option(
      options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
    )
  ) {
    result.buffer = options.buffer;
  }
  if (
    has_custom_geometry_source_option(
      options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
    )
  ) {
    result.clip = options.clip;
  }
  if (
    has_custom_geometry_source_option(
      options, MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
    )
  ) {
    result.wrap = options.wrap;
  }
  return result;
}

auto to_c_canonical_tile_id(const mbgl::CanonicalTileID& tile_id)
  -> mln_canonical_tile_id {
  return mln_canonical_tile_id{.z = tile_id.z, .x = tile_id.x, .y = tile_id.y};
}

auto to_native_tile_function(
  mln_custom_geometry_source_tile_callback callback, void* user_data
) -> mbgl::style::TileFunction {
  if (callback == nullptr) {
    return nullptr;
  }
  return [callback, user_data](const mbgl::CanonicalTileID& tile_id) -> void {
    try {
      callback(user_data, to_c_canonical_tile_id(tile_id));
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("custom geometry source callback threw");
    }
  };
}

auto to_native_custom_geometry_source_options(
  const mln_custom_geometry_source_options& options
) -> mbgl::style::CustomGeometrySource::Options {
  auto result = mbgl::style::CustomGeometrySource::Options{};
  result.fetchTileFunction =
    to_native_tile_function(options.fetch_tile, options.user_data);
  result.cancelTileFunction =
    to_native_tile_function(options.cancel_tile, options.user_data);
  result.zoomRange = mbgl::Range<uint8_t>{
    static_cast<uint8_t>(options.min_zoom),
    static_cast<uint8_t>(options.max_zoom)
  };
  result.tileOptions = mbgl::style::CustomGeometrySource::TileOptions{
    .tolerance = options.tolerance,
    .tileSize = static_cast<uint16_t>(options.tile_size),
    .buffer = static_cast<uint16_t>(options.buffer),
    .clip = options.clip,
    .wrap = options.wrap
  };
  return result;
}

}  // namespace

namespace mln::core {

auto validate_canonical_tile_id(mln_canonical_tile_id tile_id) -> mln_status {
  if (tile_id.z > 32U) {
    mln::core::set_thread_error("tile_id.z must be within [0, 32]");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinate_limit = uint64_t{1} << tile_id.z;
  if (tile_id.x >= coordinate_limit || tile_id.y >= coordinate_limit) {
    mln::core::set_thread_error("tile_id x and y must be within zoom bounds");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_native_canonical_tile_id(mln_canonical_tile_id tile_id)
  -> mbgl::CanonicalTileID {
  return mbgl::CanonicalTileID{
    static_cast<uint8_t>(tile_id.z), tile_id.x, tile_id.y
  };
}

auto validate_source_id(mln_buffer_view source_id) -> mln_status {
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    mln::core::set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_source_can_be_added(
  mbgl::style::Style& style, const std::string& source_id
) -> mln_status {
  if (style.getSource(source_id) != nullptr) {
    mln::core::set_thread_error("source already exists");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto has_style_image_option(
  const mln_style_image_options& options, uint32_t field
) -> bool {
  return (options.fields & field) != 0U;
}

auto validate_style_image_options(const mln_style_image_options* options)
  -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_style_image_options)) {
    mln::core::set_thread_error("mln_style_image_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) |
    MLN_STYLE_IMAGE_OPTION_SDF | MLN_STYLE_IMAGE_OPTION_STRETCH_X |
    MLN_STYLE_IMAGE_OPTION_STRETCH_Y | MLN_STYLE_IMAGE_OPTION_CONTENT |
    MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH |
    MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_style_image_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) &&
    (!std::isfinite(options->pixel_ratio) || options->pixel_ratio <= 0.0F)
  ) {
    mln::core::set_thread_error("pixel_ratio must be finite and positive");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  for (const auto& [field, stretches, count, name] : {
         std::tuple{
           uint32_t{MLN_STYLE_IMAGE_OPTION_STRETCH_X}, options->stretch_x,
           options->stretch_x_count, "stretch_x"
         },
         std::tuple{
           uint32_t{MLN_STYLE_IMAGE_OPTION_STRETCH_Y}, options->stretch_y,
           options->stretch_y_count, "stretch_y"
         },
       }) {
    if (!has_style_image_option(*options, field)) {
      continue;
    }
    if (stretches == nullptr && count != 0) {
      auto message =
        std::string{name} + " must not be null when its count is non-zero";
      mln::core::set_thread_error(message.c_str());
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    for (size_t index = 0; index < count; index += 1) {
      const auto& stretch = stretches[index];
      if (!std::isfinite(stretch.from) || !std::isfinite(stretch.to)) {
        auto message = std::string{name} + " intervals must be finite";
        mln::core::set_thread_error(message.c_str());
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      // MapLibre divides by the summed interval width, so an axis whose
      // intervals are all zero-width would divide by zero.
      if (stretch.from >= stretch.to) {
        auto message =
          std::string{name} + " intervals must have a positive width";
        mln::core::set_thread_error(message.c_str());
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (index != 0 && stretch.from < stretches[index - 1].to) {
        auto message =
          std::string{name} + " intervals must increase and must not overlap";
        mln::core::set_thread_error(message.c_str());
        return MLN_STATUS_INVALID_ARGUMENT;
      }
    }
  }
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_CONTENT)) {
    const auto& content = options->content;
    if (
      !std::isfinite(content.left) || !std::isfinite(content.top) ||
      !std::isfinite(content.right) || !std::isfinite(content.bottom)
    ) {
      mln::core::set_thread_error("content insets must be finite");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (content.left > content.right || content.top > content.bottom) {
      mln::core::set_thread_error("content insets must not run backwards");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  for (const auto& [field, value, name] : {
         std::tuple{
           uint32_t{MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH},
           options->text_fit_width, "text_fit_width"
         },
         std::tuple{
           uint32_t{MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT},
           options->text_fit_height, "text_fit_height"
         },
       }) {
    if (!has_style_image_option(*options, field)) {
      continue;
    }
    switch (value) {
      case MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK:
      case MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY:
      case MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL:
        break;
      default: {
        auto message = std::string{name} + " is invalid";
        mln::core::set_thread_error(message.c_str());
        return MLN_STATUS_INVALID_ARGUMENT;
      }
    }
  }
  return MLN_STATUS_OK;
}

auto effective_style_image_options(const mln_style_image_options* options)
  -> mln_style_image_options {
  auto result = mln::core::style_image_options_default();
  if (options == nullptr) {
    return result;
  }
  result.fields = options->fields;
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO)) {
    result.pixel_ratio = options->pixel_ratio;
  }
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_SDF)) {
    result.sdf = options->sdf;
  }
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_STRETCH_X)) {
    result.stretch_x = options->stretch_x;
    result.stretch_x_count = options->stretch_x_count;
  }
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_STRETCH_Y)) {
    result.stretch_y = options->stretch_y;
    result.stretch_y_count = options->stretch_y_count;
  }
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_CONTENT)) {
    result.content = options->content;
  }
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH)) {
    result.text_fit_width = options->text_fit_width;
  }
  if (
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT)
  ) {
    result.text_fit_height = options->text_fit_height;
  }
  return result;
}

auto to_native_image_stretches(const mln_image_stretch* stretches, size_t count)
  -> mbgl::style::ImageStretches {
  auto native = mbgl::style::ImageStretches{};
  native.reserve(count);
  for (size_t index = 0; index < count; index += 1) {
    native.emplace_back(stretches[index].from, stretches[index].to);
  }
  return native;
}

auto to_native_text_fit(uint32_t value) -> mbgl::style::TextFit {
  switch (value) {
    case MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY:
      return mbgl::style::TextFit::stretchOnly;
    case MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL:
      return mbgl::style::TextFit::proportional;
    default:
      return mbgl::style::TextFit::stretchOrShrink;
  }
}

auto from_native_text_fit(mbgl::style::TextFit value) -> uint32_t {
  switch (value) {
    case mbgl::style::TextFit::stretchOnly:
      return MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY;
    case mbgl::style::TextFit::proportional:
      return MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
    default:
      return MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK;
  }
}

auto required_premultiplied_rgba8_bytes(
  uint32_t width, uint32_t height, uint32_t stride
) -> std::optional<size_t> {
  constexpr auto channels = size_t{4};
  const auto row_bytes = static_cast<size_t>(width) * channels;
  if (height == 0) {
    return std::nullopt;
  }
  if (height == 1) {
    return row_bytes;
  }
  const auto trailing_rows = static_cast<size_t>(height - 1U);
  const auto row_stride = static_cast<size_t>(stride);
  if (
    trailing_rows >
    (std::numeric_limits<size_t>::max() - row_bytes) / row_stride
  ) {
    return std::nullopt;
  }
  return (trailing_rows * row_stride) + row_bytes;
}

auto validate_premultiplied_rgba8_image(
  const mln_premultiplied_rgba8_image* image
) -> mln_status {
  if (image == nullptr) {
    mln::core::set_thread_error("image must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (image->size < sizeof(mln_premultiplied_rgba8_image)) {
    mln::core::set_thread_error(
      "mln_premultiplied_rgba8_image.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (image->width == 0 || image->height == 0) {
    mln::core::set_thread_error("image dimensions must be positive");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto channels = uint32_t{4};
  if (image->width > std::numeric_limits<uint32_t>::max() / channels) {
    mln::core::set_thread_error("image row byte length overflows");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto row_bytes = image->width * channels;
  if (image->stride < row_bytes) {
    mln::core::set_thread_error("image stride must be at least width * 4");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (image->pixels == nullptr) {
    mln::core::set_thread_error("image pixels must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto required = required_premultiplied_rgba8_bytes(
    image->width, image->height, image->stride
  );
  if (!required || image->byte_length < *required) {
    mln::core::set_thread_error("image byte_length is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_native_premultiplied_rgba8_image(
  const mln_premultiplied_rgba8_image& image
) -> mbgl::PremultipliedImage {
  auto result = mbgl::PremultipliedImage{mbgl::Size{image.width, image.height}};
  const auto output_stride = result.stride();
  const auto row_bytes = static_cast<size_t>(image.width) * 4U;
  const auto input = std::span<const uint8_t>{image.pixels, image.byte_length};
  const auto output = std::span<uint8_t>{result.data.get(), result.bytes()};
  for (auto row = uint32_t{0}; row < image.height; ++row) {
    const auto input_offset = static_cast<size_t>(row) * image.stride;
    const auto output_offset = static_cast<size_t>(row) * output_stride;
    std::copy_n(
      input.subspan(input_offset, row_bytes).begin(), row_bytes,
      output.subspan(output_offset, row_bytes).begin()
    );
  }
  return result;
}

auto validate_image_id(mln_buffer_view image_id) -> mln_status {
  if (!validate_string_view(image_id, "image_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (image_id.size == 0) {
    mln::core::set_thread_error("image_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto style_image_info_from_native(const mbgl::style::Image& image)
  -> mln_style_image_info {
  const auto& pixels = image.getImage();
  return mln_style_image_info{
    .size = sizeof(mln_style_image_info),
    .width = pixels.size.width,
    .height = pixels.size.height,
    .stride = static_cast<uint32_t>(pixels.stride()),
    .byte_length = pixels.bytes(),
    .stretch_x_count = image.getStretchX().size(),
    .stretch_y_count = image.getStretchY().size(),
    .content =
      image.getContent().has_value()
        ? mln_image_content{
            .left = image.getContent()->left,
            .top = image.getContent()->top,
            .right = image.getContent()->right,
            .bottom = image.getContent()->bottom
          }
        : mln_image_content{.left = 0, .top = 0, .right = 0, .bottom = 0},
    .text_fit_width =
      image.getTextFitWidth().has_value()
        ? from_native_text_fit(*image.getTextFitWidth())
        : static_cast<uint32_t>(MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK),
    .text_fit_height =
      image.getTextFitHeight().has_value()
        ? from_native_text_fit(*image.getTextFitHeight())
        : static_cast<uint32_t>(MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK),
    .pixel_ratio = image.getPixelRatio(),
    .sdf = image.isSdf(),
    .has_content = image.getContent().has_value(),
    .has_text_fit_width = image.getTextFitWidth().has_value(),
    .has_text_fit_height = image.getTextFitHeight().has_value()
  };
}

auto validate_image_source_coordinates(
  const mln_lat_lng* coordinates, size_t coordinate_count
) -> mln_status {
  if (coordinate_count != 4) {
    mln::core::set_thread_error("image source coordinate_count must be 4");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto status =
    validate_lat_lng_array(coordinates, coordinate_count, false);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  const auto coordinate_span = std::span<const mln_lat_lng>{coordinates, 4};
  const auto first = coordinate_span.front();
  const auto all_same =
    std::ranges::all_of(coordinate_span, [first](mln_lat_lng value) -> bool {
      return value.latitude == first.latitude &&
             value.longitude == first.longitude;
    });
  if (all_same) {
    mln::core::set_thread_error("image source coordinates must not all match");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_native_image_source_coordinates(const mln_lat_lng* coordinates)
  -> std::array<mbgl::LatLng, 4> {
  auto result = std::array<mbgl::LatLng, 4>{};
  const auto coordinate_span = std::span<const mln_lat_lng>{coordinates, 4};
  auto index = size_t{0};
  for (const auto coordinate : coordinate_span) {
    result.at(index) = to_native_lat_lng(coordinate);
    ++index;
  }
  return result;
}

auto from_native_image_source_coordinates(
  const std::array<mbgl::LatLng, 4>& coordinates
) -> std::array<mln_lat_lng, 4> {
  auto result = std::array<mln_lat_lng, 4>{};
  for (auto index = size_t{0}; index < result.size(); ++index) {
    result.at(index) = from_native_lat_lng(coordinates.at(index));
  }
  return result;
}

auto create_style_id_list(
  std::vector<std::string> ids, mln_style_id_list* out_list
) -> mln_status {
  if (out_list == nullptr || *out_list != MLN_HANDLE_NULL) {
    mln::core::set_thread_error(
      "out_list must not be null and *out_list must be the null handle"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto list = std::make_shared<mln::core::StyleIdListObject>();
  list->ids = std::move(ids);
  *out_list = mln::core::handle_table<mln::core::StyleIdListObject>().insert(
    std::move(list)
  );
  return MLN_STATUS_OK;
}

auto create_style_string_list(
  std::vector<std::string> values, mln_style_string_list* out_list
) -> mln_status {
  if (out_list == nullptr || *out_list != MLN_HANDLE_NULL) {
    mln::core::set_thread_error(
      "out_list must not be null and *out_list must be the null handle"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto list = std::make_shared<mln::core::StyleStringListObject>();
  list->values = std::move(values);
  *out_list =
    mln::core::handle_table<mln::core::StyleStringListObject>().insert(
      std::move(list)
    );
  return MLN_STATUS_OK;
}

auto validate_lat_lng(mln_lat_lng coordinate) -> mln_status {
  if (
    !std::isfinite(coordinate.latitude) || coordinate.latitude < -90.0 ||
    coordinate.latitude > 90.0 || !std::isfinite(coordinate.longitude)
  ) {
    mln::core::set_thread_error(
      "latitude must be finite and within [-90, 90], and longitude must be "
      "finite"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln_status {
  const auto southwest_status = validate_lat_lng(bounds.southwest);
  if (southwest_status != MLN_STATUS_OK) {
    return southwest_status;
  }
  const auto northeast_status = validate_lat_lng(bounds.northeast);
  if (northeast_status != MLN_STATUS_OK) {
    return northeast_status;
  }
  if (
    bounds.southwest.latitude > bounds.northeast.latitude ||
    bounds.southwest.longitude > bounds.northeast.longitude
  ) {
    mln::core::set_thread_error(
      "bounds southwest must be less than or equal to northeast"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_lat_lng_array(
  const mln_lat_lng* coordinates, size_t coordinate_count, bool allow_empty
) -> mln_status {
  if (coordinate_count == 0) {
    if (allow_empty) {
      return MLN_STATUS_OK;
    }
    mln::core::set_thread_error("coordinate_count must be greater than 0");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (coordinates == nullptr) {
    mln::core::set_thread_error("coordinates must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto coordinate_span =
    std::span<const mln_lat_lng>{coordinates, coordinate_count};
  for (const auto coordinate : coordinate_span) {
    const auto status = validate_lat_lng(coordinate);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  return MLN_STATUS_OK;
}

auto to_native_lat_lng(mln_lat_lng coordinate) -> mbgl::LatLng {
  return mbgl::LatLng{coordinate.latitude, coordinate.longitude};
}

auto from_native_lat_lng(const mbgl::LatLng& coordinate) -> mln_lat_lng {
  return mln_lat_lng{
    .latitude = coordinate.latitude(), .longitude = coordinate.longitude()
  };
}

auto to_native_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mbgl::LatLngBounds {
  return mbgl::LatLngBounds::hull(
    to_native_lat_lng(bounds.southwest), to_native_lat_lng(bounds.northeast)
  );
}

auto from_native_lat_lng_bounds(const mbgl::LatLngBounds& bounds)
  -> mln_lat_lng_bounds {
  return mln_lat_lng_bounds{
    .southwest =
      mln_lat_lng{.latitude = bounds.south(), .longitude = bounds.west()},
    .northeast =
      mln_lat_lng{.latitude = bounds.north(), .longitude = bounds.east()}
  };
}

auto copy_text(
  const std::string& text, char* out_text, size_t text_capacity,
  size_t* out_text_size, const char* capacity_name
) -> mln_status {
  if (out_text == nullptr && text_capacity > 0) {
    set_thread_error(
      "output buffer must not be null when capacity is non-zero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_text_size == nullptr) {
    set_thread_error("output size pointer must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_text_size = text.size();
  // A null buffer with zero capacity is a size probe, so it succeeds rather
  // than sharing a status with a missing object.
  if (out_text == nullptr) {
    return MLN_STATUS_OK;
  }
  if (text_capacity < text.size()) {
    auto message = std::string{capacity_name} + " is too small";
    set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!text.empty()) {
    std::copy(text.begin(), text.end(), out_text);
  }
  return MLN_STATUS_OK;
}

using DoubleMilliseconds = std::chrono::duration<double, std::milli>;

auto max_native_duration_ms() -> double {
  return std::chrono::duration_cast<DoubleMilliseconds>(mbgl::Duration::max())
    .count();
}

auto duration_from_milliseconds(double milliseconds) -> mbgl::Duration {
  return std::chrono::duration_cast<mbgl::Duration>(
    DoubleMilliseconds{milliseconds}
  );
}

auto milliseconds_from_duration(mbgl::Duration duration) -> double {
  return std::chrono::duration_cast<DoubleMilliseconds>(duration).count();
}

// The accepted bound is exclusive because mbgl::Duration::max() has no exact
// double representation: the nearest double converts back to 2^63 ticks, one
// past the largest representable count. The margin holds for a nanosecond
// duration only, so pin the representation.
static_assert(
  std::is_same_v<mbgl::Duration, std::chrono::nanoseconds>,
  "the accepted duration bound is derived from a nanosecond mbgl::Duration"
);

auto is_native_duration_ms(double milliseconds) -> bool {
  return std::isfinite(milliseconds) && milliseconds >= 0.0 &&
         milliseconds < max_native_duration_ms();
}

}  // namespace mln::core

namespace mln::core {

auto style_tile_source_options_default() noexcept
  -> mln_style_tile_source_options {
  return mln_style_tile_source_options{
    .size = sizeof(mln_style_tile_source_options),
    .fields = 0,
    .min_zoom = 0,
    .max_zoom = mbgl::util::DEFAULT_MAX_ZOOM,
    .attribution = {.data = nullptr, .size = 0},
    .scheme = MLN_STYLE_TILE_SCHEME_XYZ,
    .bounds =
      {.southwest = {.latitude = 0, .longitude = 0},
       .northeast = {.latitude = 0, .longitude = 0}},
    .tile_size = mbgl::util::tileSize_I,
    .vector_encoding = MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
    .raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX
  };
}

auto geojson_source_options_default() noexcept -> mln_geojson_source_options {
  const auto defaults = mbgl::style::GeoJSONOptions{};
  return mln_geojson_source_options{
    .size = sizeof(mln_geojson_source_options),
    .fields = 0,
    .min_zoom = static_cast<double>(defaults.minzoom),
    .max_zoom = static_cast<double>(defaults.maxzoom),
    .tolerance = defaults.tolerance,
    .cluster_max_zoom = static_cast<double>(defaults.clusterMaxZoom),
    .cluster_properties = {},
    .tile_size = defaults.tileSize,
    .buffer = defaults.buffer,
    .cluster_radius = defaults.clusterRadius,
    .cluster_min_points = static_cast<uint32_t>(defaults.clusterMinPoints),
    .line_metrics = defaults.lineMetrics,
    .cluster = defaults.cluster,
    .synchronous_update = defaults.synchronousUpdate
  };
}

auto custom_geometry_source_options_default() noexcept
  -> mln_custom_geometry_source_options {
  return mln_custom_geometry_source_options{
    .size = sizeof(mln_custom_geometry_source_options),
    .fields = 0,
    .fetch_tile = nullptr,
    .cancel_tile = nullptr,
    .user_data = nullptr,
    .min_zoom = 0,
    .max_zoom = 18,
    .tolerance = 0.375,
    .tile_size = mbgl::util::tileSize_I,
    .buffer = 128,
    .clip = false,
    .wrap = false,
    .release_user_data = nullptr
  };
}

auto premultiplied_rgba8_image_default() noexcept
  -> mln_premultiplied_rgba8_image {
  return mln_premultiplied_rgba8_image{
    .size = sizeof(mln_premultiplied_rgba8_image),
    .width = 0,
    .height = 0,
    .stride = 0,
    .pixels = nullptr,
    .byte_length = 0
  };
}

auto style_image_options_default() noexcept -> mln_style_image_options {
  return mln_style_image_options{
    .size = sizeof(mln_style_image_options),
    .fields = 0,
    .stretch_x = nullptr,
    .stretch_x_count = 0,
    .stretch_y = nullptr,
    .stretch_y_count = 0,
    .content = {.left = 0, .top = 0, .right = 0, .bottom = 0},
    .text_fit_width = MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
    .text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
    .pixel_ratio = 1.0F,
    .sdf = false
  };
}

auto style_image_info_default() noexcept -> mln_style_image_info {
  return mln_style_image_info{
    .size = sizeof(mln_style_image_info),
    .width = 0,
    .height = 0,
    .stride = 0,
    .byte_length = 0,
    .stretch_x_count = 0,
    .stretch_y_count = 0,
    .content = {.left = 0, .top = 0, .right = 0, .bottom = 0},
    .text_fit_width = MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
    .text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
    .pixel_ratio = 1.0F,
    .sdf = false,
    .has_content = false,
    .has_text_fit_width = false,
    .has_text_fit_height = false
  };
}

auto style_transition_options_default() noexcept
  -> mln_style_transition_options {
  return mln_style_transition_options{
    .size = sizeof(mln_style_transition_options),
    .fields = 0,
    .duration_ms = 0.0,
    .delay_ms = 0.0,
    .enable_placement_transitions = true
  };
}

StyleOperationResult::StyleOperationResult(
  StyleOperationResult&& other
) noexcept
    : flag(other.flag),
      found(other.found),
      value_u32(other.value_u32),
      value_double(other.value_double),
      source_info(other.source_info),
      image_info(other.image_info),
      transition_options(other.transition_options),
      buffer(std::exchange(other.buffer, MLN_HANDLE_NULL)),
      id_list(std::exchange(other.id_list, MLN_HANDLE_NULL)),
      string_list(std::exchange(other.string_list, MLN_HANDLE_NULL)),
      stretch_x(std::move(other.stretch_x)),
      stretch_y(std::move(other.stretch_y)),
      coordinates(std::move(other.coordinates)) {}

auto StyleOperationResult::operator=(StyleOperationResult&& other) noexcept
  -> StyleOperationResult& {
  if (this == &other) {
    return *this;
  }
  if (buffer != MLN_HANDLE_NULL) {
    buffer_destroy(buffer);
  }
  style_id_list_destroy(id_list);
  style_string_list_destroy(string_list);
  flag = other.flag;
  found = other.found;
  value_u32 = other.value_u32;
  value_double = other.value_double;
  source_info = other.source_info;
  image_info = other.image_info;
  transition_options = other.transition_options;
  buffer = std::exchange(other.buffer, MLN_HANDLE_NULL);
  id_list = std::exchange(other.id_list, MLN_HANDLE_NULL);
  string_list = std::exchange(other.string_list, MLN_HANDLE_NULL);
  stretch_x = std::move(other.stretch_x);
  stretch_y = std::move(other.stretch_y);
  coordinates = std::move(other.coordinates);
  return *this;
}

StyleOperationResult::~StyleOperationResult() {
  if (buffer != MLN_HANDLE_NULL) {
    buffer_destroy(buffer);
  }
  style_id_list_destroy(id_list);
  style_string_list_destroy(string_list);
}

auto validate_geojson_command_options(const mln_geojson_source_options* options)
  -> mln_status {
  return validate_geojson_source_options(options);
}

auto validate_tile_command_options(
  const mln_style_tile_source_options* options, uint32_t kind
) -> mln_status {
  switch (kind) {
    case 0:
      return validate_tile_source_options(
        options, TileSourceOptionKind::Vector
      );
    case 1:
      return validate_tile_source_options(
        options, TileSourceOptionKind::Raster
      );
    case 2:
      return validate_tile_source_options(
        options, TileSourceOptionKind::RasterDEM
      );
    default:
      set_thread_error("tile source kind is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
}

auto validate_custom_geometry_command_options(
  const mln_custom_geometry_source_options* options
) -> mln_status {
  return validate_custom_geometry_source_options(options);
}

auto validate_style_image_command_input(
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) -> mln_status {
  const auto image_status = validate_premultiplied_rgba8_image(image);
  return image_status == MLN_STATUS_OK ? validate_style_image_options(options)
                                       : image_status;
}

auto validate_image_source_command_coordinates(
  const mln_lat_lng* coordinates, size_t coordinate_count
) -> mln_status {
  return validate_image_source_coordinates(coordinates, coordinate_count);
}

auto style_id_list_count(mln_style_id_list list, size_t* out_count)
  -> mln_status {
  if (out_count == nullptr) {
    set_thread_error("out_count must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // A style ID list carries no thread affinity, so another thread may destroy
  // it mid-read; the lock spans the read.
  auto& table = handle_table<StyleIdListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_count = live_list->ids.size();
  return MLN_STATUS_OK;
}

auto style_id_list_get(
  mln_style_id_list list, size_t index, mln_buffer_view* out_id
) -> mln_status {
  if (out_id == nullptr) {
    set_thread_error("out_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& table = handle_table<StyleIdListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (index >= live_list->ids.size()) {
    set_thread_error("index is out of range");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_id = string_view_from_string(live_list->ids.at(index));
  return MLN_STATUS_OK;
}

auto style_id_list_destroy(mln_style_id_list list) -> void {
  static_cast<void>(handle_table<StyleIdListObject>().remove(list));
}

auto style_string_list_count(mln_style_string_list list, size_t* out_count)
  -> mln_status {
  if (out_count == nullptr) {
    set_thread_error("out_count must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& table = handle_table<StyleStringListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_count = live_list->values.size();
  return MLN_STATUS_OK;
}

auto style_string_list_get(
  mln_style_string_list list, size_t index, mln_buffer_view* out_value
) -> mln_status {
  if (out_value == nullptr) {
    set_thread_error("out_value must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& table = handle_table<StyleStringListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (index >= live_list->values.size()) {
    set_thread_error("index is out of range");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_value = string_view_from_string(live_list->values.at(index));
  return MLN_STATUS_OK;
}

auto style_string_list_destroy(mln_style_string_list list) -> void {
  static_cast<void>(handle_table<StyleStringListObject>().remove(list));
}

auto map_add_style_source_json(
  mln_map map, mln_buffer_view source_id, mln_buffer_view source_json
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!validate_bytes(source_json, "style source")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  if (style.getSource(id) != nullptr) {
    set_thread_error("source already exists");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto error = mbgl::style::conversion::Error{};
  auto source =
    mbgl::style::conversion::convertJSON<std::unique_ptr<mbgl::style::Source>>(
      string_from_view(source_json), error, id
    );
  if (!source) {
    set_style_conversion_error("style source", error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  style.addSource(std::move(*source));
  return MLN_STATUS_OK;
}

auto map_remove_style_source(
  mln_map map, mln_buffer_view source_id, bool* out_removed
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_removed == nullptr) {
    set_thread_error("out_removed must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  if (style.getSource(id) == nullptr) {
    *out_removed = false;
    return MLN_STATUS_OK;
  }

  auto removed = style.removeSource(id);
  if (!removed) {
    set_thread_error("source is used by a layer");
    return MLN_STATUS_INVALID_STATE;
  }
  // The detached source is dropped before the release runs, so the style no
  // longer holds the callbacks that read the host's state.
  removed.reset();
  release_custom_geometry_source(*live, id);
  *out_removed = true;
  return MLN_STATUS_OK;
}

auto map_get_style_source_info(
  mln_map map, mln_buffer_view source_id, mln_style_source_info* out_info,
  bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_info == nullptr || out_info->size < sizeof(mln_style_source_info)) {
    set_thread_error("out_info must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_found == nullptr) {
    set_thread_error("out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  *out_found = source != nullptr;
  *out_info = mln_style_source_info{};
  out_info->size = sizeof(mln_style_source_info);
  out_info->type = MLN_STYLE_SOURCE_TYPE_UNKNOWN;
  if (source == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto attribution = source->getAttribution();
  out_info->type = to_c_source_type(source->getType());
  out_info->id_size = source->getID().size();
  out_info->is_volatile = source->isVolatile();
  out_info->has_attribution = attribution.has_value();
  out_info->attribution_size = attribution ? attribution->size() : 0;

  const auto url = source_url(*source);
  if (url) {
    out_info->fields |= MLN_STYLE_SOURCE_INFO_URL;
    out_info->url_size = url->size();
  }

  const auto* tile_source = tile_source_from_source(*source);
  if (tile_source == nullptr) {
    return MLN_STATUS_OK;
  }

  out_info->fields |= MLN_STYLE_SOURCE_INFO_TILE_SIZE;
  out_info->tile_size = tile_source->getTileSize();
  if (const auto* vector_source = source->as<mbgl::style::VectorSource>()) {
    out_info->fields |= MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING;
    out_info->vector_encoding =
      to_c_vector_encoding(vector_source->getEncoding());
  }

  const auto* tileset = inline_tileset(*tile_source);
  if (tileset == nullptr) {
    return MLN_STATUS_OK;
  }

  out_info->fields |= MLN_STYLE_SOURCE_INFO_TILEJSON;
  out_info->tile_count = tileset->tiles.size();
  out_info->min_zoom = tileset->zoomRange.min;
  out_info->max_zoom = tileset->zoomRange.max;
  out_info->scheme = to_c_tile_scheme(tileset->scheme);
  if (tileset->bounds) {
    out_info->fields |= MLN_STYLE_SOURCE_INFO_BOUNDS;
    out_info->bounds = from_native_lat_lng_bounds(*tileset->bounds);
  }
  if (tileset->vectorEncoding) {
    out_info->fields |= MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING;
    out_info->vector_encoding = to_c_vector_encoding(*tileset->vectorEncoding);
  }
  if (tileset->rasterEncoding) {
    out_info->fields |= MLN_STYLE_SOURCE_INFO_RASTER_ENCODING;
    out_info->raster_encoding = to_c_raster_encoding(*tileset->rasterEncoding);
  }
  return MLN_STATUS_OK;
}

auto map_copy_style_source_attribution(
  mln_map map, mln_buffer_view source_id, char* out_attribution,
  size_t attribution_capacity, size_t* out_attribution_size, bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_attribution == nullptr && attribution_capacity > 0) {
    set_thread_error(
      "out_attribution must not be null when capacity is non-zero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_attribution_size == nullptr || out_found == nullptr) {
    set_thread_error("out_attribution_size and out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  *out_found = false;
  *out_attribution_size = 0;
  if (source == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto attribution = source->getAttribution();
  if (!attribution) {
    return MLN_STATUS_OK;
  }
  *out_found = true;
  *out_attribution_size = attribution->size();
  // A null buffer with zero capacity is a size probe, so it reports the length
  // and succeeds rather than sharing a status with a missing source.
  if (out_attribution == nullptr) {
    return MLN_STATUS_OK;
  }
  if (attribution_capacity < attribution->size()) {
    set_thread_error("attribution_capacity is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!attribution->empty()) {
    std::copy(attribution->begin(), attribution->end(), out_attribution);
  }
  return MLN_STATUS_OK;
}

auto map_copy_style_source_url(
  mln_map map, mln_buffer_view source_id, char* out_url, size_t url_capacity,
  size_t* out_url_size, bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_url == nullptr && url_capacity > 0) {
    set_thread_error("out_url must not be null when capacity is non-zero");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_url_size == nullptr || out_found == nullptr) {
    set_thread_error("out_url_size and out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  const auto url = source == nullptr ? std::nullopt : source_url(*source);
  *out_found = url.has_value();
  if (!url) {
    *out_url_size = 0;
    return MLN_STATUS_OK;
  }

  return copy_text(*url, out_url, url_capacity, out_url_size, "url_capacity");
}

auto map_get_style_source_tile_urls(
  mln_map map, mln_buffer_view source_id, mln_style_string_list* out_tile_urls,
  bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    out_tile_urls == nullptr || *out_tile_urls != MLN_HANDLE_NULL ||
    out_found == nullptr
  ) {
    set_thread_error(
      "out_tile_urls must not be null, *out_tile_urls must be the null handle, "
      "and out_found must not be null"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  *out_found = source != nullptr;
  if (source == nullptr) {
    return MLN_STATUS_OK;
  }

  auto tile_urls = std::vector<std::string>{};
  if (const auto* tile_source = tile_source_from_source(*source)) {
    if (const auto* tileset = inline_tileset(*tile_source)) {
      tile_urls = tileset->tiles;
    }
  }
  return create_style_string_list(std::move(tile_urls), out_tile_urls);
}

auto map_list_style_source_ids(mln_map map, mln_style_id_list* out_source_ids)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  auto ids = std::vector<std::string>{};
  for (const auto* source : map_native(live)->getStyle().getSources()) {
    ids.push_back(source->getID());
  }
  return create_style_id_list(std::move(ids), out_source_ids);
}

auto map_add_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_geojson_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  if (!validate_string_view(url, "url")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto options_status = validate_geojson_source_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto native_options =
    to_native_geojson_source_options(effective_geojson_source_options(options));
  if (!native_options) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto source = std::make_unique<mbgl::style::GeoJSONSource>(
    id, std::move(*native_options)
  );
  source->setURL(string_from_view(url));
  style.addSource(std::move(source));
  return MLN_STATUS_OK;
}

auto map_add_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_buffer_view data,
  const mln_geojson_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }

  auto geojson = to_native_geojson(data);
  if (!geojson) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto options_status = validate_geojson_source_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_geojson_source_options(options);
  if (effective.cluster && !validate_clustered_geojson(id, *geojson)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto native_options = to_native_geojson_source_options(effective);
  if (!native_options) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto source = std::make_unique<mbgl::style::GeoJSONSource>(
    id, std::move(*native_options)
  );
  source->setGeoJSON(*geojson);
  style.addSource(std::move(source));
  return MLN_STATUS_OK;
}

auto map_set_geojson_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  if (!validate_string_view(url, "url")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* geojson_source = source->as<mbgl::style::GeoJSONSource>();
  if (geojson_source == nullptr) {
    set_thread_error("source is not a GeoJSON source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  geojson_source->setURL(string_from_view(url));
  return MLN_STATUS_OK;
}

auto map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_buffer_view data
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }

  auto geojson = to_native_geojson(data);
  if (!geojson) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* geojson_source = source->as<mbgl::style::GeoJSONSource>();
  if (geojson_source == nullptr) {
    set_thread_error("source is not a GeoJSON source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    geojson_source->getOptions().cluster &&
    !validate_clustered_geojson(string_from_view(source_id), *geojson)
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  geojson_source->setGeoJSON(*geojson);
  return MLN_STATUS_OK;
}

auto map_add_vector_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  if (!validate_string_view(url, "url")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto options_status =
    validate_tile_source_options(options, TileSourceOptionKind::Vector);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  auto min_zoom = std::optional<float>{};
  if (
    has_tile_source_option(effective, MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM)
  ) {
    min_zoom = static_cast<float>(effective.min_zoom);
  }
  auto max_zoom = std::optional<float>{};
  if (
    has_tile_source_option(effective, MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM)
  ) {
    max_zoom = static_cast<float>(effective.max_zoom);
  }

  if (
    has_tile_source_option(
      effective, MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
    )
  ) {
    style.addSource(
      std::make_unique<mbgl::style::VectorSource>(
        id, string_from_view(url), max_zoom, min_zoom,
        to_native_vector_encoding(effective.vector_encoding)
      )
    );
  } else {
    style.addSource(
      std::make_unique<mbgl::style::VectorSource>(
        id, string_from_view(url), max_zoom, min_zoom
      )
    );
  }
  return MLN_STATUS_OK;
}

auto map_add_vector_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto tiles_status = validate_tile_urls(tiles, tile_count);
  if (tiles_status != MLN_STATUS_OK) {
    return tiles_status;
  }
  const auto options_status =
    validate_tile_source_options(options, TileSourceOptionKind::Vector);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  auto tileset = to_native_tileset(tiles, tile_count, effective, true);
  if (!tileset) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  style.addSource(
    std::make_unique<mbgl::style::VectorSource>(
      id, *tileset, std::nullopt, std::nullopt,
      to_native_vector_encoding(effective.vector_encoding)
    )
  );
  return MLN_STATUS_OK;
}

auto map_add_raster_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  if (!validate_string_view(url, "url")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto options_status =
    validate_tile_source_options(options, TileSourceOptionKind::Raster);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  style.addSource(
    std::make_unique<mbgl::style::RasterSource>(
      id, string_from_view(url), static_cast<uint16_t>(effective.tile_size)
    )
  );
  return MLN_STATUS_OK;
}

auto map_add_raster_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto tiles_status = validate_tile_urls(tiles, tile_count);
  if (tiles_status != MLN_STATUS_OK) {
    return tiles_status;
  }
  const auto options_status =
    validate_tile_source_options(options, TileSourceOptionKind::Raster);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  auto tileset = to_native_tileset(tiles, tile_count, effective, false);
  if (!tileset) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  style.addSource(
    std::make_unique<mbgl::style::RasterSource>(
      id, *tileset, static_cast<uint16_t>(effective.tile_size)
    )
  );
  return MLN_STATUS_OK;
}

auto map_add_raster_dem_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url,
  const mln_style_tile_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  if (!validate_string_view(url, "url")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto options_status =
    validate_tile_source_options(options, TileSourceOptionKind::RasterDEM);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  auto source_options = std::optional<mbgl::style::SourceOptions>{};
  if (
    has_tile_source_option(
      effective, MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
    )
  ) {
    source_options = mbgl::style::SourceOptions{
      .rasterEncoding = to_native_raster_encoding(effective.raster_encoding)
    };
  }
  style.addSource(
    std::make_unique<mbgl::style::RasterDEMSource>(
      id, string_from_view(url), static_cast<uint16_t>(effective.tile_size),
      source_options
    )
  );
  return MLN_STATUS_OK;
}

auto map_add_raster_dem_source_tiles(
  mln_map map, mln_buffer_view source_id, const mln_buffer_view* tiles,
  size_t tile_count, const mln_style_tile_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto tiles_status = validate_tile_urls(tiles, tile_count);
  if (tiles_status != MLN_STATUS_OK) {
    return tiles_status;
  }
  const auto options_status =
    validate_tile_source_options(options, TileSourceOptionKind::RasterDEM);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  auto tileset = to_native_tileset(tiles, tile_count, effective, false);
  if (!tileset) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    has_tile_source_option(
      effective, MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
    )
  ) {
    tileset->rasterEncoding =
      to_native_raster_encoding(effective.raster_encoding);
  }
  style.addSource(
    std::make_unique<mbgl::style::RasterDEMSource>(
      id, *tileset, static_cast<uint16_t>(effective.tile_size)
    )
  );
  return MLN_STATUS_OK;
}

auto map_add_custom_geometry_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto options_status = validate_custom_geometry_source_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_custom_geometry_source_options(*options);
  // Tracked before the style takes the source, so the two cannot disagree. A
  // throw while tracking leaves no source in the style, and a style that
  // rejects the source untracks it again; either way the add failed and the
  // caller still owns user_data. Tracking afterwards would leave a live source
  // whose release never runs.
  track_custom_geometry_source(
    *live, id, effective.release_user_data, effective.user_data
  );
  try {
    style.addSource(
      std::make_unique<mbgl::style::CustomGeometrySource>(
        id, to_native_custom_geometry_source_options(effective)
      )
    );
  } catch (...) {
    // Mirrors add()'s own early return: a source with no release callback was
    // never tracked, so there is nothing to untrack and no entry of another
    // source's to erase.
    if (effective.release_user_data != nullptr) {
      untrack_custom_geometry_source(*live, id);
    }
    throw;
  }
  return MLN_STATUS_OK;
}

auto map_set_custom_geometry_source_tile_data(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }
  auto geojson = to_native_geojson(data);
  if (!geojson) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* custom_source = source->as<mbgl::style::CustomGeometrySource>();
  if (custom_source == nullptr) {
    set_thread_error("source is not a custom geometry source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  custom_source->setTileData(to_native_canonical_tile_id(tile_id), *geojson);
  return MLN_STATUS_OK;
}

auto map_invalidate_custom_geometry_source_tile(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* custom_source = source->as<mbgl::style::CustomGeometrySource>();
  if (custom_source == nullptr) {
    set_thread_error("source is not a custom geometry source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  custom_source->invalidateTile(to_native_canonical_tile_id(tile_id));
  return MLN_STATUS_OK;
}

auto map_invalidate_custom_geometry_source_region(
  mln_map map, mln_buffer_view source_id, mln_lat_lng_bounds bounds
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto bounds_status = validate_lat_lng_bounds(bounds);
  if (bounds_status != MLN_STATUS_OK) {
    return bounds_status;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* custom_source = source->as<mbgl::style::CustomGeometrySource>();
  if (custom_source == nullptr) {
    set_thread_error("source is not a custom geometry source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  custom_source->invalidateRegion(to_native_lat_lng_bounds(bounds));
  return MLN_STATUS_OK;
}

auto map_set_style_image(
  mln_map map, mln_buffer_view image_id,
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto image_id_status = validate_image_id(image_id);
  if (image_id_status != MLN_STATUS_OK) {
    return image_id_status;
  }
  const auto image_status = validate_premultiplied_rgba8_image(image);
  if (image_status != MLN_STATUS_OK) {
    return image_status;
  }
  const auto options_status = validate_style_image_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  const auto effective = effective_style_image_options(options);
  auto content = std::optional<mbgl::style::ImageContent>{};
  if (
    options != nullptr &&
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_CONTENT)
  ) {
    content = mbgl::style::ImageContent{
      .left = effective.content.left,
      .top = effective.content.top,
      .right = effective.content.right,
      .bottom = effective.content.bottom
    };
  }
  auto text_fit_width = std::optional<mbgl::style::TextFit>{};
  if (
    options != nullptr &&
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH)
  ) {
    text_fit_width = to_native_text_fit(effective.text_fit_width);
  }
  auto text_fit_height = std::optional<mbgl::style::TextFit>{};
  if (
    options != nullptr &&
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT)
  ) {
    text_fit_height = to_native_text_fit(effective.text_fit_height);
  }

  auto style_image = std::make_unique<mbgl::style::Image>(
    string_from_view(image_id), to_native_premultiplied_rgba8_image(*image),
    effective.pixel_ratio, effective.sdf,
    to_native_image_stretches(effective.stretch_x, effective.stretch_x_count),
    to_native_image_stretches(effective.stretch_y, effective.stretch_y_count),
    content, text_fit_width, text_fit_height
  );
  map_native(live)->getStyle().addImage(std::move(style_image));
  return MLN_STATUS_OK;
}

auto map_remove_style_image(
  mln_map map, mln_buffer_view image_id, bool* out_removed
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto image_id_status = validate_image_id(image_id);
  if (image_id_status != MLN_STATUS_OK) {
    return image_id_status;
  }
  if (out_removed == nullptr) {
    set_thread_error("out_removed must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(image_id);
  *out_removed = style.getImage(id).has_value();
  if (*out_removed) {
    style.removeImage(id);
  }
  return MLN_STATUS_OK;
}

auto map_get_style_image_info(
  mln_map map, mln_buffer_view image_id, mln_style_image_info* out_info,
  bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto image_id_status = validate_image_id(image_id);
  if (image_id_status != MLN_STATUS_OK) {
    return image_id_status;
  }
  if (out_info == nullptr || out_info->size < sizeof(mln_style_image_info)) {
    set_thread_error("out_info must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_found == nullptr) {
    set_thread_error("out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto image =
    map_native(live)->getStyle().getImage(string_from_view(image_id));
  *out_found = image.has_value();
  *out_info =
    image ? style_image_info_from_native(*image) : style_image_info_default();
  return MLN_STATUS_OK;
}

auto map_copy_style_image_stretches(
  mln_map map, mln_buffer_view image_id, mln_image_stretch* out_stretch_x,
  size_t stretch_x_capacity, size_t* out_stretch_x_count,
  mln_image_stretch* out_stretch_y, size_t stretch_y_capacity,
  size_t* out_stretch_y_count, bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto image_id_status = validate_image_id(image_id);
  if (image_id_status != MLN_STATUS_OK) {
    return image_id_status;
  }
  if (
    (out_stretch_x == nullptr && stretch_x_capacity > 0) ||
    (out_stretch_y == nullptr && stretch_y_capacity > 0)
  ) {
    set_thread_error(
      "stretch output arrays must not be null when their capacity is non-zero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    out_stretch_x_count == nullptr || out_stretch_y_count == nullptr ||
    out_found == nullptr
  ) {
    set_thread_error("stretch output counts and out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto image =
    map_native(live)->getStyle().getImage(string_from_view(image_id));
  *out_found = image.has_value();
  *out_stretch_x_count = 0;
  *out_stretch_y_count = 0;
  if (!image) {
    return MLN_STATUS_OK;
  }

  const auto& stretch_x = image->getStretchX();
  const auto& stretch_y = image->getStretchY();
  *out_stretch_x_count = stretch_x.size();
  *out_stretch_y_count = stretch_y.size();

  // A null array with zero capacity is a size probe, so it reports the counts
  // and succeeds rather than sharing a status with a missing image.
  for (const auto& [out, capacity, stretches, name] : {
         std::tuple{
           out_stretch_x, stretch_x_capacity, &stretch_x, "stretch_x_capacity"
         },
         std::tuple{
           out_stretch_y, stretch_y_capacity, &stretch_y, "stretch_y_capacity"
         },
       }) {
    if (out == nullptr) {
      continue;
    }
    if (capacity < stretches->size()) {
      auto message = std::string{name} + " is too small";
      set_thread_error(message.c_str());
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  for (const auto& [out, stretches] : {
         std::pair{out_stretch_x, &stretch_x},
         std::pair{out_stretch_y, &stretch_y},
       }) {
    if (out == nullptr) {
      continue;
    }
    for (size_t index = 0; index < stretches->size(); index += 1) {
      out[index] = mln_image_stretch{
        .from = (*stretches)[index].first, .to = (*stretches)[index].second
      };
    }
  }
  return MLN_STATUS_OK;
}

auto map_copy_style_image_premultiplied_rgba8(
  mln_map map, mln_buffer_view image_id, uint8_t* out_pixels,
  size_t pixel_capacity, size_t* out_byte_length, bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto image_id_status = validate_image_id(image_id);
  if (image_id_status != MLN_STATUS_OK) {
    return image_id_status;
  }
  if (out_pixels == nullptr && pixel_capacity > 0) {
    set_thread_error("out_pixels must not be null when capacity is non-zero");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_byte_length == nullptr || out_found == nullptr) {
    set_thread_error("out_byte_length and out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto image =
    map_native(live)->getStyle().getImage(string_from_view(image_id));
  *out_found = image.has_value();
  *out_byte_length = 0;
  if (!image) {
    return MLN_STATUS_OK;
  }

  const auto& pixels = image->getImage();
  *out_byte_length = pixels.bytes();
  // A null buffer with zero capacity is a size probe, so it reports the length
  // and succeeds rather than sharing a status with a missing image.
  if (out_pixels == nullptr) {
    return MLN_STATUS_OK;
  }
  if (pixel_capacity < pixels.bytes()) {
    set_thread_error("pixel_capacity is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (pixels.bytes() > 0) {
    std::copy_n(pixels.data.get(), pixels.bytes(), out_pixels);
  }
  return MLN_STATUS_OK;
}

auto map_add_image_source_url(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_buffer_view url
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto coordinate_status =
    validate_image_source_coordinates(coordinates, coordinate_count);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }
  if (!validate_string_view(url, "url")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto source = std::make_unique<mbgl::style::ImageSource>(
    id, to_native_image_source_coordinates(coordinates)
  );
  source->setURL(string_from_view(url));
  style.addSource(std::move(source));
  return MLN_STATUS_OK;
}

auto map_add_image_source_image(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto coordinate_status =
    validate_image_source_coordinates(coordinates, coordinate_count);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }
  const auto image_status = validate_premultiplied_rgba8_image(image);
  if (image_status != MLN_STATUS_OK) {
    return image_status;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto native_image = to_native_premultiplied_rgba8_image(*image);
  style.addSource(
    std::make_unique<mbgl::style::ImageSource>(
      id, to_native_image_source_coordinates(coordinates)
    )
  );
  auto* added_source = style.getSource(id);
  auto* image_source = added_source == nullptr
                         ? nullptr
                         : added_source->as<mbgl::style::ImageSource>();
  if (image_source == nullptr) {
    set_thread_error("added source is not an image source");
    return MLN_STATUS_NATIVE_ERROR;
  }
  image_source->setImage(std::move(native_image));
  return MLN_STATUS_OK;
}

auto map_set_image_source_url(
  mln_map map, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  if (!validate_string_view(url, "url")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* image_source = source->as<mbgl::style::ImageSource>();
  if (image_source == nullptr) {
    set_thread_error("source is not an image source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  image_source->setURL(string_from_view(url));
  return MLN_STATUS_OK;
}

auto map_set_image_source_image(
  mln_map map, mln_buffer_view source_id,
  const mln_premultiplied_rgba8_image* image
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto image_status = validate_premultiplied_rgba8_image(image);
  if (image_status != MLN_STATUS_OK) {
    return image_status;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* image_source = source->as<mbgl::style::ImageSource>();
  if (image_source == nullptr) {
    set_thread_error("source is not an image source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  image_source->setImage(to_native_premultiplied_rgba8_image(*image));
  return MLN_STATUS_OK;
}

auto map_set_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  const auto coordinate_status =
    validate_image_source_coordinates(coordinates, coordinate_count);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* image_source = source->as<mbgl::style::ImageSource>();
  if (image_source == nullptr) {
    set_thread_error("source is not an image source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  image_source->setCoordinates(to_native_image_source_coordinates(coordinates));
  return MLN_STATUS_OK;
}

auto map_get_image_source_coordinates(
  mln_map map, mln_buffer_view source_id, mln_lat_lng* out_coordinates,
  size_t coordinate_capacity, size_t* out_coordinate_count, bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto source_id_status = validate_source_id(source_id);
  if (source_id_status != MLN_STATUS_OK) {
    return source_id_status;
  }
  if (out_coordinates == nullptr && coordinate_capacity > 0) {
    set_thread_error(
      "out_coordinates must not be null when capacity is non-zero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_coordinate_count == nullptr || out_found == nullptr) {
    set_thread_error("out_coordinate_count and out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* source =
    map_native(live)->getStyle().getSource(string_from_view(source_id));
  *out_found = source != nullptr;
  *out_coordinate_count = 0;
  if (source == nullptr) {
    return MLN_STATUS_OK;
  }
  auto* image_source = source->as<mbgl::style::ImageSource>();
  if (image_source == nullptr) {
    set_thread_error("source is not an image source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto image_source_coordinate_count = size_t{4};
  *out_coordinate_count = image_source_coordinate_count;
  if (out_coordinates == nullptr) {
    return MLN_STATUS_OK;
  }
  if (coordinate_capacity < image_source_coordinate_count) {
    set_thread_error("coordinate_capacity is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinates =
    from_native_image_source_coordinates(image_source->getCoordinates());
  auto output = std::span<mln_lat_lng>{out_coordinates, coordinate_capacity};
  std::ranges::copy(coordinates, output.begin());
  return MLN_STATUS_OK;
}

auto map_add_hillshade_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    !validate_string_view(layer_id, "layer_id") ||
    !validate_string_view(source_id, "source_id") ||
    !validate_string_view(before_layer_id, "before_layer_id")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0 || source_id.size == 0) {
    set_thread_error("layer_id and source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto layer = string_from_view(layer_id);
  const auto source = string_from_view(source_id);
  if (style.getLayer(layer) != nullptr) {
    set_thread_error("layer already exists");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* source_ptr = style.getSource(source);
  if (source_ptr == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!source_ptr->is<mbgl::style::RasterDEMSource>()) {
    set_thread_error("source is not a raster DEM source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (style.getLayer(*before) == nullptr) {
      set_thread_error("before_layer_id does not exist");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  style.addLayer(
    std::make_unique<mbgl::style::HillshadeLayer>(layer, source), before
  );
  return MLN_STATUS_OK;
}

auto map_add_color_relief_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    !validate_string_view(layer_id, "layer_id") ||
    !validate_string_view(source_id, "source_id") ||
    !validate_string_view(before_layer_id, "before_layer_id")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0 || source_id.size == 0) {
    set_thread_error("layer_id and source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto layer = string_from_view(layer_id);
  const auto source = string_from_view(source_id);
  if (style.getLayer(layer) != nullptr) {
    set_thread_error("layer already exists");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* source_ptr = style.getSource(source);
  if (source_ptr == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!source_ptr->is<mbgl::style::RasterDEMSource>()) {
    set_thread_error("source is not a raster DEM source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (style.getLayer(*before) == nullptr) {
      set_thread_error("before_layer_id does not exist");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  style.addLayer(
    std::make_unique<mbgl::style::ColorReliefLayer>(layer, source), before
  );
  return MLN_STATUS_OK;
}

auto map_add_location_indicator_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    !validate_string_view(layer_id, "layer_id") ||
    !validate_string_view(before_layer_id, "before_layer_id")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto layer = string_from_view(layer_id);
  if (style.getLayer(layer) != nullptr) {
    set_thread_error("layer already exists");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (style.getLayer(*before) == nullptr) {
      set_thread_error("before_layer_id does not exist");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  style.addLayer(
    std::make_unique<mbgl::style::LocationIndicatorLayer>(layer), before
  );
  return MLN_STATUS_OK;
}

auto validate_location_indicator_layer(MapObject* map, mln_buffer_view layer_id)
  -> mln_status {
  if (!validate_string_view(layer_id, "layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto* layer =
    map_native(map)->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (std::strcmp(layer->getTypeInfo()->type, "location-indicator") != 0) {
    set_thread_error("layer is not a location indicator layer");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_float64_to_float32(double value, const char* name) -> mln_status {
  if (
    !std::isfinite(value) || value < -std::numeric_limits<float>::max() ||
    value > std::numeric_limits<float>::max()
  ) {
    const auto message = std::string{name} + " must fit in finite float32";
    set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto map_set_location_indicator_location(
  mln_map map, mln_buffer_view layer_id, mln_lat_lng coordinate, double altitude
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto layer_status = validate_location_indicator_layer(live, layer_id);
  if (layer_status != MLN_STATUS_OK) {
    return layer_status;
  }
  const auto coordinate_status = validate_lat_lng(coordinate);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }
  if (!std::isfinite(altitude)) {
    set_thread_error("altitude must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // The style property is [latitude, longitude, altitude]; the renderer reads
  // it back as LatLng{values[0], values[1]}.
  const auto location = serialize_json_value(
    mbgl::Value{mapbox::base::ValueArray{
      mbgl::Value{coordinate.latitude}, mbgl::Value{coordinate.longitude},
      mbgl::Value{altitude}
    }}
  );
  return map_set_layer_property(
    map, layer_id, string_view_from_literal("location"),
    buffer_view_from_string(location)
  );
}

auto map_set_location_indicator_bearing(
  mln_map map, mln_buffer_view layer_id, double bearing
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto layer_status = validate_location_indicator_layer(live, layer_id);
  if (layer_status != MLN_STATUS_OK) {
    return layer_status;
  }
  const auto bearing_status = validate_float64_to_float32(bearing, "bearing");
  if (bearing_status != MLN_STATUS_OK) {
    return bearing_status;
  }
  const auto value = serialize_json_value(mbgl::Value{bearing});
  return map_set_layer_property(
    map, layer_id, string_view_from_literal("bearing"),
    buffer_view_from_string(value)
  );
}

auto map_set_location_indicator_accuracy_radius(
  mln_map map, mln_buffer_view layer_id, double radius
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto layer_status = validate_location_indicator_layer(live, layer_id);
  if (layer_status != MLN_STATUS_OK) {
    return layer_status;
  }
  const auto radius_status = validate_float64_to_float32(radius, "radius");
  if (radius_status != MLN_STATUS_OK) {
    return radius_status;
  }
  if (radius < 0.0) {
    set_thread_error("radius must be non-negative");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto value = serialize_json_value(mbgl::Value{radius});
  return map_set_layer_property(
    map, layer_id, string_view_from_literal("accuracy-radius"),
    buffer_view_from_string(value)
  );
}

auto location_indicator_image_property(uint32_t image_kind)
  -> std::optional<mln_buffer_view> {
  switch (image_kind) {
    case MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP:
      return string_view_from_literal("top-image");
    case MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING:
      return string_view_from_literal("bearing-image");
    case MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW:
      return string_view_from_literal("shadow-image");
    default:
      return std::nullopt;
  }
}

auto map_set_location_indicator_image_name(
  mln_map map, mln_buffer_view layer_id, uint32_t image_kind,
  mln_buffer_view image_id
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto layer_status = validate_location_indicator_layer(live, layer_id);
  if (layer_status != MLN_STATUS_OK) {
    return layer_status;
  }
  if (!validate_string_view(image_id, "image_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (image_id.size == 0) {
    set_thread_error("image_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto property = location_indicator_image_property(image_kind);
  if (!property) {
    set_thread_error("image_kind is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto value =
    serialize_json_value(mbgl::Value{string_from_view(image_id)});
  return map_set_layer_property(
    map, layer_id, *property, buffer_view_from_string(value)
  );
}

auto map_add_style_layer_json(
  mln_map map, mln_buffer_view layer_json, mln_buffer_view before_layer_id
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(before_layer_id, "before_layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!validate_bytes(layer_json, "style layer")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (style.getLayer(*before) == nullptr) {
      set_thread_error("before_layer_id does not exist");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }

  auto error = mbgl::style::conversion::Error{};
  auto layer =
    mbgl::style::conversion::convertJSON<std::unique_ptr<mbgl::style::Layer>>(
      string_from_view(layer_json), error
    );
  if (!layer) {
    set_style_conversion_error("style layer", error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto id = (*layer)->getID();
  if (style.getLayer(id) != nullptr) {
    set_thread_error("layer already exists");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (*layer)->getTypeInfo()->source ==
      mbgl::style::LayerTypeInfo::Source::Required &&
    style.getSource((*layer)->getSourceID()) == nullptr
  ) {
    set_thread_error("layer source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  style.addLayer(std::move(*layer), before);
  return MLN_STATUS_OK;
}

auto map_remove_style_layer(
  mln_map map, mln_buffer_view layer_id, bool* out_removed
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(layer_id, "layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_removed == nullptr) {
    set_thread_error("out_removed must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto removed =
    map_native(live)->getStyle().removeLayer(string_from_view(layer_id));
  *out_removed = removed != nullptr;
  return MLN_STATUS_OK;
}

auto map_get_style_layer_type(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view* out_layer_type,
  bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(layer_id, "layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_layer_type == nullptr || out_found == nullptr) {
    set_thread_error("out_layer_type and out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto* layer =
    map_native(live)->getStyle().getLayer(string_from_view(layer_id));
  *out_found = layer != nullptr;
  *out_layer_type = {};
  if (layer != nullptr) {
    *out_layer_type = string_view_from_literal(layer->getTypeInfo()->type);
  }
  return MLN_STATUS_OK;
}

auto map_list_style_layer_ids(mln_map map, mln_style_id_list* out_layer_ids)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  auto ids = std::vector<std::string>{};
  for (const auto* layer : map_native(live)->getStyle().getLayers()) {
    ids.push_back(layer->getID());
  }
  return create_style_id_list(std::move(ids), out_layer_ids);
}

auto map_move_style_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    !validate_string_view(layer_id, "layer_id") ||
    !validate_string_view(before_layer_id, "before_layer_id")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = map_native(live)->getStyle();
  const auto id = string_from_view(layer_id);
  if (style.getLayer(id) == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (*before == id) {
      return MLN_STATUS_OK;
    }
    if (style.getLayer(*before) == nullptr) {
      set_thread_error("before_layer_id does not exist");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }

  auto layer = style.removeLayer(id);
  style.addLayer(std::move(layer), before);
  return MLN_STATUS_OK;
}

auto map_get_style_layer_json(
  mln_map map, mln_buffer_view layer_id, mln_buffer* out_layer, bool* out_found
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(layer_id, "layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    out_layer == nullptr || *out_layer != MLN_HANDLE_NULL ||
    out_found == nullptr
  ) {
    set_thread_error(
      "out_layer must not be null, *out_layer must be the null handle, and "
      "out_found must not be null"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto* layer =
    map_native(live)->getStyle().getLayer(string_from_view(layer_id));
  *out_found = layer != nullptr;
  if (layer == nullptr) {
    return MLN_STATUS_OK;
  }
  return create_buffer(serialize_json_value(layer->serialize()), out_layer);
}

auto map_set_style_light_json(mln_map map, mln_buffer_view light_json)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_bytes(light_json, "style light")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto error = mbgl::style::conversion::Error{};
  auto light = mbgl::style::conversion::convertJSON<mbgl::style::Light>(
    string_from_view(light_json), error
  );
  if (!light) {
    set_style_conversion_error("style light", error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  map_native(live)->getStyle().setLight(
    std::make_unique<mbgl::style::Light>(*light)
  );
  return MLN_STATUS_OK;
}

auto map_set_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer_view value
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(property_name, "property_name")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (property_name.size == 0) {
    set_thread_error("property_name must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto document = mbgl::JSDocument{};
  if (!parse_json_document(value, "style light property", document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* light = map_native(live)->getStyle().getLight();
  if (light == nullptr) {
    set_thread_error("style light does not exist");
    return MLN_STATUS_INVALID_STATE;
  }

  auto error = light->setProperty(
    string_from_view(property_name),
    mbgl::style::conversion::Convertible{
      static_cast<const mbgl::JSValue*>(&document)
    }
  );
  if (error) {
    set_style_conversion_error("style light property", *error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto map_get_style_light_property(
  mln_map map, mln_buffer_view property_name, mln_buffer* out_value
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(property_name, "property_name")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (property_name.size == 0) {
    set_thread_error("property_name must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_value == nullptr || *out_value != MLN_HANDLE_NULL) {
    set_thread_error(
      "out_value must not be null and *out_value must be the null handle"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* light = map_native(live)->getStyle().getLight();
  if (light == nullptr) {
    set_thread_error("style light does not exist");
    return MLN_STATUS_INVALID_STATE;
  }

  const auto property = light->getProperty(string_from_view(property_name));
  if (property.getKind() == mbgl::style::StyleProperty::Kind::Undefined) {
    return MLN_STATUS_OK;
  }
  return create_buffer(serialize_json_value(property.getValue()), out_value);
}

auto map_set_style_transition_options(
  mln_map map, const mln_style_transition_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    options == nullptr || options->size < sizeof(mln_style_transition_options)
  ) {
    set_thread_error("options must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_STYLE_TRANSITION_OPTION_DURATION) |
    MLN_STYLE_TRANSITION_OPTION_DELAY |
    MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  if ((options->fields & ~known_fields) != 0U) {
    set_thread_error(
      "mln_style_transition_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // The native default is already on, so an omitted field leaves it alone.
  auto native = mbgl::style::TransitionOptions{};
  if (
    (options->fields &
     MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) != 0U
  ) {
    native.enablePlacementTransitions = options->enable_placement_transitions;
  }
  if ((options->fields & MLN_STYLE_TRANSITION_OPTION_DURATION) != 0U) {
    if (!is_native_duration_ms(options->duration_ms)) {
      set_thread_error(
        "transition duration_ms must fit the native duration range"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    native.duration = duration_from_milliseconds(options->duration_ms);
  }
  if ((options->fields & MLN_STYLE_TRANSITION_OPTION_DELAY) != 0U) {
    if (!is_native_duration_ms(options->delay_ms)) {
      set_thread_error(
        "transition delay_ms must fit the native duration range"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    native.delay = duration_from_milliseconds(options->delay_ms);
  }

  map_native(live)->getStyle().setTransitionOptions(native);
  return MLN_STATUS_OK;
}

auto map_get_style_transition_options(
  mln_map map, mln_style_transition_options* out_options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_options == nullptr ||
    out_options->size < sizeof(mln_style_transition_options)
  ) {
    set_thread_error("out_options must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto native = map_native(live)->getStyle().getTransitionOptions();
  auto result = style_transition_options_default();
  // MapLibre Native always holds this one, so it always reports as present.
  result.fields |= MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  result.enable_placement_transitions = native.enablePlacementTransitions;
  if (native.duration) {
    result.fields |= MLN_STYLE_TRANSITION_OPTION_DURATION;
    result.duration_ms = milliseconds_from_duration(*native.duration);
  }
  if (native.delay) {
    result.fields |= MLN_STYLE_TRANSITION_OPTION_DELAY;
    result.delay_ms = milliseconds_from_duration(*native.delay);
  }
  *out_options = result;
  return MLN_STATUS_OK;
}

auto map_set_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    !validate_string_view(layer_id, "layer_id") ||
    !validate_string_view(property_name, "property_name")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0 || property_name.size == 0) {
    set_thread_error("layer_id and property_name must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto document = mbgl::JSDocument{};
  if (!parse_json_document(value, "layer property", document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* layer =
    map_native(live)->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto error = layer->setProperty(
    string_from_view(property_name),
    mbgl::style::conversion::Convertible{
      static_cast<const mbgl::JSValue*>(&document)
    }
  );
  if (error) {
    set_style_conversion_error("layer property", *error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

auto map_get_layer_property(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer* out_value
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    !validate_string_view(layer_id, "layer_id") ||
    !validate_string_view(property_name, "property_name")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0 || property_name.size == 0) {
    set_thread_error("layer_id and property_name must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_value == nullptr || *out_value != MLN_HANDLE_NULL) {
    set_thread_error(
      "out_value must not be null and *out_value must be the null handle"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* layer =
    map_native(live)->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto property = layer->getProperty(string_from_view(property_name));
  if (property.getKind() == mbgl::style::StyleProperty::Kind::Undefined) {
    return MLN_STATUS_OK;
  }
  return create_buffer(serialize_json_value(property.getValue()), out_value);
}

auto map_set_layer_filter(
  mln_map map, mln_buffer_view layer_id, const mln_buffer_view* filter
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(layer_id, "layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* layer =
    map_native(live)->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (filter == nullptr) {
    layer->setFilter(mbgl::style::Filter{});
    return MLN_STATUS_OK;
  }

  auto native_filter = to_native_style_filter(filter);
  if (!native_filter) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  layer->setFilter(*native_filter);
  return MLN_STATUS_OK;
}

auto map_get_layer_filter(
  mln_map map, mln_buffer_view layer_id, mln_buffer* out_filter
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(layer_id, "layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_filter == nullptr || *out_filter != MLN_HANDLE_NULL) {
    set_thread_error(
      "out_filter must not be null and *out_filter must be the null handle"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* layer =
    map_native(live)->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto filter = layer->getFilter().serialize();
  if (filter.is<mbgl::NullValue>()) {
    return MLN_STATUS_OK;
  }
  return create_buffer(serialize_json_value(filter), out_filter);
}

namespace {

auto resolve_layer_for_access(
  mln_map map, mln_buffer_view layer_id, mbgl::style::Layer*& out_layer
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(layer_id, "layer_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (layer_id.size == 0) {
    set_thread_error("layer_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* layer =
    map_native(live)->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_layer = layer;
  return MLN_STATUS_OK;
}

// MapLibre's setProperty path logs a warning and does nothing when a layer type
// takes no source, so the typed setters reject that case instead.
auto require_layer_takes_source(
  const mbgl::style::Layer& layer, const char* field
) -> bool {
  if (
    layer.getTypeInfo()->source == mbgl::style::LayerTypeInfo::Source::Required
  ) {
    return true;
  }
  auto message = std::string{"layer type does not take a "} + field +
                 "; layer id is " + layer.getID();
  set_thread_error(message.c_str());
  return false;
}

// MapLibre stores the layer zoom range as floats, and infinities survive the
// narrowing that bounds a layer to one end of the range.
auto validate_layer_zoom(double zoom, const char* field) -> bool {
  if (!std::isnan(zoom)) {
    return true;
  }
  auto message = std::string{field} + " must not be NaN";
  set_thread_error(message.c_str());
  return false;
}

}  // namespace

auto map_set_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_layer
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_layer, "source_layer")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!require_layer_takes_source(*layer, "source-layer")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  layer->setSourceLayer(string_from_view(source_layer));
  return MLN_STATUS_OK;
}

auto map_copy_layer_source_layer(
  mln_map map, mln_buffer_view layer_id, char* out_source_layer,
  size_t source_layer_capacity, size_t* out_source_layer_size
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return copy_text(
    layer->getSourceLayer(), out_source_layer, source_layer_capacity,
    out_source_layer_size, "source_layer_capacity"
  );
}

auto map_set_layer_source_id(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_string_view(source_id, "source_id")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (source_id.size == 0) {
    set_thread_error("source_id must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!require_layer_takes_source(*layer, "source")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  layer->setSourceID(string_from_view(source_id));
  return MLN_STATUS_OK;
}

auto map_copy_layer_source_id(
  mln_map map, mln_buffer_view layer_id, char* out_source_id,
  size_t source_id_capacity, size_t* out_source_id_size
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return copy_text(
    layer->getSourceID(), out_source_id, source_id_capacity, out_source_id_size,
    "source_id_capacity"
  );
}

auto map_set_layer_min_zoom(
  mln_map map, mln_buffer_view layer_id, double min_zoom
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_layer_zoom(min_zoom, "min_zoom")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  layer->setMinZoom(static_cast<float>(min_zoom));
  return MLN_STATUS_OK;
}

auto map_get_layer_min_zoom(
  mln_map map, mln_buffer_view layer_id, double* out_min_zoom
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_min_zoom == nullptr) {
    set_thread_error("out_min_zoom must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_min_zoom = static_cast<double>(layer->getMinZoom());
  return MLN_STATUS_OK;
}

auto map_set_layer_max_zoom(
  mln_map map, mln_buffer_view layer_id, double max_zoom
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_layer_zoom(max_zoom, "max_zoom")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  layer->setMaxZoom(static_cast<float>(max_zoom));
  return MLN_STATUS_OK;
}

auto map_get_layer_max_zoom(
  mln_map map, mln_buffer_view layer_id, double* out_max_zoom
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_max_zoom == nullptr) {
    set_thread_error("out_max_zoom must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_max_zoom = static_cast<double>(layer->getMaxZoom());
  return MLN_STATUS_OK;
}

auto map_set_layer_visibility(
  mln_map map, mln_buffer_view layer_id, uint32_t visibility
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  auto native_visibility = mbgl::style::VisibilityType::Visible;
  switch (visibility) {
    case MLN_STYLE_LAYER_VISIBILITY_VISIBLE:
      native_visibility = mbgl::style::VisibilityType::Visible;
      break;
    case MLN_STYLE_LAYER_VISIBILITY_NONE:
      native_visibility = mbgl::style::VisibilityType::None;
      break;
    default:
      set_thread_error("visibility is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }

  layer->setVisibility(native_visibility);
  return MLN_STATUS_OK;
}

auto map_get_layer_visibility(
  mln_map map, mln_buffer_view layer_id, uint32_t* out_visibility
) -> mln_status {
  mbgl::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_visibility == nullptr) {
    set_thread_error("out_visibility must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_visibility = layer->getVisibility() == mbgl::style::VisibilityType::None
                      ? MLN_STYLE_LAYER_VISIBILITY_NONE
                      : MLN_STYLE_LAYER_VISIBILITY_VISIBLE;
  return MLN_STATUS_OK;
}

}  // namespace mln::core
