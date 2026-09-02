#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
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
#include <vector>

#include <mln/actor/actor_ref.hpp>
#include <mln/actor/mailbox.hpp>
#include <mln/actor/scheduler.hpp>
#include <mln/gfx/rendering_stats.hpp>
#include <mln/map/bound_options.hpp>
#include <mln/map/camera.hpp>
#include <mln/map/map.hpp>
#include <mln/map/map_observer.hpp>
#include <mln/map/map_options.hpp>
#include <mln/map/map_projection.hpp>
#include <mln/map/mode.hpp>
#include <mln/map/projection_mode.hpp>
#include <mln/renderer/renderer_frontend.hpp>
#include <mln/renderer/renderer_observer.hpp>
#include <mln/renderer/update_parameters.hpp>
#include <mln/style/conversion.hpp>
#include <mln/style/conversion/json.hpp>
#include <mln/style/conversion/layer.hpp>   // IWYU pragma: keep
#include <mln/style/conversion/light.hpp>   // IWYU pragma: keep
#include <mln/style/conversion/source.hpp>  // IWYU pragma: keep
#include <mln/style/conversion_impl.hpp>
#include <mln/style/image.hpp>
#include <mln/style/layer.hpp>
#include <mln/style/layers/color_relief_layer.hpp>
#include <mln/style/layers/hillshade_layer.hpp>
#include <mln/style/layers/location_indicator_layer.hpp>
#include <mln/style/light.hpp>
#include <mln/style/rapidjson_conversion.hpp>
#include <mln/style/source.hpp>
#include <mln/style/sources/custom_geometry_source.hpp>
#include <mln/style/sources/custom_vector_source.hpp>
#include <mln/style/sources/geojson_source.hpp>
#include <mln/style/sources/image_source.hpp>
#include <mln/style/sources/raster_dem_source.hpp>
#include <mln/style/sources/raster_source.hpp>
#include <mln/style/sources/vector_source.hpp>
#include <mln/style/style.hpp>
#include <mln/style/style_property.hpp>
#include <mln/style/transition_options.hpp>
#include <mln/style/types.hpp>
#include <mln/tile/tile_id.hpp>
#include <mln/tile/tile_operation.hpp>
#include <mln/util/chrono.hpp>
#include <mln/util/constants.hpp>
#include <mln/util/feature.hpp>
#include <mln/util/geo.hpp>
#include <mln/util/image.hpp>
#include <mln/util/immutable.hpp>
#include <mln/util/projection.hpp>
#include <mln/util/range.hpp>
#include <mln/util/size.hpp>
#include <mln/util/tileset.hpp>
#include <mln/util/vectors.hpp>

#include "map/map.hpp"

#include "bytes/buffer.hpp"
#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "geojson/geojson_source_data.hpp"
#include "handles/handle_table.hpp"
#include "map/feature_state.hpp"
#include "maplibre_native_c.h"
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

}  // namespace mln::core

namespace {

auto buffer_view_from_string(const std::string& value) -> mln_buffer_view {
  return {
    .data = value.data(),
    .size = value.size(),
  };
}

enum class TileSourceOptionKind : uint8_t { Vector, Raster, RasterDEM };

constexpr auto default_map_width = uint32_t{256};
constexpr auto default_map_height = uint32_t{256};
constexpr double default_scale_factor = 1.0;

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

auto validate_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln_status;
auto validate_lat_lng_array(
  const mln_lat_lng* coordinates, size_t coordinate_count, bool allow_empty
) -> mln_status;
auto to_native_lat_lng(mln_lat_lng coordinate) -> mln::LatLng;
auto from_native_lat_lng(const mln::LatLng& coordinate) -> mln_lat_lng;
auto to_native_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln::LatLngBounds;
auto from_native_lat_lng_bounds(const mln::LatLngBounds& bounds)
  -> mln_lat_lng_bounds;

auto to_c_source_type(mln::style::SourceType type) -> uint32_t {
  switch (type) {
    case mln::style::SourceType::Vector:
      return MLN_STYLE_SOURCE_TYPE_VECTOR;
    case mln::style::SourceType::Raster:
      return MLN_STYLE_SOURCE_TYPE_RASTER;
    case mln::style::SourceType::RasterDEM:
      return MLN_STYLE_SOURCE_TYPE_RASTER_DEM;
    case mln::style::SourceType::GeoJSON:
      return MLN_STYLE_SOURCE_TYPE_GEOJSON;
    case mln::style::SourceType::Video:
      return MLN_STYLE_SOURCE_TYPE_VIDEO;
    case mln::style::SourceType::Annotations:
      return MLN_STYLE_SOURCE_TYPE_ANNOTATIONS;
    case mln::style::SourceType::Image:
      return MLN_STYLE_SOURCE_TYPE_IMAGE;
    case mln::style::SourceType::CustomVector:
      return MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR;
    case mln::style::SourceType::CustomMVTVector:
      return MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR;
  }
  assert(false);
  return MLN_STYLE_SOURCE_TYPE_UNKNOWN;
}

auto to_c_tile_scheme(mln::Tileset::Scheme scheme) -> uint32_t {
  switch (scheme) {
    case mln::Tileset::Scheme::XYZ:
      return MLN_STYLE_TILE_SCHEME_XYZ;
    case mln::Tileset::Scheme::TMS:
      return MLN_STYLE_TILE_SCHEME_TMS;
  }
  assert(false);
  return MLN_STYLE_TILE_SCHEME_XYZ;
}

auto to_c_vector_encoding(mln::Tileset::VectorEncoding encoding) -> uint32_t {
  switch (encoding) {
    case mln::Tileset::VectorEncoding::Mapbox:
      return MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
    case mln::Tileset::VectorEncoding::MLT:
      return MLN_STYLE_VECTOR_TILE_ENCODING_MLT;
  }
  assert(false);
  return MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
}

auto to_c_raster_encoding(mln::Tileset::RasterEncoding encoding) -> uint32_t {
  switch (encoding) {
    case mln::Tileset::RasterEncoding::Mapbox:
      return MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
    case mln::Tileset::RasterEncoding::Terrarium:
      return MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM;
  }
  assert(false);
  return MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
}

auto tile_source_from_source(const mln::style::Source& source)
  -> const mln::style::TileSource* {
  switch (source.getType()) {
    case mln::style::SourceType::Vector:
      return source.as<mln::style::VectorSource>();
    case mln::style::SourceType::Raster:
      return source.as<mln::style::RasterSource>();
    case mln::style::SourceType::RasterDEM:
      return source.as<mln::style::RasterDEMSource>();
    default:
      return nullptr;
  }
}

auto inline_tileset(const mln::style::TileSource& source)
  -> const mln::Tileset* {
  const auto& url_or_tileset = source.getURLOrTileset();
  return url_or_tileset.is<mln::Tileset>() ? &url_or_tileset.get<mln::Tileset>()
                                           : nullptr;
}

auto source_url(const mln::style::Source& source)
  -> std::optional<std::string> {
  if (const auto* tile_source = tile_source_from_source(source)) {
    return tile_source->getURL();
  }
  if (const auto* geojson = source.as<mln::style::GeoJSONSource>()) {
    return geojson->getURL();
  }
  if (const auto* image = source.as<mln::style::ImageSource>()) {
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
  return validate_lat_lng_bounds(options.bounds);
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

auto to_native_tile_scheme(uint32_t scheme) -> mln::Tileset::Scheme {
  return scheme == MLN_STYLE_TILE_SCHEME_TMS ? mln::Tileset::Scheme::TMS
                                             : mln::Tileset::Scheme::XYZ;
}

auto to_native_vector_encoding(uint32_t encoding)
  -> mln::Tileset::VectorEncoding {
  return encoding == MLN_STYLE_VECTOR_TILE_ENCODING_MLT
           ? mln::Tileset::VectorEncoding::MLT
           : mln::Tileset::VectorEncoding::Mapbox;
}

auto to_native_raster_encoding(uint32_t encoding)
  -> mln::Tileset::RasterEncoding {
  return encoding == MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM
           ? mln::Tileset::RasterEncoding::Terrarium
           : mln::Tileset::RasterEncoding::Mapbox;
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
) -> std::optional<mln::Tileset> {
  if (options.min_zoom > options.max_zoom) {
    mln::core::set_thread_error(
      "effective min_zoom must be less than or equal to max_zoom"
    );
    return std::nullopt;
  }

  auto tileset = mln::Tileset{
    to_native_tile_urls(tiles, tile_count),
    mln::Range<uint8_t>{
      static_cast<uint8_t>(options.min_zoom),
      static_cast<uint8_t>(options.max_zoom)
    },
    string_from_view(options.attribution),
    to_native_tile_scheme(options.scheme),
    std::nullopt,
    vector_source
      ? std::optional<mln::Tileset::VectorEncoding>{to_native_vector_encoding(
          options.vector_encoding
        )}
      : std::nullopt
  };
  if (has_tile_source_option(options, MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS)) {
    tileset.bounds = to_native_lat_lng_bounds(options.bounds);
  }
  return tileset;
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

auto to_c_canonical_tile_id(const mln::CanonicalTileID& tile_id)
  -> mln_canonical_tile_id {
  return mln_canonical_tile_id{.z = tile_id.z, .x = tile_id.x, .y = tile_id.y};
}

auto to_native_tile_function(
  void (*callback)(void*, mln_canonical_tile_id), void* user_data
) -> mln::style::TileFunction {
  if (callback == nullptr) {
    return nullptr;
  }
  return [callback, user_data](const mln::CanonicalTileID& tile_id) -> void {
    try {
      callback(user_data, to_c_canonical_tile_id(tile_id));
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("custom source callback threw");
    }
  };
}

auto to_native_custom_geometry_source_options(
  const mln_custom_geometry_source_options& options
) -> mln::style::CustomGeometrySource::Options {
  auto result = mln::style::CustomGeometrySource::Options{};
  result.fetchTileFunction =
    to_native_tile_function(options.fetch_tile, options.user_data);
  result.cancelTileFunction =
    to_native_tile_function(options.cancel_tile, options.user_data);
  result.zoomRange = mln::Range<uint8_t>{
    static_cast<uint8_t>(options.min_zoom),
    static_cast<uint8_t>(options.max_zoom)
  };
  result.tileOptions = mln::style::CustomGeometrySource::TileOptions{
    .tolerance = options.tolerance,
    .tileSize = static_cast<uint16_t>(options.tile_size),
    .buffer = static_cast<uint16_t>(options.buffer),
    .clip = options.clip,
    .wrap = options.wrap
  };
  return result;
}

auto has_custom_mvt_vector_source_option(
  const mln_custom_mvt_vector_source_options& options, uint32_t field
) -> bool {
  return (options.fields & field) != 0U;
}

auto effective_custom_mvt_vector_source_options(
  const mln_custom_mvt_vector_source_options& options
) -> mln_custom_mvt_vector_source_options;

auto validate_custom_mvt_vector_source_options(
  const mln_custom_mvt_vector_source_options* options
) -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_custom_mvt_vector_source_options)) {
    mln::core::set_thread_error(
      "mln_custom_mvt_vector_source_options.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM) |
    MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_custom_mvt_vector_source_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->fetch_tile == nullptr) {
    mln::core::set_thread_error("fetch_tile must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    has_custom_mvt_vector_source_option(
      *options, MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
    )
  ) {
    const auto status =
      validate_custom_geometry_zoom(options->min_zoom, "min_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    has_custom_mvt_vector_source_option(
      *options, MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
    )
  ) {
    const auto status =
      validate_custom_geometry_zoom(options->max_zoom, "max_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  const auto effective = effective_custom_mvt_vector_source_options(*options);
  if (effective.min_zoom > effective.max_zoom) {
    mln::core::set_thread_error(
      "min_zoom must be less than or equal to max_zoom"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto effective_custom_mvt_vector_source_options(
  const mln_custom_mvt_vector_source_options& options
) -> mln_custom_mvt_vector_source_options {
  auto result = mln::core::custom_mvt_vector_source_options_default();
  result.fields = options.fields;
  result.fetch_tile = options.fetch_tile;
  result.cancel_tile = options.cancel_tile;
  result.user_data = options.user_data;
  result.release_user_data = options.release_user_data;
  if (
    has_custom_mvt_vector_source_option(
      options, MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
    )
  ) {
    result.min_zoom = options.min_zoom;
  }
  if (
    has_custom_mvt_vector_source_option(
      options, MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
    )
  ) {
    result.max_zoom = options.max_zoom;
  }
  return result;
}

auto to_native_custom_mvt_vector_source_options(
  const mln_custom_mvt_vector_source_options& options
) -> mln::style::CustomVectorSource::Options {
  auto result = mln::style::CustomVectorSource::Options{};
  result.fetchTileFunction =
    to_native_tile_function(options.fetch_tile, options.user_data);
  result.cancelTileFunction =
    to_native_tile_function(options.cancel_tile, options.user_data);
  result.zoomRange = mln::Range<uint8_t>{
    static_cast<uint8_t>(options.min_zoom),
    static_cast<uint8_t>(options.max_zoom)
  };
  return result;
}

enum class CallbackSourceKind : uint8_t { CustomGeometry, CustomMvtVector };

using CallbackSourceRelease = void (*)(void*);

auto source_matches_kind(
  const mln::style::Source* source, CallbackSourceKind kind
) -> bool {
  if (source == nullptr) {
    return false;
  }
  switch (kind) {
    case CallbackSourceKind::CustomGeometry:
      return source->as<mln::style::CustomGeometrySource>() != nullptr;
    case CallbackSourceKind::CustomMvtVector:
      return source->as<mln::style::CustomVectorSource>() != nullptr;
  }
  return false;
}

// Holds the release callback owed for each tracked callback source. A host
// cannot see when a style load drops a source, so this layer tracks it.
//
// Every mutation runs on the map owner thread, which is the only thread that
// adds or removes sources and the only thread mbgl reports style loads on.
class CallbackSourceRegistry final {
 public:
  CallbackSourceRegistry() = default;

  // Runs when the owning map is destroyed, after the mln::Map that could still
  // call into the source is gone.
  ~CallbackSourceRegistry() { release_all(); }

  CallbackSourceRegistry(const CallbackSourceRegistry&) = delete;
  CallbackSourceRegistry(CallbackSourceRegistry&&) = delete;
  auto operator=(const CallbackSourceRegistry&)
    -> CallbackSourceRegistry& = delete;
  auto operator=(CallbackSourceRegistry&&) -> CallbackSourceRegistry& = delete;

  // A source with no release callback is not tracked.
  auto add(
    const std::string& source_id, CallbackSourceKind kind,
    CallbackSourceRelease release, void* user_data
  ) -> void {
    // An entry already under this ID belongs to a source that the style dropped
    // before reconciliation ran, so it still owes its release. Invoke it here
    // rather than letting the assignment below drop it, which keeps
    // exactly-once independent of when the style-loaded observer reconciles.
    if (const auto stale = entries_.find(source_id); stale != entries_.end()) {
      const auto owed = stale->second;
      entries_.erase(stale);
      invoke(owed);
    }
    if (release == nullptr) {
      return;
    }
    // The caller tracks before the style takes the source, so a throw here
    // leaves nothing committed and the caller still owns user_data. A failed
    // add owes no release.
    entries_.insert_or_assign(source_id, Entry{kind, release, user_data});
  }

  // Drops an entry without releasing it, for a source the style then rejected.
  // The caller still owns user_data on that path.
  auto untrack(const std::string& source_id) noexcept -> void {
    entries_.erase(source_id);
  }

  // The observer that reports style loads has no route to a style, so the
  // registry keeps the map it belongs to. destroy_map() clears it before the
  // map is destroyed.
  auto attach(mln::Map& map) noexcept -> void { map_ = &map; }

  auto detach() noexcept -> void { map_ = nullptr; }

  auto release(const std::string& source_id) -> void {
    const auto entry = entries_.find(source_id);
    if (entry == entries_.end()) {
      return;
    }
    const auto owed = entry->second;
    entries_.erase(entry);
    invoke(owed);
  }

  // Releases every tracked source the current style no longer holds. A style
  // document cannot declare a callback source, so a source of another type
  // under a tracked ID means the tracked source is gone.
  auto reconcile() -> void {
    if (map_ == nullptr || entries_.empty()) {
      return;
    }
    auto& style = map_->getStyle();
    auto owed = std::vector<Entry>{};
    for (auto entry = entries_.begin(); entry != entries_.end();) {
      auto* source = style.getSource(entry->first);
      if (source_matches_kind(source, entry->second.kind)) {
        ++entry;
        continue;
      }
      owed.push_back(entry->second);
      entry = entries_.erase(entry);
    }
    for (const auto& release : owed) {
      invoke(release);
    }
  }

  auto release_all() -> void {
    auto owed = std::move(entries_);
    entries_.clear();
    for (const auto& entry : owed) {
      invoke(entry.second);
    }
  }

 private:
  struct Entry {
    CallbackSourceKind kind = CallbackSourceKind::CustomGeometry;
    CallbackSourceRelease release = nullptr;
    void* user_data = nullptr;
  };

  static auto invoke(const Entry& entry) noexcept -> void {
    try {
      entry.release(entry.user_data);
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("callback source release threw");
    }
  }

  std::unordered_map<std::string, Entry> entries_;
  mln::Map* map_ = nullptr;
};

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
  -> mln::CanonicalTileID {
  return mln::CanonicalTileID{
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
  mln::style::Style& style, const std::string& source_id
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
  -> mln::style::ImageStretches {
  auto native = mln::style::ImageStretches{};
  native.reserve(count);
  for (size_t index = 0; index < count; index += 1) {
    native.emplace_back(stretches[index].from, stretches[index].to);
  }
  return native;
}

auto to_native_text_fit(uint32_t value) -> mln::style::TextFit {
  switch (value) {
    case MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY:
      return mln::style::TextFit::stretchOnly;
    case MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL:
      return mln::style::TextFit::proportional;
    default:
      return mln::style::TextFit::stretchOrShrink;
  }
}

auto from_native_text_fit(mln::style::TextFit value) -> uint32_t {
  switch (value) {
    case mln::style::TextFit::stretchOnly:
      return MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY;
    case mln::style::TextFit::proportional:
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
) -> mln::PremultipliedImage {
  auto result = mln::PremultipliedImage{mln::Size{image.width, image.height}};
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

auto style_image_info_from_native(const mln::style::Image& image)
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
  -> std::array<mln::LatLng, 4> {
  auto result = std::array<mln::LatLng, 4>{};
  const auto coordinate_span = std::span<const mln_lat_lng>{coordinates, 4};
  auto index = size_t{0};
  for (const auto coordinate : coordinate_span) {
    result.at(index) = to_native_lat_lng(coordinate);
    ++index;
  }
  return result;
}

auto from_native_image_source_coordinates(
  const std::array<mln::LatLng, 4>& coordinates
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

auto to_c_camera_change_mode(mln::MapObserver::CameraChangeMode mode)
  -> int32_t {
  switch (mode) {
    case mln::MapObserver::CameraChangeMode::Immediate:
      return MLN_CAMERA_CHANGE_MODE_IMMEDIATE;
    case mln::MapObserver::CameraChangeMode::Animated:
      return MLN_CAMERA_CHANGE_MODE_ANIMATED;
  }
  assert(false);
  return MLN_CAMERA_CHANGE_MODE_IMMEDIATE;
}

auto to_c_render_mode(mln::MapObserver::RenderMode mode) -> uint32_t {
  switch (mode) {
    case mln::MapObserver::RenderMode::Partial:
      return MLN_RENDER_MODE_PARTIAL;
    case mln::MapObserver::RenderMode::Full:
      return MLN_RENDER_MODE_FULL;
  }
  assert(false);
  return MLN_RENDER_MODE_PARTIAL;
}

auto to_c_rendering_stats(const mln::gfx::RenderingStats& stats)
  -> mln_rendering_stats {
  return mln_rendering_stats{
    .encoding_time = stats.encodingTime,
    .rendering_time = stats.renderingTime,
    .frame_count = stats.numFrames,
    .draw_call_count = stats.numDrawCalls,
    .total_draw_call_count = stats.totalDrawCalls
  };
}

auto render_frame_payload(const mln::MapObserver::RenderFrameStatus& status)
  -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.render_frame = mln_runtime_event_render_frame{
    .mode = to_c_render_mode(status.mode),
    .needs_repaint = status.needsRepaint,
    .placement_changed = status.placementChanged,
    .stats = to_c_rendering_stats(status.renderingStats)
  };
  return payload;
}

auto render_map_payload(mln::MapObserver::RenderMode mode)
  -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.render_map =
    mln_runtime_event_render_map{.mode = to_c_render_mode(mode)};
  return payload;
}

auto to_c_tile_operation(mln::TileOperation operation) -> uint32_t {
  switch (operation) {
    case mln::TileOperation::RequestedFromCache:
      return MLN_TILE_OPERATION_REQUESTED_FROM_CACHE;
    case mln::TileOperation::RequestedFromNetwork:
      return MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK;
    case mln::TileOperation::LoadFromNetwork:
      return MLN_TILE_OPERATION_LOAD_FROM_NETWORK;
    case mln::TileOperation::LoadFromCache:
      return MLN_TILE_OPERATION_LOAD_FROM_CACHE;
    case mln::TileOperation::StartParse:
      return MLN_TILE_OPERATION_START_PARSE;
    case mln::TileOperation::EndParse:
      return MLN_TILE_OPERATION_END_PARSE;
    case mln::TileOperation::Error:
      return MLN_TILE_OPERATION_ERROR;
    case mln::TileOperation::Cancelled:
      return MLN_TILE_OPERATION_CANCELLED;
    case mln::TileOperation::NullOp:
      return MLN_TILE_OPERATION_NULL;
  }
  return MLN_TILE_OPERATION_NULL;
}

auto to_c_tile_id(const mln::OverscaledTileID& tile_id) -> mln_tile_id {
  return mln_tile_id{
    .overscaled_z = tile_id.overscaledZ,
    .wrap = tile_id.wrap,
    .canonical_z = tile_id.canonical.z,
    .canonical_x = tile_id.canonical.x,
    .canonical_y = tile_id.canonical.y
  };
}

auto tile_action_payload(
  mln::TileOperation operation, const mln::OverscaledTileID& tile_id
) -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.tile_action = mln_runtime_event_tile_action{
    .operation = to_c_tile_operation(operation),
    .tile_id = to_c_tile_id(tile_id)
  };
  return payload;
}

// Every callback tests the map's subscription mask before it builds anything,
// so an unselected event allocates no payload, message, or queue node.
class HeadlessObserver final : public mln::MapObserver {
 public:
  HeadlessObserver(
    mln_runtime runtime, mln_map map,
    std::shared_ptr<mln::core::MapEventState> event_state,
    std::shared_ptr<CallbackSourceRegistry> callback_sources
  )
      : runtime_(runtime),
        map_(map),
        event_state_(std::move(event_state)),
        callback_sources_(std::move(callback_sources)) {}

  void onCameraWillChange(CameraChangeMode mode) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE)) {
      return;
    }
    push(
      MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, to_c_camera_change_mode(mode)
    );
  }

  void onCameraIsChanging() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING);
  }

  void onCameraDidChange(CameraChangeMode mode) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE)) {
      return;
    }
    push(
      MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, to_c_camera_change_mode(mode)
    );
  }

  void onWillStartLoadingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED);
  }

  void onDidFinishLoadingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED);
  }

  // The failure text is map state that both style setters read, so it is
  // recorded whatever the mask selects.
  void onDidFailLoadingMap(
    mln::MapLoadError error, const std::string& message
  ) override {
    event_state_->style_load_failure = message;
    event_state_->style_load_failed = true;
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_FAILED)) {
      return;
    }
    push(
      MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, static_cast<int32_t>(error),
      message.c_str()
    );
  }

  // A style load can drop callback sources that the previous style held.
  // The release callbacks it owes are map state rather than an event, so the
  // reconciliation runs whatever the mask selects.
  void onDidFinishLoadingStyle() override {
    // The event is queued before the reconciliation, and the registry is held
    // by a local share across it, because reconcile() runs host release
    // callbacks. Nothing may touch this observer or its members after host code
    // runs: a callback that destroys its map destroys this observer with it.
    if (selected(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)) {
      push(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED);
    }
    const auto sources = callback_sources_;
    sources->reconcile();
  }

  void onWillStartRenderingFrame() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED);
  }

  void onDidFinishRenderingFrame(const RenderFrameStatus& status) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME, render_frame_payload(status)
    );
  }

  void onWillStartRenderingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED);
  }

  void onDidFinishRenderingMap(RenderMode mode) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP, render_map_payload(mode)
    );
  }

  void onDidBecomeIdle() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_IDLE)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_IDLE);
  }

  void onStyleImageMissing(const std::string& image_id) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING, MLN_RUNTIME_EVENT_PAYLOAD_NONE,
      mln::core::zeroed_event_payload(), 0, image_id
    );
  }

  void onTileAction(
    mln::TileOperation operation, const mln::OverscaledTileID& tile_id,
    const std::string& source_id
  ) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_TILE_ACTION)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_TILE_ACTION, MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
      tile_action_payload(operation, tile_id), 0, source_id
    );
  }

  // The mask test precedes the try block, so a suppressed render error never
  // formats the exception text.
  void onRenderError(std::exception_ptr error) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR)) {
      return;
    }
    try {
      if (error) {
        std::rethrow_exception(error);
      }
      push(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR);
    } catch (const std::exception& exception) {
      push(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, 0, exception.what());
    } catch (...) {
      push(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, 0, "unknown render error");
    }
  }

 private:
  [[nodiscard]] auto selected(uint32_t type) const noexcept -> bool {
    return mln::core::event_selected(event_state_->mask, type);
  }

  auto push(uint32_t type, int32_t code = 0, const char* message = nullptr)
    -> void {
    mln::core::push_runtime_map_event(runtime_, map_, type, code, message);
  }

  auto push_payload(
    uint32_t type, uint32_t payload_type,
    const mln_runtime_event_payload& payload, int32_t code = 0,
    std::string message = {}
  ) -> void {
    mln::core::push_runtime_map_event_payload(
      runtime_, map_, type, payload_type, payload, code, std::move(message)
    );
  }

  mln_runtime runtime_;
  mln_map map_;
  std::shared_ptr<mln::core::MapEventState> event_state_;
  std::shared_ptr<CallbackSourceRegistry> callback_sources_;
};

// Delivers mln::RendererObserver callbacks on the map's run loop instead of on
// whichever thread rendered. The delegate is mln::Map::Impl, whose handlers
// touch map-thread state, so every callback becomes a mailbox message that runs
// during the host's next mln_runtime_pump(). Forwarding is unconditional, so
// delivery order is the same whether or not the session shares the map's owner
// thread.
class ForwardingRendererObserver final : public mln::RendererObserver {
 public:
  ForwardingRendererObserver(
    mln::Scheduler& map_scheduler, mln::RendererObserver& delegate
  )
      : mailbox_(std::make_shared<mln::Mailbox>(map_scheduler)),
        delegate_(delegate, mailbox_) {}

  ForwardingRendererObserver(const ForwardingRendererObserver&) = delete;
  auto operator=(const ForwardingRendererObserver&)
    -> ForwardingRendererObserver& = delete;
  ForwardingRendererObserver(ForwardingRendererObserver&&) = delete;
  auto operator=(ForwardingRendererObserver&&)
    -> ForwardingRendererObserver& = delete;

  ~ForwardingRendererObserver() override { mailbox_->close(); }

  // Waits out an in-flight receive and drops anything queued, so the delegate
  // can be torn down once this returns. Idempotent.
  auto close() -> void { mailbox_->close(); }

  void onInvalidate() override {
    delegate_.invoke(&mln::RendererObserver::onInvalidate);
  }

  void onResourceError(std::exception_ptr error) override {
    delegate_.invoke(&mln::RendererObserver::onResourceError, error);
  }

  void onWillStartRenderingMap() override {
    delegate_.invoke(&mln::RendererObserver::onWillStartRenderingMap);
  }

  void onWillStartRenderingFrame() override {
    delegate_.invoke(&mln::RendererObserver::onWillStartRenderingFrame);
  }

  void onDidFinishRenderingFrame(
    RenderMode mode, bool repaint_needed, bool placement_changed,
    const mln::gfx::RenderingStats& stats
  ) override {
    // The name carries three overloads; mln::Map::Impl implements only this
    // one.
    void (mln::RendererObserver::*method)(
      RenderMode, bool, bool, const mln::gfx::RenderingStats&
    ) = &mln::RendererObserver::onDidFinishRenderingFrame;
    delegate_.invoke(method, mode, repaint_needed, placement_changed, stats);
  }

  void onDidFinishRenderingMap() override {
    delegate_.invoke(&mln::RendererObserver::onDidFinishRenderingMap);
  }

  void onStyleImageMissing(
    const std::string& id, const StyleImageMissingCallback& done
  ) override {
    delegate_.invoke(&mln::RendererObserver::onStyleImageMissing, id, done);
  }

  void onRemoveUnusedStyleImages(const std::vector<std::string>& ids) override {
    delegate_.invoke(&mln::RendererObserver::onRemoveUnusedStyleImages, ids);
  }

  void onPreCompileShader(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onPreCompileShader, id, type, defines
    );
  }

  void onPostCompileShader(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onPostCompileShader, id, type, defines
    );
  }

  void onShaderCompileFailed(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onShaderCompileFailed, id, type, defines
    );
  }

  void onGlyphsLoaded(
    const mln::FontStack& stack, const mln::GlyphRange& range
  ) override {
    delegate_.invoke(&mln::RendererObserver::onGlyphsLoaded, stack, range);
  }

  void onGlyphsError(
    const mln::FontStack& stack, const mln::GlyphRange& range,
    std::exception_ptr error
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onGlyphsError, stack, range, error
    );
  }

  void onGlyphsRequested(
    const mln::FontStack& stack, const mln::GlyphRange& range
  ) override {
    delegate_.invoke(&mln::RendererObserver::onGlyphsRequested, stack, range);
  }

  void onTileAction(
    mln::TileOperation operation, const mln::OverscaledTileID& id,
    const std::string& source_id
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onTileAction, operation, id, source_id
    );
  }

  void onRenderError(std::exception_ptr error) override {
    delegate_.invoke(&mln::RendererObserver::onRenderError, error);
  }

 private:
  std::shared_ptr<mln::Mailbox> mailbox_;
  mln::ActorRef<mln::RendererObserver> delegate_;
};

// Map mutations a render session reaches for from its own owner thread. The
// mailbox on the map's run loop keeps mln::Map single-threaded, and closing it
// during map teardown turns late messages into no-ops.
class MapCommands {
 public:
  explicit MapCommands(mln::Map& map) : map_(map) {}

  auto set_size(uint32_t width, uint32_t height) -> void {
    map_.setSize(mln::Size{width, height});
  }

  auto trigger_repaint() -> void { map_.triggerRepaint(); }

 private:
  mln::Map& map_;
};

class HeadlessFrontend final : public mln::RendererFrontend {
 public:
  // The thread pool tag must be a default-constructed identity, unique per map.
  // SimpleIdentity::Empty pools every map's work into one bucket that
  // waitForEmpty() cannot wait on, because it remaps the empty tag to the
  // pool's own identity. The run loop comes in by reference because mbgl calls
  // setObserver() from the map constructor; it outlives the map.
  HeadlessFrontend(
    mln_runtime runtime, mln_map map, mln::util::RunLoop& run_loop,
    std::shared_ptr<mln::core::MapEventState> event_state
  )
      : runtime_(runtime),
        map_(map),
        run_loop_(run_loop),
        event_state_(std::move(event_state)),
        thread_pool_(
          mln::Scheduler::GetBackground(), mln::util::SimpleIdentity{}
        ) {}

  void reset() override {
    const std::scoped_lock lock(latest_update_mutex_);
    latest_update_.reset();
  }

  // mln::Map calls this once, from its constructor, on the map owner thread.
  void setObserver(mln::RendererObserver& observer) override {
    observer_ =
      std::make_unique<ForwardingRendererObserver>(run_loop_, observer);
  }

  // The latest update is render state that a session reads whatever the mask
  // selects, so it is stored first. Pushing outside the update lock keeps
  // `latest_update_mutex_` off the handle table and `event_mutex`; only the map
  // owner thread reaches this, so nothing can interleave in between.
  void update(std::shared_ptr<mln::UpdateParameters> update) override {
    {
      const std::scoped_lock lock(latest_update_mutex_);
      latest_update_ = std::move(update);
    }
    if (!mln::core::event_selected(
          event_state_->mask, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
        )) {
      return;
    }
    mln::core::push_runtime_map_event(
      runtime_, map_, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
    );
  }

  [[nodiscard]] auto latest_update() const
    -> std::shared_ptr<mln::UpdateParameters> {
    const std::scoped_lock lock(latest_update_mutex_);
    return latest_update_;
  }

  auto run_render_jobs() -> void { thread_pool_.runRenderJobs(); }

  // Every map must call this: only waitForEmpty() erases the map's bucket in
  // the process-global scheduler, which the worker loop otherwise keeps
  // walking.
  auto shutdown_thread_pool() -> void {
    thread_pool_.runRenderJobs(/*closeQueue=*/true);
    thread_pool_.waitForEmpty();
  }

  [[nodiscard]] auto renderer_observer() const -> mln::RendererObserver* {
    return observer_.get();
  }

  // Must run before the map that backs the delegate is torn down.
  auto close_renderer_observer() -> void {
    if (observer_ != nullptr) {
      observer_->close();
    }
  }

  [[nodiscard]] auto getThreadPool() const
    -> const mln::TaggedScheduler& override {
    return thread_pool_;
  }

 private:
  mln_runtime runtime_;
  mln_map map_;
  mln::util::RunLoop& run_loop_;
  std::shared_ptr<mln::core::MapEventState> event_state_;
  std::unique_ptr<ForwardingRendererObserver> observer_;
  mln::TaggedScheduler thread_pool_;
  mutable std::mutex latest_update_mutex_;
  std::shared_ptr<mln::UpdateParameters> latest_update_;
};

auto validate_map_options(const mln_map_options* options) -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }

  if (options->size < sizeof(mln_map_options)) {
    mln::core::set_thread_error("mln_map_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // Validated here as well as in the setter, so a mask this library cannot
  // honour is rejected wherever it arrives.
  constexpr auto known_mask_bits =
    static_cast<uint64_t>(MLN_RUNTIME_EVENT_MASK_ALL);
  if ((options->event_mask & ~known_mask_bits) != 0U) {
    mln::core::set_thread_error(
      "mln_map_options.event_mask contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (
    options->width == 0 || options->height == 0 ||
    !std::isfinite(options->scale_factor) || options->scale_factor <= 0
  ) {
    mln::core::set_thread_error(
      "map dimensions and scale_factor must be positive"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  switch (options->map_mode) {
    case MLN_MAP_MODE_CONTINUOUS:
    case MLN_MAP_MODE_STATIC:
    case MLN_MAP_MODE_TILE:
      break;
    default:
      mln::core::set_thread_error("map_mode is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

auto to_native_map_mode(uint32_t mode) -> mln::MapMode {
  switch (mode) {
    case MLN_MAP_MODE_STATIC:
      return mln::MapMode::Static;
    case MLN_MAP_MODE_TILE:
      return mln::MapMode::Tile;
    case MLN_MAP_MODE_CONTINUOUS:
      return mln::MapMode::Continuous;
    default:
      assert(false);
      return mln::MapMode::Continuous;
  }
}

auto is_still_map_mode(uint32_t mode) -> bool {
  return mode == MLN_MAP_MODE_STATIC || mode == MLN_MAP_MODE_TILE;
}

auto exception_message(std::exception_ptr error) -> std::string {
  if (!error) {
    return {};
  }
  try {
    std::rethrow_exception(error);
  } catch (const std::exception& exception) {
    return exception.what();
  } catch (...) {
    return "unknown still-image request error";
  }
}

auto validate_lat_lng(mln_lat_lng coordinate) -> mln_status;
auto validate_edge_insets(mln_edge_insets padding) -> mln_status;
auto validate_screen_point(mln_screen_point point) -> mln_status;

auto validate_camera_options(const mln_camera_options* camera) -> mln_status {
  if (camera == nullptr) {
    mln::core::set_thread_error("camera must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (camera->size < sizeof(mln_camera_options)) {
    mln::core::set_thread_error("mln_camera_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_CAMERA_OPTION_CENTER) | MLN_CAMERA_OPTION_ZOOM |
    MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
    MLN_CAMERA_OPTION_CENTER_ALTITUDE | MLN_CAMERA_OPTION_PADDING |
    MLN_CAMERA_OPTION_ANCHOR | MLN_CAMERA_OPTION_ROLL | MLN_CAMERA_OPTION_FOV;
  if ((camera->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_camera_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if ((camera->fields & MLN_CAMERA_OPTION_CENTER) != 0U) {
    const auto status = validate_lat_lng(
      mln_lat_lng{.latitude = camera->latitude, .longitude = camera->longitude}
    );
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    ((camera->fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0U &&
     !std::isfinite(camera->center_altitude)) ||
    ((camera->fields & MLN_CAMERA_OPTION_ZOOM) != 0U &&
     !std::isfinite(camera->zoom)) ||
    ((camera->fields & MLN_CAMERA_OPTION_BEARING) != 0U &&
     !std::isfinite(camera->bearing)) ||
    ((camera->fields & MLN_CAMERA_OPTION_PITCH) != 0U &&
     !std::isfinite(camera->pitch)) ||
    ((camera->fields & MLN_CAMERA_OPTION_ROLL) != 0U &&
     !std::isfinite(camera->roll)) ||
    ((camera->fields & MLN_CAMERA_OPTION_FOV) != 0U &&
     !std::isfinite(camera->field_of_view))
  ) {
    mln::core::set_thread_error("enabled camera numeric fields must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((camera->fields & MLN_CAMERA_OPTION_PADDING) != 0U) {
    const auto status = validate_edge_insets(camera->padding);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if ((camera->fields & MLN_CAMERA_OPTION_ANCHOR) != 0U) {
    const auto status = validate_screen_point(camera->anchor);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }

  return MLN_STATUS_OK;
}

using DoubleMilliseconds = std::chrono::duration<double, std::milli>;

auto max_native_duration_ms() -> double {
  return std::chrono::duration_cast<DoubleMilliseconds>(mln::Duration::max())
    .count();
}

auto duration_from_milliseconds(double milliseconds) -> mln::Duration {
  return std::chrono::duration_cast<mln::Duration>(
    DoubleMilliseconds{milliseconds}
  );
}

auto milliseconds_from_duration(mln::Duration duration) -> double {
  return std::chrono::duration_cast<DoubleMilliseconds>(duration).count();
}

// The accepted bound is exclusive because mln::Duration::max() has no exact
// double representation: the nearest double converts back to 2^63 ticks, one
// past the largest representable count. The margin holds for a nanosecond
// duration only, so pin the representation.
static_assert(
  std::is_same_v<mln::Duration, std::chrono::nanoseconds>,
  "the accepted duration bound is derived from a nanosecond mln::Duration"
);

auto is_native_duration_ms(double milliseconds) -> bool {
  return std::isfinite(milliseconds) && milliseconds >= 0.0 &&
         milliseconds < max_native_duration_ms();
}

auto validate_animation_options(const mln_animation_options* animation)
  -> mln_status {
  if (animation == nullptr) {
    return MLN_STATUS_OK;
  }
  if (animation->size < sizeof(mln_animation_options)) {
    mln::core::set_thread_error("mln_animation_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_ANIMATION_OPTION_DURATION) |
    MLN_ANIMATION_OPTION_VELOCITY | MLN_ANIMATION_OPTION_MIN_ZOOM |
    MLN_ANIMATION_OPTION_EASING | MLN_ANIMATION_OPTION_TRANSITION_ID;
  if ((animation->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_animation_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (animation->fields & MLN_ANIMATION_OPTION_DURATION) != 0U &&
    !is_native_duration_ms(animation->duration_ms)
  ) {
    mln::core::set_thread_error(
      "animation duration_ms must fit the native duration range"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (animation->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0U &&
    (!std::isfinite(animation->velocity) || animation->velocity <= 0.0)
  ) {
    mln::core::set_thread_error(
      "animation velocity must be positive and finite"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (animation->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0U &&
    !std::isfinite(animation->min_zoom)
  ) {
    mln::core::set_thread_error("animation min_zoom must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_EASING) != 0U) {
    const auto easing = animation->easing;
    if (
      !std::isfinite(easing.x1) || !std::isfinite(easing.y1) ||
      !std::isfinite(easing.x2) || !std::isfinite(easing.y2) ||
      easing.x1 < 0.0 || easing.x1 > 1.0 || easing.x2 < 0.0 || easing.x2 > 1.0
    ) {
      mln::core::set_thread_error(
        "animation easing x values must be within [0, 1] and all easing values "
        "must be finite"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_camera_fit_options(const mln_camera_fit_options* options)
  -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_camera_fit_options)) {
    mln::core::set_thread_error("mln_camera_fit_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_CAMERA_FIT_OPTION_PADDING) |
    MLN_CAMERA_FIT_OPTION_BEARING | MLN_CAMERA_FIT_OPTION_PITCH;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_camera_fit_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_CAMERA_FIT_OPTION_PADDING) != 0U) {
    const auto status = validate_edge_insets(options->padding);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    ((options->fields & MLN_CAMERA_FIT_OPTION_BEARING) != 0U &&
     !std::isfinite(options->bearing)) ||
    ((options->fields & MLN_CAMERA_FIT_OPTION_PITCH) != 0U &&
     !std::isfinite(options->pitch))
  ) {
    mln::core::set_thread_error("camera fit bearing and pitch must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_bound_options(const mln_bound_options* options) -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("bound options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_bound_options)) {
    mln::core::set_thread_error("mln_bound_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_BOUND_OPTION_BOUNDS) | MLN_BOUND_OPTION_MIN_ZOOM |
    MLN_BOUND_OPTION_MAX_ZOOM | MLN_BOUND_OPTION_MIN_PITCH |
    MLN_BOUND_OPTION_MAX_PITCH | MLN_BOUND_OPTION_UNBOUNDED;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_bound_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_BOUND_OPTION_BOUNDS) != 0U &&
    (options->fields & MLN_BOUND_OPTION_UNBOUNDED) != 0U
  ) {
    mln::core::set_thread_error(
      "MLN_BOUND_OPTION_BOUNDS and MLN_BOUND_OPTION_UNBOUNDED are mutually "
      "exclusive"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_BOUND_OPTION_BOUNDS) != 0U) {
    const auto status = validate_lat_lng_bounds(options->bounds);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    ((options->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0U &&
     !std::isfinite(options->min_zoom)) ||
    ((options->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0U &&
     !std::isfinite(options->max_zoom)) ||
    ((options->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0U &&
     !std::isfinite(options->min_pitch)) ||
    ((options->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0U &&
     !std::isfinite(options->max_pitch))
  ) {
    mln::core::set_thread_error("bound numeric fields must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0U &&
    (options->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0U &&
    options->min_zoom > options->max_zoom
  ) {
    mln::core::set_thread_error(
      "min_zoom must be less than or equal to max_zoom"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0U &&
    (options->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0U &&
    options->min_pitch > options->max_pitch
  ) {
    mln::core::set_thread_error(
      "min_pitch must be less than or equal to max_pitch"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vec3(mln_vec3 value, const char* name) -> mln_status {
  if (
    !std::isfinite(value.x) || !std::isfinite(value.y) ||
    !std::isfinite(value.z)
  ) {
    mln::core::set_thread_error(name);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_quaternion(mln_quaternion value) -> mln_status {
  if (
    !std::isfinite(value.x) || !std::isfinite(value.y) ||
    !std::isfinite(value.z) || !std::isfinite(value.w)
  ) {
    mln::core::set_thread_error(
      "free camera orientation values must be finite"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (value.x == 0.0 && value.y == 0.0 && value.z == 0.0 && value.w == 0.0) {
    mln::core::set_thread_error(
      "free camera orientation must not be zero length"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_free_camera_options(const mln_free_camera_options* options)
  -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("free camera options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_free_camera_options)) {
    mln::core::set_thread_error("mln_free_camera_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_FREE_CAMERA_OPTION_POSITION) |
    MLN_FREE_CAMERA_OPTION_ORIENTATION;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_free_camera_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0U) {
    const auto status = validate_vec3(
      options->position, "free camera position values must be finite"
    );
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if ((options->fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0U) {
    const auto status = validate_quaternion(options->orientation);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_projection_mode_options(const mln_projection_mode* mode)
  -> mln_status {
  if (mode == nullptr) {
    mln::core::set_thread_error("projection mode must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (mode->size < sizeof(mln_projection_mode)) {
    mln::core::set_thread_error("mln_projection_mode.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_PROJECTION_MODE_AXONOMETRIC) |
    MLN_PROJECTION_MODE_X_SKEW | MLN_PROJECTION_MODE_Y_SKEW;
  if ((mode->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_projection_mode.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (
    ((mode->fields & MLN_PROJECTION_MODE_X_SKEW) != 0U &&
     !std::isfinite(mode->x_skew)) ||
    ((mode->fields & MLN_PROJECTION_MODE_Y_SKEW) != 0U &&
     !std::isfinite(mode->y_skew))
  ) {
    mln::core::set_thread_error("projection skew values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

auto validate_debug_options(uint32_t options) -> mln_status {
  constexpr auto known_options =
    static_cast<uint32_t>(MLN_MAP_DEBUG_TILE_BORDERS) |
    MLN_MAP_DEBUG_PARSE_STATUS | MLN_MAP_DEBUG_TIMESTAMPS |
    MLN_MAP_DEBUG_COLLISION | MLN_MAP_DEBUG_OVERDRAW |
    MLN_MAP_DEBUG_STENCIL_CLIP | MLN_MAP_DEBUG_DEPTH_BUFFER;
  if ((options & ~known_options) != 0U) {
    mln::core::set_thread_error("debug options contain unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_frustum_offset(mln_edge_insets offset) -> mln_status {
  if (
    !std::isfinite(offset.top) || !std::isfinite(offset.left) ||
    !std::isfinite(offset.bottom) || !std::isfinite(offset.right)
  ) {
    mln::core::set_thread_error("frustum offset values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    offset.top < 0.0 || offset.left < 0.0 || offset.bottom < 0.0 ||
    offset.right < 0.0
  ) {
    mln::core::set_thread_error(
      "frustum offset values must be greater than or equal to 0"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_viewport_options(const mln_map_viewport_options* options)
  -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("viewport options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_map_viewport_options)) {
    mln::core::set_thread_error("mln_map_viewport_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) |
    MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE |
    MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE |
    MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_map_viewport_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0U) {
    switch (options->north_orientation) {
      case MLN_NORTH_ORIENTATION_UP:
      case MLN_NORTH_ORIENTATION_RIGHT:
      case MLN_NORTH_ORIENTATION_DOWN:
      case MLN_NORTH_ORIENTATION_LEFT:
        break;
      default:
        mln::core::set_thread_error("north_orientation is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0U) {
    switch (options->constrain_mode) {
      case MLN_CONSTRAIN_MODE_NONE:
      case MLN_CONSTRAIN_MODE_HEIGHT_ONLY:
      case MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT:
      case MLN_CONSTRAIN_MODE_SCREEN:
        break;
      default:
        mln::core::set_thread_error("constrain_mode is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0U) {
    switch (options->viewport_mode) {
      case MLN_VIEWPORT_MODE_DEFAULT:
      case MLN_VIEWPORT_MODE_FLIPPED_Y:
        break;
      default:
        mln::core::set_thread_error("viewport_mode is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0U) {
    return validate_frustum_offset(options->frustum_offset);
  }
  return MLN_STATUS_OK;
}

auto validate_tile_options(const mln_map_tile_options* options) -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("tile options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_map_tile_options)) {
    mln::core::set_thread_error("mln_map_tile_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) |
    MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS | MLN_MAP_TILE_OPTION_LOD_SCALE |
    MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD |
    MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT | MLN_MAP_TILE_OPTION_LOD_MODE;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_map_tile_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0U &&
    options->prefetch_zoom_delta > std::numeric_limits<uint8_t>::max()
  ) {
    mln::core::set_thread_error("prefetch_zoom_delta must be at most 255");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0U &&
     !std::isfinite(options->lod_min_radius)) ||
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0U &&
     !std::isfinite(options->lod_scale)) ||
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0U &&
     !std::isfinite(options->lod_pitch_threshold)) ||
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0U &&
     !std::isfinite(options->lod_zoom_shift))
  ) {
    mln::core::set_thread_error("tile LOD values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0U) {
    switch (options->lod_mode) {
      case MLN_TILE_LOD_MODE_DEFAULT:
      case MLN_TILE_LOD_MODE_DISTANCE:
        break;
      default:
        mln::core::set_thread_error("lod_mode is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
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

auto validate_screen_point(mln_screen_point point) -> mln_status {
  if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
    mln::core::set_thread_error("screen point values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_screen_point_array(
  const mln_screen_point* points, size_t point_count
) -> mln_status {
  if (point_count == 0) {
    return MLN_STATUS_OK;
  }

  if (points == nullptr) {
    mln::core::set_thread_error("points must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto point_span =
    std::span<const mln_screen_point>{points, point_count};
  for (const auto point : point_span) {
    const auto status = validate_screen_point(point);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_edge_insets(mln_edge_insets padding) -> mln_status {
  if (
    !std::isfinite(padding.top) || !std::isfinite(padding.left) ||
    !std::isfinite(padding.bottom) || !std::isfinite(padding.right) ||
    padding.top < 0.0 || padding.left < 0.0 || padding.bottom < 0.0 ||
    padding.right < 0.0
  ) {
    mln::core::set_thread_error(
      "padding values must be finite and greater than or equal to 0"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_projected_meters(mln_projected_meters meters) -> mln_status {
  if (!std::isfinite(meters.northing) || !std::isfinite(meters.easting)) {
    mln::core::set_thread_error("projected meter values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_native_screen_point(mln_screen_point point) -> mln::ScreenCoordinate;
auto from_native_screen_point(const mln::ScreenCoordinate& point)
  -> mln_screen_point;
auto to_native_edge_insets(mln_edge_insets padding) -> mln::EdgeInsets;
auto from_native_edge_insets(const mln::EdgeInsets& insets) -> mln_edge_insets;

auto to_native_camera(const mln_camera_options& camera) -> mln::CameraOptions {
  auto result = mln::CameraOptions{};
  if ((camera.fields & MLN_CAMERA_OPTION_CENTER) != 0U) {
    result.withCenter(mln::LatLng{camera.latitude, camera.longitude});
  }
  if ((camera.fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0U) {
    result.withCenterAltitude(camera.center_altitude);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_PADDING) != 0U) {
    result.withPadding(to_native_edge_insets(camera.padding));
  }
  if ((camera.fields & MLN_CAMERA_OPTION_ANCHOR) != 0U) {
    result.withAnchor(to_native_screen_point(camera.anchor));
  }
  if ((camera.fields & MLN_CAMERA_OPTION_ZOOM) != 0U) {
    result.withZoom(camera.zoom);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_BEARING) != 0U) {
    result.withBearing(camera.bearing);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_PITCH) != 0U) {
    result.withPitch(camera.pitch);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_ROLL) != 0U) {
    result.withRoll(camera.roll);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_FOV) != 0U) {
    result.withFov(camera.field_of_view);
  }
  return result;
}

auto from_native_camera(const mln::CameraOptions& camera)
  -> mln_camera_options {
  auto result = mln::core::camera_options_default();
  if (camera.center) {
    result.fields |= MLN_CAMERA_OPTION_CENTER;
    result.latitude = camera.center->latitude();
    result.longitude = camera.center->longitude();
  }
  if (camera.centerAltitude) {
    result.fields |= MLN_CAMERA_OPTION_CENTER_ALTITUDE;
    result.center_altitude = *camera.centerAltitude;
  }
  if (camera.padding) {
    result.fields |= MLN_CAMERA_OPTION_PADDING;
    result.padding = from_native_edge_insets(*camera.padding);
  }
  if (camera.anchor) {
    result.fields |= MLN_CAMERA_OPTION_ANCHOR;
    result.anchor = from_native_screen_point(*camera.anchor);
  }
  if (camera.zoom) {
    result.fields |= MLN_CAMERA_OPTION_ZOOM;
    result.zoom = *camera.zoom;
  }
  if (camera.bearing) {
    result.fields |= MLN_CAMERA_OPTION_BEARING;
    result.bearing = *camera.bearing;
  }
  if (camera.pitch) {
    result.fields |= MLN_CAMERA_OPTION_PITCH;
    result.pitch = *camera.pitch;
  }
  if (camera.roll) {
    result.fields |= MLN_CAMERA_OPTION_ROLL;
    result.roll = *camera.roll;
  }
  if (camera.fov) {
    result.fields |= MLN_CAMERA_OPTION_FOV;
    result.field_of_view = *camera.fov;
  }
  return result;
}

auto camera_transition_finished_payload(uint64_t transition_id)
  -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.camera_transition_finished =
    mln_runtime_event_camera_transition_finished{
      .transition_id = transition_id
    };
  return payload;
}

// MapLibre Native owns the returned AnimationOptions for the lifetime of the
// transition, and invokes transitionFinishFn on the map owner thread. The push
// discards events for a destroyed map, so a transition outliving the map
// enqueues nothing. The lambda holds the event state by value, so it reads a
// live mask cell at completion time even for a map that is already gone.
auto to_native_animation(
  mln_runtime runtime, mln_map map,
  const std::shared_ptr<mln::core::MapEventState>& event_state,
  const mln_animation_options* animation
) -> mln::AnimationOptions {
  auto result = mln::AnimationOptions{};
  if (animation == nullptr) {
    return result;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0U) {
    result.transitionFinishFn = [runtime, map, event_state,
                                 transition_id = animation->transition_id] {
      if (!mln::core::event_selected(
            event_state->mask, MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED
          )) {
        return;
      }
      mln::core::push_runtime_map_event_payload(
        runtime, map, MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED,
        MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED,
        camera_transition_finished_payload(transition_id)
      );
    };
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_DURATION) != 0U) {
    result.duration = duration_from_milliseconds(animation->duration_ms);
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0U) {
    result.velocity = animation->velocity;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0U) {
    result.minZoom = animation->min_zoom;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_EASING) != 0U) {
    const auto easing = animation->easing;
    result.easing.emplace(easing.x1, easing.y1, easing.x2, easing.y2);
  }
  return result;
}

auto camera_fit_padding(const mln_camera_fit_options* options)
  -> mln::EdgeInsets {
  if (
    options == nullptr ||
    (options->fields & MLN_CAMERA_FIT_OPTION_PADDING) == 0U
  ) {
    return mln::EdgeInsets{};
  }
  return to_native_edge_insets(options->padding);
}

auto camera_fit_bearing(const mln_camera_fit_options* options)
  -> std::optional<double> {
  if (
    options == nullptr ||
    (options->fields & MLN_CAMERA_FIT_OPTION_BEARING) == 0U
  ) {
    return std::nullopt;
  }
  return options->bearing;
}

auto camera_fit_pitch(const mln_camera_fit_options* options)
  -> std::optional<double> {
  if (
    options == nullptr || (options->fields & MLN_CAMERA_FIT_OPTION_PITCH) == 0U
  ) {
    return std::nullopt;
  }
  return options->pitch;
}

auto to_native_projection_mode(const mln_projection_mode& mode)
  -> mln::ProjectionMode {
  auto result = mln::ProjectionMode{};
  if ((mode.fields & MLN_PROJECTION_MODE_AXONOMETRIC) != 0U) {
    result.withAxonometric(mode.axonometric);
  }
  if ((mode.fields & MLN_PROJECTION_MODE_X_SKEW) != 0U) {
    result.withXSkew(mode.x_skew);
  }
  if ((mode.fields & MLN_PROJECTION_MODE_Y_SKEW) != 0U) {
    result.withYSkew(mode.y_skew);
  }
  return result;
}

auto to_native_debug_options(uint32_t options) -> mln::MapDebugOptions {
  return static_cast<mln::MapDebugOptions>(options);
}

auto from_native_debug_options(mln::MapDebugOptions options) -> uint32_t {
  return static_cast<uint32_t>(options);
}

auto to_native_north_orientation(uint32_t orientation)
  -> mln::NorthOrientation {
  switch (orientation) {
    case MLN_NORTH_ORIENTATION_RIGHT:
      return mln::NorthOrientation::Rightwards;
    case MLN_NORTH_ORIENTATION_DOWN:
      return mln::NorthOrientation::Downwards;
    case MLN_NORTH_ORIENTATION_LEFT:
      return mln::NorthOrientation::Leftwards;
    case MLN_NORTH_ORIENTATION_UP:
      return mln::NorthOrientation::Upwards;
    default:
      assert(false);
      return mln::NorthOrientation::Upwards;
  }
}

auto from_native_north_orientation(mln::NorthOrientation orientation)
  -> uint32_t {
  switch (orientation) {
    case mln::NorthOrientation::Rightwards:
      return MLN_NORTH_ORIENTATION_RIGHT;
    case mln::NorthOrientation::Downwards:
      return MLN_NORTH_ORIENTATION_DOWN;
    case mln::NorthOrientation::Leftwards:
      return MLN_NORTH_ORIENTATION_LEFT;
    case mln::NorthOrientation::Upwards:
      return MLN_NORTH_ORIENTATION_UP;
  }
  assert(false);
  return MLN_NORTH_ORIENTATION_UP;
}

auto to_native_constrain_mode(uint32_t mode) -> mln::ConstrainMode {
  switch (mode) {
    case MLN_CONSTRAIN_MODE_NONE:
      return mln::ConstrainMode::None;
    case MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT:
      return mln::ConstrainMode::WidthAndHeight;
    case MLN_CONSTRAIN_MODE_SCREEN:
      return mln::ConstrainMode::Screen;
    case MLN_CONSTRAIN_MODE_HEIGHT_ONLY:
      return mln::ConstrainMode::HeightOnly;
    default:
      assert(false);
      return mln::ConstrainMode::HeightOnly;
  }
}

auto from_native_constrain_mode(mln::ConstrainMode mode) -> uint32_t {
  switch (mode) {
    case mln::ConstrainMode::None:
      return MLN_CONSTRAIN_MODE_NONE;
    case mln::ConstrainMode::WidthAndHeight:
      return MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT;
    case mln::ConstrainMode::Screen:
      return MLN_CONSTRAIN_MODE_SCREEN;
    case mln::ConstrainMode::HeightOnly:
      return MLN_CONSTRAIN_MODE_HEIGHT_ONLY;
  }
  assert(false);
  return MLN_CONSTRAIN_MODE_HEIGHT_ONLY;
}

auto to_native_viewport_mode(uint32_t mode) -> mln::ViewportMode {
  switch (mode) {
    case MLN_VIEWPORT_MODE_FLIPPED_Y:
      return mln::ViewportMode::FlippedY;
    case MLN_VIEWPORT_MODE_DEFAULT:
      return mln::ViewportMode::Default;
    default:
      assert(false);
      return mln::ViewportMode::Default;
  }
}

auto from_native_viewport_mode(mln::ViewportMode mode) -> uint32_t {
  switch (mode) {
    case mln::ViewportMode::FlippedY:
      return MLN_VIEWPORT_MODE_FLIPPED_Y;
    case mln::ViewportMode::Default:
      return MLN_VIEWPORT_MODE_DEFAULT;
  }
  assert(false);
  return MLN_VIEWPORT_MODE_DEFAULT;
}

auto from_native_edge_insets(const mln::EdgeInsets& insets) -> mln_edge_insets {
  return mln_edge_insets{
    .top = insets.top(),
    .left = insets.left(),
    .bottom = insets.bottom(),
    .right = insets.right()
  };
}

auto to_native_tile_lod_mode(uint32_t mode) -> mln::TileLodMode {
  switch (mode) {
    case MLN_TILE_LOD_MODE_DISTANCE:
      return mln::TileLodMode::Distance;
    case MLN_TILE_LOD_MODE_DEFAULT:
      return mln::TileLodMode::Default;
    default:
      assert(false);
      return mln::TileLodMode::Default;
  }
}

auto from_native_tile_lod_mode(mln::TileLodMode mode) -> uint32_t {
  switch (mode) {
    case mln::TileLodMode::Distance:
      return MLN_TILE_LOD_MODE_DISTANCE;
    case mln::TileLodMode::Default:
      return MLN_TILE_LOD_MODE_DEFAULT;
  }
  assert(false);
  return MLN_TILE_LOD_MODE_DEFAULT;
}

auto from_native_projection_mode(const mln::ProjectionMode& mode)
  -> mln_projection_mode {
  auto result = mln::core::projection_mode_default();
  if (mode.axonometric) {
    result.fields |= MLN_PROJECTION_MODE_AXONOMETRIC;
    result.axonometric = *mode.axonometric;
  }
  if (mode.xSkew) {
    result.fields |= MLN_PROJECTION_MODE_X_SKEW;
    result.x_skew = *mode.xSkew;
  }
  if (mode.ySkew) {
    result.fields |= MLN_PROJECTION_MODE_Y_SKEW;
    result.y_skew = *mode.ySkew;
  }
  return result;
}

auto to_native_lat_lng(mln_lat_lng coordinate) -> mln::LatLng {
  return mln::LatLng{coordinate.latitude, coordinate.longitude};
}

auto from_native_lat_lng(const mln::LatLng& coordinate) -> mln_lat_lng {
  return mln_lat_lng{
    .latitude = coordinate.latitude(), .longitude = coordinate.longitude()
  };
}

auto to_native_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln::LatLngBounds {
  return mln::LatLngBounds::hull(
    to_native_lat_lng(bounds.southwest), to_native_lat_lng(bounds.northeast)
  );
}

auto from_native_lat_lng_bounds(const mln::LatLngBounds& bounds)
  -> mln_lat_lng_bounds {
  return mln_lat_lng_bounds{
    .southwest =
      mln_lat_lng{.latitude = bounds.south(), .longitude = bounds.west()},
    .northeast =
      mln_lat_lng{.latitude = bounds.north(), .longitude = bounds.east()}
  };
}

// mbgl keeps the unbounded flag private. Its operator== treats unbounded values
// as equal to each other and distinct from every bounded one, so comparing
// against a default-constructed value tests the flag exactly.
auto is_unbounded_lat_lng_bounds(const mln::LatLngBounds& bounds) -> bool {
  return bounds == mln::LatLngBounds{};
}

auto to_native_lat_lngs(const mln_lat_lng* coordinates, size_t coordinate_count)
  -> std::vector<mln::LatLng> {
  auto result = std::vector<mln::LatLng>{};
  result.reserve(coordinate_count);
  const auto coordinate_span =
    std::span<const mln_lat_lng>{coordinates, coordinate_count};
  for (const auto coordinate : coordinate_span) {
    result.emplace_back(to_native_lat_lng(coordinate));
  }
  return result;
}

auto to_native_screen_point(mln_screen_point point) -> mln::ScreenCoordinate {
  return mln::ScreenCoordinate{point.x, point.y};
}

auto from_native_screen_point(const mln::ScreenCoordinate& point)
  -> mln_screen_point {
  return mln_screen_point{.x = point.x, .y = point.y};
}

auto to_native_screen_points(const mln_screen_point* points, size_t point_count)
  -> std::vector<mln::ScreenCoordinate> {
  auto result = std::vector<mln::ScreenCoordinate>{};
  result.reserve(point_count);
  const auto point_span =
    std::span<const mln_screen_point>{points, point_count};
  for (const auto point : point_span) {
    result.emplace_back(to_native_screen_point(point));
  }
  return result;
}

auto to_native_edge_insets(mln_edge_insets padding) -> mln::EdgeInsets {
  return mln::EdgeInsets{
    padding.top, padding.left, padding.bottom, padding.right
  };
}

auto to_native_bound_options(const mln_bound_options& options)
  -> mln::BoundOptions {
  auto result = mln::BoundOptions{};
  if ((options.fields & MLN_BOUND_OPTION_BOUNDS) != 0U) {
    result.withLatLngBounds(to_native_lat_lng_bounds(options.bounds));
  }
  if ((options.fields & MLN_BOUND_OPTION_UNBOUNDED) != 0U) {
    // A default-constructed LatLngBounds is the mbgl unbounded constraint.
    result.withLatLngBounds(mln::LatLngBounds{});
  }
  if ((options.fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0U) {
    result.withMinZoom(options.min_zoom);
  }
  if ((options.fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0U) {
    result.withMaxZoom(options.max_zoom);
  }
  if ((options.fields & MLN_BOUND_OPTION_MIN_PITCH) != 0U) {
    result.withMinPitch(options.min_pitch);
  }
  if ((options.fields & MLN_BOUND_OPTION_MAX_PITCH) != 0U) {
    result.withMaxPitch(options.max_pitch);
  }
  return result;
}

auto from_native_bound_options(const mln::BoundOptions& options)
  -> mln_bound_options {
  auto result = mln::core::bound_options_default();
  if (options.bounds) {
    if (is_unbounded_lat_lng_bounds(*options.bounds)) {
      result.fields |= MLN_BOUND_OPTION_UNBOUNDED;
    } else {
      result.fields |= MLN_BOUND_OPTION_BOUNDS;
      result.bounds = from_native_lat_lng_bounds(*options.bounds);
    }
  }
  if (options.minZoom) {
    result.fields |= MLN_BOUND_OPTION_MIN_ZOOM;
    result.min_zoom = *options.minZoom;
  }
  if (options.maxZoom) {
    result.fields |= MLN_BOUND_OPTION_MAX_ZOOM;
    result.max_zoom = *options.maxZoom;
  }
  if (options.minPitch) {
    result.fields |= MLN_BOUND_OPTION_MIN_PITCH;
    result.min_pitch = *options.minPitch;
  }
  if (options.maxPitch) {
    result.fields |= MLN_BOUND_OPTION_MAX_PITCH;
    result.max_pitch = *options.maxPitch;
  }
  return result;
}

auto to_native_vec3(mln_vec3 value) -> mln::vec3 {
  return mln::vec3{{value.x, value.y, value.z}};
}

auto from_native_vec3(const mln::vec3& value) -> mln_vec3 {
  const auto [x_component, y_component, z_component] = value;
  return mln_vec3{.x = x_component, .y = y_component, .z = z_component};
}

auto to_native_vec4(mln_quaternion value) -> mln::vec4 {
  return mln::vec4{{value.x, value.y, value.z, value.w}};
}

auto from_native_vec4(const mln::vec4& value) -> mln_quaternion {
  const auto [x_component, y_component, z_component, w_component] = value;
  return mln_quaternion{
    .x = x_component, .y = y_component, .z = z_component, .w = w_component
  };
}

auto to_native_free_camera(const mln_free_camera_options& options)
  -> mln::FreeCameraOptions {
  auto result = mln::FreeCameraOptions{};
  if ((options.fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0U) {
    result.position = to_native_vec3(options.position);
  }
  if ((options.fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0U) {
    result.orientation = to_native_vec4(options.orientation);
  }
  return result;
}

auto from_native_free_camera(const mln::FreeCameraOptions& options)
  -> mln_free_camera_options {
  auto result = mln::core::free_camera_options_default();
  if (options.position) {
    result.fields |= MLN_FREE_CAMERA_OPTION_POSITION;
    result.position = from_native_vec3(*options.position);
  }
  if (options.orientation) {
    result.fields |= MLN_FREE_CAMERA_OPTION_ORIENTATION;
    result.orientation = from_native_vec4(*options.orientation);
  }
  return result;
}

auto screen_point(mln_screen_point point) -> mln::ScreenCoordinate {
  return mln::ScreenCoordinate{point.x, point.y};
}
}  // namespace

namespace mln::core {

struct MapObject {
  mln_runtime runtime = MLN_HANDLE_NULL;
  std::thread::id owner_thread;
  uint32_t map_mode = MLN_MAP_MODE_CONTINUOUS;
  double scale_factor = default_scale_factor;
  bool still_image_request_pending = false;
  // Declared first so reverse-order destruction runs the release callbacks it
  // still owes after the mln::Map that could still reach a source is gone.
  std::shared_ptr<CallbackSourceRegistry> callback_sources;
  // Declared before `observer` so reverse-order destruction retires the
  // observer, which holds its own reference, before this member is destroyed.
  std::shared_ptr<MapEventState> event_state;
  std::unique_ptr<HeadlessObserver> observer;
  std::unique_ptr<HeadlessFrontend> frontend;
  std::unique_ptr<mln::Map> map;
  // Declared after `map` so reverse-order destruction retires the command
  // channel before the mln::Map it targets.
  std::unique_ptr<MapCommands> commands;
  std::shared_ptr<mln::Mailbox> command_mailbox;
  std::optional<mln::ActorRef<MapCommands>> command_ref;
  // Guarded by the map handle table's mutex; a render session on another thread
  // clears it from map_detach_render_target_session().
  void* render_target_session = nullptr;
  FeatureStateStore feature_state;
};

template <>
struct HandleTraits<MapObject> {
  static constexpr auto kind = HandleKind::Map;
  static constexpr auto leasable = false;
};

struct MapProjectionObject {
  // Serializes projection calls with each other and with destruction, so a
  // standalone projection may be used from any thread.
  std::mutex call_mutex;
  // Null after destruction retires the handle; guarded by call_mutex.
  std::unique_ptr<mln::MapProjection> projection;
};

template <>
struct HandleTraits<MapProjectionObject> {
  static constexpr auto kind = HandleKind::MapProjection;
  static constexpr auto leasable = true;
};

}  // namespace mln::core

namespace mln::core {

namespace {

class RuntimeMapRetainGuard final {
 public:
  explicit RuntimeMapRetainGuard(mln_runtime runtime) noexcept
      : runtime_(runtime) {}

  ~RuntimeMapRetainGuard() { release_runtime_map(runtime_); }

  RuntimeMapRetainGuard(const RuntimeMapRetainGuard&) = delete;
  RuntimeMapRetainGuard(RuntimeMapRetainGuard&&) = delete;
  auto operator=(const RuntimeMapRetainGuard&)
    -> RuntimeMapRetainGuard& = delete;
  auto operator=(RuntimeMapRetainGuard&&) -> RuntimeMapRetainGuard& = delete;

  auto dismiss() noexcept -> void { runtime_ = MLN_HANDLE_NULL; }

 private:
  mln_runtime runtime_ = MLN_HANDLE_NULL;
};

// Runs on the map thread from mbgl's still-image continuation. It takes the
// handle rather than the object because the map can be destroyed between the
// request and the callback, and try_resolve leaves the thread-local diagnostic
// to the pump this fires under.
auto finish_still_image_request(mln_map map, std::exception_ptr error) -> void {
  auto* live = handle_table<MapObject>().try_resolve(map);
  if (live == nullptr) {
    return;
  }
  // Clearing the pending flag is map state that the next request reads, so it
  // happens whatever the mask selects.
  live->still_image_request_pending = false;
  if (error) {
    if (!event_selected(
          live->event_state->mask, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED
        )) {
      return;
    }
    const auto message = exception_message(error);
    push_runtime_map_event(
      live->runtime, map, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED, 0,
      message.c_str()
    );
    return;
  }

  if (!event_selected(
        live->event_state->mask, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED
      )) {
    return;
  }
  push_runtime_map_event(
    live->runtime, map, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED
  );
}

// The caller holds the map handle table's mutex, so it can act on the result
// without the handle being retired in between.
auto validate_map_live_locked(mln_map map, MapObject*& out_map) -> mln_status {
  out_map = handle_table<MapObject>().resolve_locked(map);
  return out_map == nullptr ? MLN_STATUS_INVALID_ARGUMENT : MLN_STATUS_OK;
}

// Same locking contract as validate_map_live_locked().
auto validate_map_locked(mln_map map, MapObject*& out_map) -> mln_status {
  const auto status = validate_map_live_locked(map, out_map);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_map->owner_thread != std::this_thread::get_id()) {
    set_thread_error("map call must be made on its owner thread");
    return MLN_STATUS_WRONG_THREAD;
  }
  return MLN_STATUS_OK;
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

}  // namespace

auto map_options_default() noexcept -> mln_map_options {
  return mln_map_options{
    .size = sizeof(mln_map_options),
    .width = default_map_width,
    .height = default_map_height,
    .scale_factor = default_scale_factor,
    .map_mode = MLN_MAP_MODE_CONTINUOUS,
    .fast_pfor_enabled = false,
    .event_mask = MLN_RUNTIME_EVENT_MASK_ALL
  };
}

auto camera_options_default() noexcept -> mln_camera_options {
  return mln_camera_options{
    .size = sizeof(mln_camera_options),
    .fields = 0,
    .latitude = 0,
    .longitude = 0,
    .center_altitude = 0,
    .padding = {.top = 0, .left = 0, .bottom = 0, .right = 0},
    .anchor = {.x = 0, .y = 0},
    .zoom = 0,
    .bearing = 0,
    .pitch = 0,
    .roll = 0,
    .field_of_view = 0
  };
}

auto animation_options_default() noexcept -> mln_animation_options {
  return mln_animation_options{
    .size = sizeof(mln_animation_options),
    .fields = 0,
    .duration_ms = 0,
    .velocity = 0,
    .min_zoom = 0,
    .easing = {.x1 = 0, .y1 = 0, .x2 = 0.25, .y2 = 1},
    .transition_id = 0
  };
}

auto camera_fit_options_default() noexcept -> mln_camera_fit_options {
  return mln_camera_fit_options{
    .size = sizeof(mln_camera_fit_options),
    .fields = 0,
    .padding = {.top = 0, .left = 0, .bottom = 0, .right = 0},
    .bearing = 0,
    .pitch = 0
  };
}

auto bound_options_default() noexcept -> mln_bound_options {
  return mln_bound_options{
    .size = sizeof(mln_bound_options),
    .fields = 0,
    .bounds =
      {.southwest = {.latitude = 0, .longitude = 0},
       .northeast = {.latitude = 0, .longitude = 0}},
    .min_zoom = 0,
    .max_zoom = 0,
    .min_pitch = 0,
    .max_pitch = 0
  };
}

auto free_camera_options_default() noexcept -> mln_free_camera_options {
  return mln_free_camera_options{
    .size = sizeof(mln_free_camera_options),
    .fields = 0,
    .position = {.x = 0, .y = 0, .z = 0},
    .orientation = {.x = 0, .y = 0, .z = 0, .w = 1}
  };
}

auto projection_mode_default() noexcept -> mln_projection_mode {
  return mln_projection_mode{
    .size = sizeof(mln_projection_mode),
    .fields = 0,
    .axonometric = false,
    .x_skew = 0,
    .y_skew = 0
  };
}

auto map_viewport_options_default() noexcept -> mln_map_viewport_options {
  return mln_map_viewport_options{
    .size = sizeof(mln_map_viewport_options),
    .fields = 0,
    .north_orientation = MLN_NORTH_ORIENTATION_UP,
    .constrain_mode = MLN_CONSTRAIN_MODE_HEIGHT_ONLY,
    .viewport_mode = MLN_VIEWPORT_MODE_DEFAULT,
    .frustum_offset = {.top = 0, .left = 0, .bottom = 0, .right = 0}
  };
}

auto map_tile_options_default() noexcept -> mln_map_tile_options {
  return mln_map_tile_options{
    .size = sizeof(mln_map_tile_options),
    .fields = 0,
    .prefetch_zoom_delta = 0,
    .lod_min_radius = 0,
    .lod_scale = 0,
    .lod_pitch_threshold = 0,
    .lod_zoom_shift = 0,
    .lod_mode = MLN_TILE_LOD_MODE_DEFAULT
  };
}

auto style_tile_source_options_default() noexcept
  -> mln_style_tile_source_options {
  return mln_style_tile_source_options{
    .size = sizeof(mln_style_tile_source_options),
    .fields = 0,
    .min_zoom = 0,
    .max_zoom = mln::util::DEFAULT_MAX_ZOOM,
    .attribution = {.data = nullptr, .size = 0},
    .scheme = MLN_STYLE_TILE_SCHEME_XYZ,
    .bounds =
      {.southwest = {.latitude = 0, .longitude = 0},
       .northeast = {.latitude = 0, .longitude = 0}},
    .tile_size = mln::util::tileSize_I,
    .vector_encoding = MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
    .raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX
  };
}

auto geojson_source_options_default() noexcept -> mln_geojson_source_options {
  const auto defaults = mln::style::GeoJSONOptions{};
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
    .synchronous_tiling = defaults.synchronousUpdate
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
    .tile_size = mln::util::tileSize_I,
    .buffer = 128,
    .clip = false,
    .wrap = false,
    .release_user_data = nullptr
  };
}

auto custom_mvt_vector_source_options_default() noexcept
  -> mln_custom_mvt_vector_source_options {
  return mln_custom_mvt_vector_source_options{
    .size = sizeof(mln_custom_mvt_vector_source_options),
    .fields = 0,
    .fetch_tile = nullptr,
    .cancel_tile = nullptr,
    .user_data = nullptr,
    .min_zoom = 0,
    .max_zoom = 18,
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

auto validate_map_live(mln_map map, MapObject*& out_map) -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  return validate_map_live_locked(map, out_map);
}

// Only the owner thread destroys a map, so the borrowed object stays alive for
// as long as the calling thread can use it.
auto validate_map(mln_map map, MapObject*& out_map) -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  return validate_map_locked(map, out_map);
}

template <typename Work>
auto with_map_projection(mln_map_projection handle, Work&& work) -> mln_status {
  auto live = handle_table<MapProjectionObject>().lease(handle);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock lock(live->call_mutex);
  if (live->projection == nullptr) {
    set_handle_fault_error(
      HandleTraits<MapProjectionObject>::kind, handle, HandleFault::Stale
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return std::invoke(std::forward<Work>(work), *live->projection);
}

auto create_map(
  mln_runtime runtime, const mln_map_options* options, mln_map* out_map
) -> mln_status {
  const auto options_status = validate_map_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  if (out_map == nullptr) {
    set_thread_error("out_map must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_map != MLN_HANDLE_NULL) {
    set_thread_error("out_map must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  RuntimeObject* live_runtime = nullptr;
  const auto runtime_status = validate_runtime(runtime, live_runtime);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }

  const auto retain_status = retain_runtime_map(runtime);
  if (retain_status != MLN_STATUS_OK) {
    return retain_status;
  }
  auto retain_guard = RuntimeMapRetainGuard{runtime};

  const auto effective = options == nullptr ? map_options_default() : *options;
  auto owned_map = std::make_shared<MapObject>();
  // Every allocation this function owns happens before the handle is published,
  // so a throw here cannot leave a registered map the caller has no handle to
  // destroy.
  auto event_state = std::make_shared<MapEventState>();
  auto source_registry = std::make_shared<CallbackSourceRegistry>();
  // Publish the handle first, so the observer and frontend capture an id that
  // already resolves.
  const auto handle = handle_table<MapObject>().insert(owned_map);
  owned_map->runtime = runtime;
  owned_map->owner_thread = std::this_thread::get_id();
  owned_map->map_mode = effective.map_mode;
  owned_map->scale_factor = effective.scale_factor;
  owned_map->event_state = std::move(event_state);
  owned_map->callback_sources = std::move(source_registry);
  owned_map->event_state->mask.store(
    effective.event_mask, std::memory_order_relaxed
  );
  try {
    // Registering allocates, so it belongs inside the scope that unpublishes
    // the handle on failure. Nothing before this point queues an event, and the
    // observer and frontend below are the first producers that need it.
    register_runtime_map_events(runtime, handle);
    owned_map->observer = std::make_unique<HeadlessObserver>(
      runtime, handle, owned_map->event_state, owned_map->callback_sources
    );
    owned_map->frontend = std::make_unique<HeadlessFrontend>(
      runtime, handle, runtime_run_loop(live_runtime), owned_map->event_state
    );

    auto map_options = mln::MapOptions{};
    map_options.withMapMode(to_native_map_mode(effective.map_mode))
      .withSize(mln::Size{effective.width, effective.height})
      .withPixelRatio(static_cast<float>(effective.scale_factor))
      .withFastPFOREnabled(effective.fast_pfor_enabled);
    owned_map->map = std::make_unique<mln::Map>(
      *owned_map->frontend, *owned_map->observer, map_options,
      resource_options_for_runtime(runtime)
    );
    owned_map->callback_sources->attach(*owned_map->map);

    owned_map->commands = std::make_unique<MapCommands>(*owned_map->map);
    owned_map->command_mailbox =
      std::make_shared<mln::Mailbox>(runtime_run_loop(live_runtime));
    owned_map->command_ref.emplace(
      *owned_map->commands, owned_map->command_mailbox
    );

  } catch (...) {
    static_cast<void>(handle_table<MapObject>().remove(handle));
    discard_runtime_map_events(runtime, handle);
    throw;
  }
  *out_map = handle;
  retain_guard.dismiss();
  return MLN_STATUS_OK;
}

auto destroy_map(mln_map map) -> mln_status {
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  auto owned_map = std::shared_ptr<MapObject>{};
  {
    // One critical section covers validation, the render-session check, and
    // taking ownership, because a render session on another thread detaches
    // under this same lock.
    auto& table = handle_table<MapObject>();
    const std::scoped_lock lock(table.mutex());
    MapObject* live = nullptr;
    const auto status = validate_map_locked(map, live);
    if (status != MLN_STATUS_OK) {
      return status;
    }
    if (live->render_target_session != nullptr) {
      set_thread_error("map still has an attached render session");
      return MLN_STATUS_INVALID_STATE;
    }
    runtime = live->runtime;
    owned_map = table.remove_locked(map);
  }
  // Both cross-thread channels close before the map is destroyed. The registry
  // clears its map at the same point, so nothing reconciles against a map
  // that is being destroyed; the releases it still owes run from its own
  // destructor once the map is gone.
  owned_map->callback_sources->detach();
  owned_map->frontend->close_renderer_observer();
  owned_map->command_mailbox->close();
  // Runs outside the registry lock: it can block on in-flight background work.
  owned_map->frontend->shutdown_thread_pool();
  discard_runtime_map_events(runtime, map);
  owned_map.reset();
  release_runtime_map(runtime);
  return MLN_STATUS_OK;
}

auto map_request_repaint(mln_map map) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  if (live->map_mode != MLN_MAP_MODE_CONTINUOUS) {
    set_thread_error("map is not in continuous mode");
    return MLN_STATUS_INVALID_STATE;
  }

  live->map->triggerRepaint();
  return MLN_STATUS_OK;
}

auto map_set_feature_state(
  mln_map map, const mln_feature_state_selector* selector, mln_buffer_view state
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }

  auto native_state = to_native_json_value(state);
  if (!native_state) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto* state_object = native_state->getObject();
  if (state_object == nullptr) {
    set_thread_error("feature state value must be a JSON object");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  live->feature_state.set(
    string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    string_from_view(selector->feature_id), *state_object
  );
  live->map->triggerRepaint();
  return MLN_STATUS_OK;
}

auto map_get_feature_state(
  mln_map map, const mln_feature_state_selector* selector, mln_buffer* out_state
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }

  auto state = live->feature_state.get(
    string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    string_from_view(selector->feature_id)
  );
  return create_buffer(
    serialize_json_value(mln::Value{std::move(state)}), out_state
  );
}

auto map_remove_feature_state(
  mln_map map, const mln_feature_state_selector* selector
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, false);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }

  live->feature_state.remove(
    string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, selector->feature_id
    ),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_STATE_KEY, selector->state_key
    )
  );
  live->map->triggerRepaint();
  return MLN_STATUS_OK;
}

auto map_request_still_image(mln_map map) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  if (!is_still_map_mode(live->map_mode)) {
    set_thread_error("map is not in static or tile mode");
    return MLN_STATUS_INVALID_STATE;
  }

  if (live->still_image_request_pending) {
    set_thread_error("map already has a pending still-image request");
    return MLN_STATUS_INVALID_STATE;
  }

  live->still_image_request_pending = true;
  live->map->renderStill([map](std::exception_ptr error) -> void {
    finish_still_image_request(map, error);
  });
  return MLN_STATUS_OK;
}

// The render-facing helpers below run on the session's thread while the map
// lives on its own. They are reachable only through an attached session, and
// destroy_map() returns MLN_STATUS_INVALID_STATE while a session is attached,
// so the map cannot be retired underneath them.
auto map_scale_factor(mln_map map) -> double {
  const auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? default_scale_factor : live->scale_factor;
}

// Map-thread only. The render path posts through map_post_set_size() and
// map_post_trigger_repaint() instead.
auto map_native(MapObject* map) -> mln::Map* { return map->map.get(); }

// Both posting helpers hold the map table's mutex across the liveness check and
// the send, so the map cannot be retired in between. Mailbox::push takes only
// its own mutex and the run loop's, so there is no path back to this lock.
auto map_post_set_size(mln_map map, uint32_t width, uint32_t height)
  -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  MapObject* live = nullptr;
  const auto status = validate_map_live_locked(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->command_ref->invoke(&MapCommands::set_size, width, height);
  return MLN_STATUS_OK;
}

auto map_post_trigger_repaint(mln_map map) -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  MapObject* live = nullptr;
  const auto status = validate_map_live_locked(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->command_ref->invoke(&MapCommands::trigger_repaint);
  return MLN_STATUS_OK;
}

auto map_latest_update(mln_map map) -> std::shared_ptr<mln::UpdateParameters> {
  auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? nullptr : live->frontend->latest_update();
}

auto map_feature_state_snapshot(mln_map map)
  -> std::shared_ptr<const FeatureStateSnapshot> {
  auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? std::make_shared<FeatureStateSnapshot>()
                         : live->feature_state.snapshot();
}

auto map_renderer_observer(mln_map map) -> mln::RendererObserver* {
  auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? nullptr : live->frontend->renderer_observer();
}

auto map_run_render_jobs(mln_map map) -> void {
  if (
    auto* live = handle_table<MapObject>().try_resolve(map); live != nullptr
  ) {
    live->frontend->run_render_jobs();
  }
}

// Claims the map's single render-session slot. Runs on the render session's own
// thread, so it validates liveness only, and holds the map handle table's mutex
// across the check and the claim to stay race-free against destroy_map().
auto map_attach_render_target_session(mln_map map, void* session)
  -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  MapObject* live = nullptr;
  const auto status = validate_map_live_locked(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (session == nullptr) {
    set_thread_error("render session must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (live->render_target_session != nullptr) {
    set_thread_error("map already has an attached render session");
    return MLN_STATUS_INVALID_STATE;
  }
  live->render_target_session = session;
  return MLN_STATUS_OK;
}

// Runs on the render session's owner thread, so it validates liveness only, and
// holds the map handle table's mutex across the check and the clear to stay
// race-free against destroy_map().
auto map_detach_render_target_session(mln_map map, void* session)
  -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  MapObject* live = nullptr;
  const auto status = validate_map_live_locked(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (session == nullptr) {
    set_thread_error("render session must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (live->render_target_session != session) {
    set_thread_error("render session is not attached to this map");
    return MLN_STATUS_INVALID_STATE;
  }
  live->render_target_session = nullptr;
  return MLN_STATUS_OK;
}

auto map_set_style_url(mln_map map, const char* url) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (url == nullptr) {
    set_thread_error("url must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // A style that fails to parse inside this call reaches
  // HeadlessObserver::onDidFailLoadingMap on this stack, so the flag below is
  // owner-thread state that needs no lock and no queued event.
  live->event_state->style_load_failed = false;
  live->event_state->style_load_failure.clear();
  live->map->getStyle().loadURL(url);
  if (live->event_state->style_load_failed) {
    set_thread_error(live->event_state->style_load_failure.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto map_set_style_json(mln_map map, mln_buffer_view json) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_bytes(json, "style JSON")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  try {
    live->event_state->style_load_failed = false;
    live->event_state->style_load_failure.clear();
    live->map->getStyle().loadJSON(
      std::string{reinterpret_cast<const char*>(json.data), json.size}
    );
  } catch (const std::exception& exception) {
    // The diagnostic is this call's own status text, so it is set whatever the
    // mask selects; only the event is gated.
    set_thread_error(exception.what());
    if (
      event_selected(
        live->event_state->mask, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED
      )
    ) {
      push_runtime_map_event(
        live->runtime, map, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, 0,
        exception.what()
      );
    }
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (live->event_state->style_load_failed) {
    set_thread_error(live->event_state->style_load_failure.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

// Reports the document the style loader last parsed, not the live style, so
// runtime style mutations never reach it.
auto map_copy_loaded_style_json(
  mln_map map, uint8_t* out_json, size_t json_capacity, size_t* out_json_size
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return copy_text(
    live->map->getStyle().getJSON(), reinterpret_cast<char*>(out_json),
    json_capacity, out_json_size, "json_capacity"
  );
}

// Reports live state: MapLibre records the style URL when the request is made
// and clears it when a JSON style replaces it.
auto map_copy_style_url(
  mln_map map, char* out_url, size_t url_capacity, size_t* out_url_size
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return copy_text(
    live->map->getStyle().getURL(), out_url, url_capacity, out_url_size,
    "url_capacity"
  );
}

auto map_set_event_mask(mln_map map, uint64_t mask) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if ((mask & ~static_cast<uint64_t>(MLN_RUNTIME_EVENT_MASK_ALL)) != 0U) {
    set_thread_error("mask contains unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // The whole value is stored, including the runtime-event bits this map's
  // producers never test, so a getter reports what a host wrote.
  live->event_state->mask.store(mask, std::memory_order_relaxed);
  return MLN_STATUS_OK;
}

auto map_get_event_mask(mln_map map, uint64_t* out_mask) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_mask == nullptr) {
    set_thread_error("out_mask must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_mask = live->event_state->mask.load(std::memory_order_relaxed);
  return MLN_STATUS_OK;
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

  auto& style = live->map->getStyle();
  const auto id = string_from_view(source_id);
  if (style.getSource(id) != nullptr) {
    set_thread_error("source already exists");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto error = mln::style::conversion::Error{};
  auto source =
    mln::style::conversion::convertJSON<std::unique_ptr<mln::style::Source>>(
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

  auto& style = live->map->getStyle();
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
  live->callback_sources->release(id);
  *out_removed = true;
  return MLN_STATUS_OK;
}

auto map_style_source_exists(
  mln_map map, mln_buffer_view source_id, bool* out_exists
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
  if (out_exists == nullptr) {
    set_thread_error("out_exists must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_exists =
    live->map->getStyle().getSource(string_from_view(source_id)) != nullptr;
  return MLN_STATUS_OK;
}

auto map_get_style_source_type(
  mln_map map, mln_buffer_view source_id, uint32_t* out_source_type,
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
  if (out_source_type == nullptr || out_found == nullptr) {
    set_thread_error("out_source_type and out_found must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto* source =
    live->map->getStyle().getSource(string_from_view(source_id));
  *out_found = source != nullptr;
  *out_source_type = MLN_STYLE_SOURCE_TYPE_UNKNOWN;
  if (source != nullptr) {
    *out_source_type = to_c_source_type(source->getType());
  }
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
    live->map->getStyle().getSource(string_from_view(source_id));
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
  if (const auto* vector_source = source->as<mln::style::VectorSource>()) {
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

auto map_set_style_source_volatile(
  mln_map map, mln_buffer_view source_id, bool is_volatile
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  source->setVolatile(is_volatile);
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
    live->map->getStyle().getSource(string_from_view(source_id));
  *out_found = source != nullptr;
  *out_attribution_size = 0;
  if (source == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto attribution = source->getAttribution();
  if (!attribution) {
    return MLN_STATUS_OK;
  }
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
    live->map->getStyle().getSource(string_from_view(source_id));
  *out_found = source != nullptr;
  if (source == nullptr) {
    *out_url_size = 0;
    return MLN_STATUS_OK;
  }

  return copy_text(
    source_url(*source).value_or(std::string{}), out_url, url_capacity,
    out_url_size, "url_capacity"
  );
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
    live->map->getStyle().getSource(string_from_view(source_id));
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
  for (const auto* source : live->map->getStyle().getSources()) {
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

  auto& style = live->map->getStyle();
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

  auto source =
    std::make_unique<mln::style::GeoJSONSource>(id, std::move(*native_options));
  source->setURL(string_from_view(url));
  style.addSource(std::move(source));
  return MLN_STATUS_OK;
}

auto map_add_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data
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

  // A lease keeps the prepared data alive across a concurrent destroy from
  // another thread, which the data handle's contract allows.
  const auto prepared = geojson_source_data_table().lease(data);
  if (prepared == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& style = live->map->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto source =
    std::make_unique<mln::style::GeoJSONSource>(id, prepared->options);
  source->setGeoJSONData(prepared->data);
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* geojson_source = source->as<mln::style::GeoJSONSource>();
  if (geojson_source == nullptr) {
    set_thread_error("source is not a GeoJSON source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  geojson_source->setURL(string_from_view(url));
  return MLN_STATUS_OK;
}

auto map_set_geojson_source_data(
  mln_map map, mln_buffer_view source_id, mln_geojson_source_data data
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

  // A lease keeps the prepared data alive across a concurrent destroy from
  // another thread, which the data handle's contract allows.
  const auto prepared = geojson_source_data_table().lease(data);
  if (prepared == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* geojson_source = source->as<mln::style::GeoJSONSource>();
  if (geojson_source == nullptr) {
    set_thread_error("source is not a GeoJSON source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!geojson_source_options_equal(
        geojson_source->getOptions(), *prepared->options
      )) {
    const auto message =
      "prepared GeoJSON data options do not match the options that GeoJSON "
      "source \"" +
      string_from_view(source_id) + "\" was added with";
    set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  geojson_source->setGeoJSONData(prepared->data);
  return MLN_STATUS_OK;
}

auto map_set_geojson_source_synchronous_tiling(
  mln_map map, mln_buffer_view source_id, bool enabled
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* geojson_source = source->as<mln::style::GeoJSONSource>();
  if (geojson_source == nullptr) {
    set_thread_error("source is not a GeoJSON source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  geojson_source->setOverrideSynchronousUpdate(enabled);
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

  auto& style = live->map->getStyle();
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
      std::make_unique<mln::style::VectorSource>(
        id, string_from_view(url), max_zoom, min_zoom,
        to_native_vector_encoding(effective.vector_encoding)
      )
    );
  } else {
    style.addSource(
      std::make_unique<mln::style::VectorSource>(
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

  auto& style = live->map->getStyle();
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
    std::make_unique<mln::style::VectorSource>(
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

  auto& style = live->map->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  style.addSource(
    std::make_unique<mln::style::RasterSource>(
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

  auto& style = live->map->getStyle();
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
    std::make_unique<mln::style::RasterSource>(
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

  auto& style = live->map->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_tile_source_options(options);
  auto source_options = std::optional<mln::style::SourceOptions>{};
  if (
    has_tile_source_option(
      effective, MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
    )
  ) {
    source_options = mln::style::SourceOptions{
      .rasterEncoding = to_native_raster_encoding(effective.raster_encoding)
    };
  }
  style.addSource(
    std::make_unique<mln::style::RasterDEMSource>(
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

  auto& style = live->map->getStyle();
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
    std::make_unique<mln::style::RasterDEMSource>(
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

  auto& style = live->map->getStyle();
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
  live->callback_sources->add(
    id, CallbackSourceKind::CustomGeometry, effective.release_user_data,
    effective.user_data
  );
  try {
    style.addSource(
      std::make_unique<mln::style::CustomGeometrySource>(
        id, to_native_custom_geometry_source_options(effective)
      )
    );
  } catch (...) {
    // Mirrors add()'s own early return: a source with no release callback was
    // never tracked, so there is nothing to untrack and no entry of another
    // source's to erase.
    if (effective.release_user_data != nullptr) {
      live->callback_sources->untrack(id);
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* custom_source = source->as<mln::style::CustomGeometrySource>();
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* custom_source = source->as<mln::style::CustomGeometrySource>();
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* custom_source = source->as<mln::style::CustomGeometrySource>();
  if (custom_source == nullptr) {
    set_thread_error("source is not a custom geometry source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  custom_source->invalidateRegion(to_native_lat_lng_bounds(bounds));
  return MLN_STATUS_OK;
}

auto lookup_custom_mvt_vector_source(
  MapObject* live, mln_buffer_view source_id,
  mln::style::CustomVectorSource*& out_source
) -> mln_status {
  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_source = source->as<mln::style::CustomVectorSource>();
  if (out_source == nullptr) {
    set_thread_error("source is not a custom MVT vector source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto map_add_custom_mvt_vector_source(
  mln_map map, mln_buffer_view source_id,
  const mln_custom_mvt_vector_source_options* options
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
  const auto options_status =
    validate_custom_mvt_vector_source_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto& style = live->map->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_custom_mvt_vector_source_options(*options);
  live->callback_sources->add(
    id, CallbackSourceKind::CustomMvtVector, effective.release_user_data,
    effective.user_data
  );
  try {
    style.addSource(
      std::make_unique<mln::style::CustomVectorSource>(
        id, to_native_custom_mvt_vector_source_options(effective)
      )
    );
  } catch (...) {
    if (effective.release_user_data != nullptr) {
      live->callback_sources->untrack(id);
    }
    throw;
  }
  return MLN_STATUS_OK;
}

auto map_set_custom_mvt_vector_source_tile_data(
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
  if (!validate_string_view(data, "data")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  mln::style::CustomVectorSource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_mvt_vector_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }

  auto native_data = std::shared_ptr<const std::string>{};
  if (data.size != 0) {
    native_data = std::make_shared<const std::string>(
      static_cast<const char*>(data.data), data.size
    );
  }
  custom_source->setTileData(
    to_native_canonical_tile_id(tile_id), native_data,
    mln::style::TileDataFormat::MVT
  );
  return MLN_STATUS_OK;
}

auto map_set_custom_mvt_vector_source_tile_error(
  mln_map map, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message
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
  if (!validate_string_view(message, "message")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  mln::style::CustomVectorSource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_mvt_vector_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }

  custom_source->setTileError(
    to_native_canonical_tile_id(tile_id),
    std::make_exception_ptr(std::runtime_error(string_from_view(message)))
  );
  return MLN_STATUS_OK;
}

auto map_invalidate_custom_mvt_vector_source_tile(
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

  mln::style::CustomVectorSource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_mvt_vector_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }
  custom_source->invalidateTile(to_native_canonical_tile_id(tile_id));
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
  auto content = std::optional<mln::style::ImageContent>{};
  if (
    options != nullptr &&
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_CONTENT)
  ) {
    content = mln::style::ImageContent{
      .left = effective.content.left,
      .top = effective.content.top,
      .right = effective.content.right,
      .bottom = effective.content.bottom
    };
  }
  auto text_fit_width = std::optional<mln::style::TextFit>{};
  if (
    options != nullptr &&
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH)
  ) {
    text_fit_width = to_native_text_fit(effective.text_fit_width);
  }
  auto text_fit_height = std::optional<mln::style::TextFit>{};
  if (
    options != nullptr &&
    has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT)
  ) {
    text_fit_height = to_native_text_fit(effective.text_fit_height);
  }

  auto style_image = std::make_unique<mln::style::Image>(
    string_from_view(image_id), to_native_premultiplied_rgba8_image(*image),
    effective.pixel_ratio, effective.sdf,
    to_native_image_stretches(effective.stretch_x, effective.stretch_x_count),
    to_native_image_stretches(effective.stretch_y, effective.stretch_y_count),
    content, text_fit_width, text_fit_height
  );
  live->map->getStyle().addImage(std::move(style_image));
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

  auto& style = live->map->getStyle();
  const auto id = string_from_view(image_id);
  *out_removed = style.getImage(id).has_value();
  if (*out_removed) {
    style.removeImage(id);
  }
  return MLN_STATUS_OK;
}

auto map_style_image_exists(
  mln_map map, mln_buffer_view image_id, bool* out_exists
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
  if (out_exists == nullptr) {
    set_thread_error("out_exists must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_exists =
    live->map->getStyle().getImage(string_from_view(image_id)).has_value();
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

  const auto image = live->map->getStyle().getImage(string_from_view(image_id));
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

  const auto image = live->map->getStyle().getImage(string_from_view(image_id));
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

  const auto image = live->map->getStyle().getImage(string_from_view(image_id));
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

  auto& style = live->map->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto source = std::make_unique<mln::style::ImageSource>(
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

  auto& style = live->map->getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto native_image = to_native_premultiplied_rgba8_image(*image);
  style.addSource(
    std::make_unique<mln::style::ImageSource>(
      id, to_native_image_source_coordinates(coordinates)
    )
  );
  auto* added_source = style.getSource(id);
  auto* image_source = added_source == nullptr
                         ? nullptr
                         : added_source->as<mln::style::ImageSource>();
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* image_source = source->as<mln::style::ImageSource>();
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* image_source = source->as<mln::style::ImageSource>();
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    set_thread_error("source does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* image_source = source->as<mln::style::ImageSource>();
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

  auto* source = live->map->getStyle().getSource(string_from_view(source_id));
  *out_found = source != nullptr;
  *out_coordinate_count = 0;
  if (source == nullptr) {
    return MLN_STATUS_OK;
  }
  auto* image_source = source->as<mln::style::ImageSource>();
  if (image_source == nullptr) {
    set_thread_error("source is not an image source");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto image_source_coordinate_count = size_t{4};
  *out_coordinate_count = image_source_coordinate_count;
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

  auto& style = live->map->getStyle();
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
  if (!source_ptr->is<mln::style::RasterDEMSource>()) {
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
    std::make_unique<mln::style::HillshadeLayer>(layer, source), before
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

  auto& style = live->map->getStyle();
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
  if (!source_ptr->is<mln::style::RasterDEMSource>()) {
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
    std::make_unique<mln::style::ColorReliefLayer>(layer, source), before
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

  auto& style = live->map->getStyle();
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
    std::make_unique<mln::style::LocationIndicatorLayer>(layer), before
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
  const auto* layer = map->map->getStyle().getLayer(string_from_view(layer_id));
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
    mln::Value{mapbox::base::ValueArray{
      mln::Value{coordinate.latitude}, mln::Value{coordinate.longitude},
      mln::Value{altitude}
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
  const auto value = serialize_json_value(mln::Value{bearing});
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
  const auto value = serialize_json_value(mln::Value{radius});
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
    serialize_json_value(mln::Value{string_from_view(image_id)});
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

  auto& style = live->map->getStyle();
  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (style.getLayer(*before) == nullptr) {
      set_thread_error("before_layer_id does not exist");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }

  auto error = mln::style::conversion::Error{};
  auto layer =
    mln::style::conversion::convertJSON<std::unique_ptr<mln::style::Layer>>(
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
      mln::style::LayerTypeInfo::Source::Required &&
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

  auto removed = live->map->getStyle().removeLayer(string_from_view(layer_id));
  *out_removed = removed != nullptr;
  return MLN_STATUS_OK;
}

auto map_style_layer_exists(
  mln_map map, mln_buffer_view layer_id, bool* out_exists
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
  if (out_exists == nullptr) {
    set_thread_error("out_exists must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_exists =
    live->map->getStyle().getLayer(string_from_view(layer_id)) != nullptr;
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
    live->map->getStyle().getLayer(string_from_view(layer_id));
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
  for (const auto* layer : live->map->getStyle().getLayers()) {
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

  auto& style = live->map->getStyle();
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
    live->map->getStyle().getLayer(string_from_view(layer_id));
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

  auto error = mln::style::conversion::Error{};
  auto light = mln::style::conversion::convertJSON<mln::style::Light>(
    string_from_view(light_json), error
  );
  if (!light) {
    set_style_conversion_error("style light", error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  live->map->getStyle().setLight(std::make_unique<mln::style::Light>(*light));
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
  auto document = mln::JSDocument{};
  if (!parse_json_document(value, "style light property", document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* light = live->map->getStyle().getLight();
  if (light == nullptr) {
    set_thread_error("style light does not exist");
    return MLN_STATUS_INVALID_STATE;
  }

  auto error = light->setProperty(
    string_from_view(property_name),
    mln::style::conversion::Convertible{
      static_cast<const mln::JSValue*>(&document)
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

  auto* light = live->map->getStyle().getLight();
  if (light == nullptr) {
    set_thread_error("style light does not exist");
    return MLN_STATUS_INVALID_STATE;
  }

  const auto property = light->getProperty(string_from_view(property_name));
  if (property.getKind() == mln::style::StyleProperty::Kind::Undefined) {
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
  auto native = mln::style::TransitionOptions{};
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

  live->map->getStyle().setTransitionOptions(native);
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

  const auto native = live->map->getStyle().getTransitionOptions();
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

  auto document = mln::JSDocument{};
  if (!parse_json_document(value, "layer property", document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* layer = live->map->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto error = layer->setProperty(
    string_from_view(property_name),
    mln::style::conversion::Convertible{
      static_cast<const mln::JSValue*>(&document)
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

  auto* layer = live->map->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto property = layer->getProperty(string_from_view(property_name));
  if (property.getKind() == mln::style::StyleProperty::Kind::Undefined) {
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

  auto* layer = live->map->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (filter == nullptr) {
    layer->setFilter(mln::style::Filter{});
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

  auto* layer = live->map->getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    set_thread_error("layer does not exist");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto filter = layer->getFilter().serialize();
  if (filter.is<mln::NullValue>()) {
    return MLN_STATUS_OK;
  }
  return create_buffer(serialize_json_value(filter), out_filter);
}

namespace {

auto resolve_layer_for_access(
  mln_map map, mln_buffer_view layer_id, mln::style::Layer*& out_layer
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

  auto* layer = live->map->getStyle().getLayer(string_from_view(layer_id));
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
  const mln::style::Layer& layer, const char* field
) -> bool {
  if (
    layer.getTypeInfo()->source == mln::style::LayerTypeInfo::Source::Required
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
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
  mln::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  auto native_visibility = mln::style::VisibilityType::Visible;
  switch (visibility) {
    case MLN_STYLE_LAYER_VISIBILITY_VISIBLE:
      native_visibility = mln::style::VisibilityType::Visible;
      break;
    case MLN_STYLE_LAYER_VISIBILITY_NONE:
      native_visibility = mln::style::VisibilityType::None;
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
  mln::style::Layer* layer = nullptr;
  const auto status = resolve_layer_for_access(map, layer_id, layer);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_visibility == nullptr) {
    set_thread_error("out_visibility must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_visibility = layer->getVisibility() == mln::style::VisibilityType::None
                      ? MLN_STYLE_LAYER_VISIBILITY_NONE
                      : MLN_STYLE_LAYER_VISIBILITY_VISIBLE;
  return MLN_STATUS_OK;
}

auto map_get_camera(mln_map map, mln_camera_options* out_camera) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_camera == nullptr || out_camera->size < sizeof(mln_camera_options)) {
    set_thread_error("out_camera must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_camera = from_native_camera(live->map->getCameraOptions());
  return MLN_STATUS_OK;
}

auto map_jump_to(mln_map map, const mln_camera_options* camera) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  live->map->jumpTo(to_native_camera(*camera));
  return MLN_STATUS_OK;
}

auto map_ease_to(
  mln_map map, const mln_camera_options* camera,
  const mln_animation_options* animation
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  const auto animation_status = validate_animation_options(animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }

  live->map->easeTo(
    to_native_camera(*camera),
    to_native_animation(live->runtime, map, live->event_state, animation)
  );
  return MLN_STATUS_OK;
}

auto map_fly_to(
  mln_map map, const mln_camera_options* camera,
  const mln_animation_options* animation
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  const auto animation_status = validate_animation_options(animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }

  live->map->flyTo(
    to_native_camera(*camera),
    to_native_animation(live->runtime, map, live->event_state, animation)
  );
  return MLN_STATUS_OK;
}

auto map_get_projection_mode(mln_map map, mln_projection_mode* out_mode)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_mode == nullptr || out_mode->size < sizeof(mln_projection_mode)) {
    set_thread_error("out_mode must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_mode = from_native_projection_mode(live->map->getProjectionMode());
  return MLN_STATUS_OK;
}

auto map_set_projection_mode(mln_map map, const mln_projection_mode* mode)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto mode_status = validate_projection_mode_options(mode);
  if (mode_status != MLN_STATUS_OK) {
    return mode_status;
  }

  live->map->setProjectionMode(to_native_projection_mode(*mode));
  return MLN_STATUS_OK;
}

auto map_set_debug_options(mln_map map, uint32_t options) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_debug_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  live->map->setDebug(to_native_debug_options(options));
  return MLN_STATUS_OK;
}

auto map_get_debug_options(mln_map map, uint32_t* out_options) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_options == nullptr) {
    set_thread_error("out_options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_options = from_native_debug_options(live->map->getDebug());
  return MLN_STATUS_OK;
}

auto map_set_rendering_stats_view_enabled(mln_map map, bool enabled)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->map->enableRenderingStatsView(enabled);
  return MLN_STATUS_OK;
}

auto map_get_rendering_stats_view_enabled(mln_map map, bool* out_enabled)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_enabled == nullptr) {
    set_thread_error("out_enabled must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_enabled = live->map->isRenderingStatsViewEnabled();
  return MLN_STATUS_OK;
}

auto map_is_fully_loaded(mln_map map, bool* out_loaded) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_loaded == nullptr) {
    set_thread_error("out_loaded must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_loaded = live->map->isFullyLoaded();
  return MLN_STATUS_OK;
}

auto map_dump_debug_logs(mln_map map) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->map->dumpDebugLogs();
  return MLN_STATUS_OK;
}

auto map_get_size(
  mln_map map, uint32_t* out_width, uint32_t* out_height,
  double* out_scale_factor
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_width == nullptr || out_height == nullptr || out_scale_factor == nullptr
  ) {
    set_thread_error(
      "out_width, out_height, and out_scale_factor must not be null"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto options = live->map->getMapOptions();
  const auto size = options.size();
  *out_width = size.width;
  *out_height = size.height;
  *out_scale_factor = live->scale_factor;
  return MLN_STATUS_OK;
}

auto map_get_viewport_options(
  mln_map map, mln_map_viewport_options* out_options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_options == nullptr ||
    out_options->size < sizeof(mln_map_viewport_options)
  ) {
    set_thread_error("out_options must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto options = live->map->getMapOptions();
  *out_options = mln_map_viewport_options{
    .size = sizeof(mln_map_viewport_options),
    .fields = static_cast<uint32_t>(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) |
              MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE |
              MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE |
              MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET,
    .north_orientation =
      from_native_north_orientation(options.northOrientation()),
    .constrain_mode = from_native_constrain_mode(options.constrainMode()),
    .viewport_mode = from_native_viewport_mode(options.viewportMode()),
    .frustum_offset = from_native_edge_insets(live->map->getFrustumOffset())
  };
  return MLN_STATUS_OK;
}

auto map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_viewport_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0U) {
    live->map->setNorthOrientation(
      to_native_north_orientation(options->north_orientation)
    );
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0U) {
    live->map->setConstrainMode(
      to_native_constrain_mode(options->constrain_mode)
    );
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0U) {
    live->map->setViewportMode(to_native_viewport_mode(options->viewport_mode));
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0U) {
    live->map->setFrustumOffset(to_native_edge_insets(options->frustum_offset));
  }
  return MLN_STATUS_OK;
}

auto map_get_tile_options(mln_map map, mln_map_tile_options* out_options)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_options == nullptr || out_options->size < sizeof(mln_map_tile_options)
  ) {
    set_thread_error("out_options must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_options = mln_map_tile_options{
    .size = sizeof(mln_map_tile_options),
    .fields = static_cast<uint32_t>(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) |
              MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS |
              MLN_MAP_TILE_OPTION_LOD_SCALE |
              MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD |
              MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT | MLN_MAP_TILE_OPTION_LOD_MODE,
    .prefetch_zoom_delta = live->map->getPrefetchZoomDelta(),
    .lod_min_radius = live->map->getTileLodMinRadius(),
    .lod_scale = live->map->getTileLodScale(),
    .lod_pitch_threshold = live->map->getTileLodPitchThreshold(),
    .lod_zoom_shift = live->map->getTileLodZoomShift(),
    .lod_mode = from_native_tile_lod_mode(live->map->getTileLodMode())
  };
  return MLN_STATUS_OK;
}

auto map_set_tile_options(mln_map map, const mln_map_tile_options* options)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_tile_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  if ((options->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0U) {
    live->map->setPrefetchZoomDelta(
      static_cast<uint8_t>(options->prefetch_zoom_delta)
    );
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0U) {
    live->map->setTileLodMinRadius(options->lod_min_radius);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0U) {
    live->map->setTileLodScale(options->lod_scale);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0U) {
    live->map->setTileLodPitchThreshold(options->lod_pitch_threshold);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0U) {
    live->map->setTileLodZoomShift(options->lod_zoom_shift);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0U) {
    live->map->setTileLodMode(to_native_tile_lod_mode(options->lod_mode));
  }
  return MLN_STATUS_OK;
}

auto map_pixel_for_lat_lng(
  mln_map map, mln_lat_lng coordinate, mln_screen_point* out_point
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_point == nullptr) {
    set_thread_error("out_point must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinate_status = validate_lat_lng(coordinate);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }

  *out_point = from_native_screen_point(
    live->map->pixelForLatLng(to_native_lat_lng(coordinate))
  );
  return MLN_STATUS_OK;
}

auto map_lat_lng_for_pixel(
  mln_map map, mln_screen_point point, mln_lat_lng* out_coordinate
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_coordinate == nullptr) {
    set_thread_error("out_coordinate must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto point_status = validate_screen_point(point);
  if (point_status != MLN_STATUS_OK) {
    return point_status;
  }

  *out_coordinate = from_native_lat_lng(
    live->map->latLngForPixel(to_native_screen_point(point))
  );
  return MLN_STATUS_OK;
}

auto map_pixels_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  mln_screen_point* out_points
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (coordinate_count != 0 && out_points == nullptr) {
    set_thread_error(
      "out_points must not be null when coordinate_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinates_status =
    validate_lat_lng_array(coordinates, coordinate_count, true);
  if (coordinates_status != MLN_STATUS_OK) {
    return coordinates_status;
  }
  if (coordinate_count == 0) {
    return MLN_STATUS_OK;
  }

  const auto native_coordinates =
    to_native_lat_lngs(coordinates, coordinate_count);
  const auto pixels = live->map->pixelsForLatLngs(native_coordinates);
  auto output = std::span<mln_screen_point>{out_points, pixels.size()};
  auto output_position = output.begin();
  for (const auto& pixel : pixels) {
    *output_position = from_native_screen_point(pixel);
    ++output_position;
  }
  return MLN_STATUS_OK;
}

auto map_lat_lngs_for_pixels(
  mln_map map, const mln_screen_point* points, size_t point_count,
  mln_lat_lng* out_coordinates
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (point_count != 0 && out_coordinates == nullptr) {
    set_thread_error(
      "out_coordinates must not be null when point_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto points_status = validate_screen_point_array(points, point_count);
  if (points_status != MLN_STATUS_OK) {
    return points_status;
  }
  if (point_count == 0) {
    return MLN_STATUS_OK;
  }

  const auto native_points = to_native_screen_points(points, point_count);
  const auto coordinates = live->map->latLngsForPixels(native_points);
  auto output = std::span<mln_lat_lng>{out_coordinates, coordinates.size()};
  auto output_position = output.begin();
  for (const auto& coordinate : coordinates) {
    *output_position = from_native_lat_lng(coordinate);
    ++output_position;
  }
  return MLN_STATUS_OK;
}

auto map_projection_create(mln_map map, mln_map_projection* out_projection)
  -> mln_status {
  if (out_projection == nullptr) {
    set_thread_error("out_projection must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_projection != MLN_HANDLE_NULL) {
    set_thread_error("out_projection must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  auto owned_projection = std::make_shared<MapProjectionObject>();
  owned_projection->projection =
    std::make_unique<mln::MapProjection>(*live->map);

  *out_projection =
    handle_table<MapProjectionObject>().insert(std::move(owned_projection));
  return MLN_STATUS_OK;
}

auto map_projection_destroy(mln_map_projection projection) -> mln_status {
  auto& table = handle_table<MapProjectionObject>();
  std::shared_ptr<MapProjectionObject> owned;
  {
    const std::scoped_lock lock(table.mutex());
    if (table.resolve_locked(projection) == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    owned = table.remove_locked(projection);
  }
  {
    // A call that leased the handle before retirement either completes first
    // or observes the retired projection after this lock releases.
    const std::scoped_lock lock(owned->call_mutex);
    owned->projection.reset();
  }
  return MLN_STATUS_OK;
}

auto map_projection_get_camera(
  mln_map_projection projection, mln_camera_options* out_camera
) -> mln_status {
  return with_map_projection(
    projection, [out_camera](mln::MapProjection& live) -> mln_status {
      if (
        out_camera == nullptr || out_camera->size < sizeof(mln_camera_options)
      ) {
        set_thread_error(
          "out_camera must not be null and must have a valid size"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      *out_camera = from_native_camera(live.getCamera());
      return MLN_STATUS_OK;
    }
  );
}

auto map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera
) -> mln_status {
  return with_map_projection(
    projection, [camera](mln::MapProjection& live) -> mln_status {
      const auto camera_status = validate_camera_options(camera);
      if (camera_status != MLN_STATUS_OK) {
        return camera_status;
      }
      live.setCamera(to_native_camera(*camera));
      return MLN_STATUS_OK;
    }
  );
}

auto map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding
) -> mln_status {
  return with_map_projection(
    projection,
    [coordinates, coordinate_count,
     padding](mln::MapProjection& live) -> mln_status {
      const auto coordinates_status =
        validate_lat_lng_array(coordinates, coordinate_count, false);
      if (coordinates_status != MLN_STATUS_OK) {
        return coordinates_status;
      }
      const auto padding_status = validate_edge_insets(padding);
      if (padding_status != MLN_STATUS_OK) {
        return padding_status;
      }
      live.setVisibleCoordinates(
        to_native_lat_lngs(coordinates, coordinate_count),
        to_native_edge_insets(padding)
      );
      return MLN_STATUS_OK;
    }
  );
}

auto map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding
) -> mln_status {
  return with_map_projection(
    projection, [geometry, padding](mln::MapProjection& live) -> mln_status {
      const auto padding_status = validate_edge_insets(padding);
      if (padding_status != MLN_STATUS_OK) {
        return padding_status;
      }
      auto native_geometry = to_native_geometry(geometry);
      if (!native_geometry) {
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      auto coordinates = geometry_lat_lngs(*native_geometry);
      if (coordinates.empty()) {
        set_thread_error("geometry must contain at least one coordinate");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      live.setVisibleCoordinates(coordinates, to_native_edge_insets(padding));
      return MLN_STATUS_OK;
    }
  );
}

auto map_projection_pixel_for_lat_lng(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_screen_point* out_point
) -> mln_status {
  return with_map_projection(
    projection,
    [coordinate, out_point](mln::MapProjection& live) -> mln_status {
      if (out_point == nullptr) {
        set_thread_error("out_point must not be null");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      const auto coordinate_status = validate_lat_lng(coordinate);
      if (coordinate_status != MLN_STATUS_OK) {
        return coordinate_status;
      }
      *out_point = from_native_screen_point(
        live.pixelForLatLng(to_native_lat_lng(coordinate))
      );
      return MLN_STATUS_OK;
    }
  );
}

auto map_projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate
) -> mln_status {
  return with_map_projection(
    projection,
    [point, out_coordinate](mln::MapProjection& live) -> mln_status {
      if (out_coordinate == nullptr) {
        set_thread_error("out_coordinate must not be null");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      const auto point_status = validate_screen_point(point);
      if (point_status != MLN_STATUS_OK) {
        return point_status;
      }
      *out_coordinate =
        from_native_lat_lng(live.latLngForPixel(to_native_screen_point(point)));
      return MLN_STATUS_OK;
    }
  );
}

auto projected_meters_for_lat_lng(
  mln_lat_lng coordinate, mln_projected_meters* out_meters
) -> mln_status {
  if (out_meters == nullptr) {
    set_thread_error("out_meters must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinate_status = validate_lat_lng(coordinate);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }

  const auto meters =
    mln::Projection::projectedMetersForLatLng(to_native_lat_lng(coordinate));
  *out_meters = mln_projected_meters{
    .northing = meters.northing(), .easting = meters.easting()
  };
  return MLN_STATUS_OK;
}

auto lat_lng_for_projected_meters(
  mln_projected_meters meters, mln_lat_lng* out_coordinate
) -> mln_status {
  if (out_coordinate == nullptr) {
    set_thread_error("out_coordinate must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto meters_status = validate_projected_meters(meters);
  if (meters_status != MLN_STATUS_OK) {
    return meters_status;
  }

  *out_coordinate = from_native_lat_lng(
    mln::Projection::latLngForProjectedMeters(
      mln::ProjectedMeters{meters.northing, meters.easting}
    )
  );
  return MLN_STATUS_OK;
}

auto map_move_by(mln_map map, double delta_x, double delta_y) -> mln_status {
  return map_move_by_animated(map, delta_x, delta_y, nullptr);
}

auto map_move_by_animated(
  mln_map map, double delta_x, double delta_y,
  const mln_animation_options* animation
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!std::isfinite(delta_x) || !std::isfinite(delta_y)) {
    set_thread_error("move deltas must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto animation_status = validate_animation_options(animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }

  live->map->moveBy(
    mln::ScreenCoordinate{delta_x, delta_y},
    to_native_animation(live->runtime, map, live->event_state, animation)
  );
  return MLN_STATUS_OK;
}

auto map_scale_by(mln_map map, double scale, const mln_screen_point* anchor)
  -> mln_status {
  return map_scale_by_animated(map, scale, anchor, nullptr);
}

auto map_scale_by_animated(
  mln_map map, double scale, const mln_screen_point* anchor,
  const mln_animation_options* animation
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!std::isfinite(scale) || scale <= 0.0) {
    set_thread_error("scale must be positive and finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto native_anchor = std::optional<mln::ScreenCoordinate>{};
  if (anchor != nullptr) {
    const auto anchor_status = validate_screen_point(*anchor);
    if (anchor_status != MLN_STATUS_OK) {
      return anchor_status;
    }
    native_anchor = screen_point(*anchor);
  }
  const auto animation_status = validate_animation_options(animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }

  live->map->scaleBy(
    scale, native_anchor,
    to_native_animation(live->runtime, map, live->event_state, animation)
  );
  return MLN_STATUS_OK;
}

auto map_rotate_by(mln_map map, mln_screen_point first, mln_screen_point second)
  -> mln_status {
  return map_rotate_by_animated(map, first, second, nullptr);
}

auto map_rotate_by_animated(
  mln_map map, mln_screen_point first, mln_screen_point second,
  const mln_animation_options* animation
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto first_status = validate_screen_point(first);
  if (first_status != MLN_STATUS_OK) {
    return first_status;
  }
  const auto second_status = validate_screen_point(second);
  if (second_status != MLN_STATUS_OK) {
    return second_status;
  }
  const auto animation_status = validate_animation_options(animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }

  live->map->rotateBy(
    screen_point(first), screen_point(second),
    to_native_animation(live->runtime, map, live->event_state, animation)
  );
  return MLN_STATUS_OK;
}

auto map_pitch_by(mln_map map, double pitch) -> mln_status {
  return map_pitch_by_animated(map, pitch, nullptr);
}

auto map_pitch_by_animated(
  mln_map map, double pitch, const mln_animation_options* animation
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!std::isfinite(pitch)) {
    set_thread_error("pitch must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto animation_status = validate_animation_options(animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }

  live->map->pitchBy(
    pitch, to_native_animation(live->runtime, map, live->event_state, animation)
  );
  return MLN_STATUS_OK;
}

auto map_cancel_transitions(mln_map map) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->map->cancelTransitions();
  return MLN_STATUS_OK;
}

auto map_set_gesture_in_progress(mln_map map, bool in_progress) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->map->setGestureInProgress(in_progress);
  return MLN_STATUS_OK;
}

auto map_is_gesture_in_progress(mln_map map, bool* out_in_progress)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_in_progress == nullptr) {
    set_thread_error("out_in_progress must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_in_progress = live->map->isGestureInProgress();
  return MLN_STATUS_OK;
}

auto validate_camera_output(mln_camera_options* out_camera) -> mln_status {
  if (out_camera == nullptr || out_camera->size < sizeof(mln_camera_options)) {
    set_thread_error("out_camera must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto map_camera_for_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto bounds_status = validate_lat_lng_bounds(bounds);
  if (bounds_status != MLN_STATUS_OK) {
    return bounds_status;
  }
  const auto fit_status = validate_camera_fit_options(fit_options);
  if (fit_status != MLN_STATUS_OK) {
    return fit_status;
  }
  const auto output_status = validate_camera_output(out_camera);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  *out_camera = from_native_camera(live->map->cameraForLatLngBounds(
    to_native_lat_lng_bounds(bounds), camera_fit_padding(fit_options),
    camera_fit_bearing(fit_options), camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_camera_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto coordinates_status =
    validate_lat_lng_array(coordinates, coordinate_count, false);
  if (coordinates_status != MLN_STATUS_OK) {
    return coordinates_status;
  }
  const auto fit_status = validate_camera_fit_options(fit_options);
  if (fit_status != MLN_STATUS_OK) {
    return fit_status;
  }
  const auto output_status = validate_camera_output(out_camera);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  *out_camera = from_native_camera(live->map->cameraForLatLngs(
    to_native_lat_lngs(coordinates, coordinate_count),
    camera_fit_padding(fit_options), camera_fit_bearing(fit_options),
    camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  auto native_geometry = to_native_geometry(geometry);
  if (!native_geometry) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (geometry_lat_lngs(*native_geometry).empty()) {
    set_thread_error("geometry must contain at least one coordinate");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto fit_status = validate_camera_fit_options(fit_options);
  if (fit_status != MLN_STATUS_OK) {
    return fit_status;
  }
  const auto output_status = validate_camera_output(out_camera);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  *out_camera = from_native_camera(live->map->cameraForGeometry(
    *native_geometry, camera_fit_padding(fit_options),
    camera_fit_bearing(fit_options), camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera, mln_lat_lng_bounds* out_bounds
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  if (out_bounds == nullptr) {
    set_thread_error("out_bounds must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_bounds = from_native_lat_lng_bounds(
    live->map->latLngBoundsForCamera(to_native_camera(*camera))
  );
  return MLN_STATUS_OK;
}

auto map_lat_lng_bounds_for_camera_unwrapped(
  mln_map map, const mln_camera_options* camera, mln_lat_lng_bounds* out_bounds
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  if (out_bounds == nullptr) {
    set_thread_error("out_bounds must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_bounds = from_native_lat_lng_bounds(
    live->map->latLngBoundsForCameraUnwrapped(to_native_camera(*camera))
  );
  return MLN_STATUS_OK;
}

auto map_get_bounds(mln_map map, mln_bound_options* out_options) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_options == nullptr || out_options->size < sizeof(mln_bound_options)) {
    set_thread_error("out_options must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_options = from_native_bound_options(live->map->getBounds());
  return MLN_STATUS_OK;
}

auto map_set_bounds(mln_map map, const mln_bound_options* options)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_bound_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  // Native setBounds only applies optionals that are set, so this preserves
  // constraints omitted from options->fields.
  live->map->setBounds(to_native_bound_options(*options));
  return MLN_STATUS_OK;
}

auto map_get_free_camera_options(
  mln_map map, mln_free_camera_options* out_options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_options == nullptr ||
    out_options->size < sizeof(mln_free_camera_options)
  ) {
    set_thread_error("out_options must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_options = from_native_free_camera(live->map->getFreeCameraOptions());
  return MLN_STATUS_OK;
}

auto map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_free_camera_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  live->map->setFreeCameraOptions(to_native_free_camera(*options));
  return MLN_STATUS_OK;
}

}  // namespace mln::core
