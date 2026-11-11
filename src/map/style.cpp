#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bit>
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
#include <mln/style/conversion/geojson_options.hpp>  // IWYU pragma: keep
#include <mln/style/conversion/json.hpp>
#include <mln/style/conversion/layer.hpp>   // IWYU pragma: keep
#include <mln/style/conversion/light.hpp>   // IWYU pragma: keep
#include <mln/style/conversion/source.hpp>  // IWYU pragma: keep
#include <mln/style/conversion/stringify.hpp>
#include <mln/style/conversion/transition_options.hpp>  // IWYU pragma: keep
#include <mln/style/conversion_impl.hpp>
#include <mln/style/image.hpp>
#include <mln/style/layer.hpp>
#include <mln/style/light.hpp>
#include <mln/style/rapidjson_conversion.hpp>
#include <mln/style/source.hpp>
#include <mln/style/sources/custom_geometry_source.hpp>
#include <mln/style/sources/custom_vector_source.hpp>
#include <mln/style/sources/geojson_source.hpp>
#include <mln/style/sources/geojson_source_impl.hpp>
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

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "geojson/geojson.hpp"
#include "geojson/geojson_source_data.hpp"
#include "handles/handle_table.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"
#include "style/style_value.hpp"

namespace mln::core {

auto validate_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln_status;
auto validate_lat_lng_array(
  const mln_lat_lng* coordinates, size_t coordinate_count, bool allow_empty
) -> mln_status;
auto to_native_lat_lng(mln_lat_lng coordinate) -> mln::LatLng;
auto from_native_lat_lng(const mln::LatLng& coordinate) -> mln_lat_lng;
auto to_native_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln::LatLngBounds;
auto from_native_lat_lng_bounds(const mln::LatLngBounds& bounds)
  -> mln_lat_lng_bounds;

}  // namespace mln::core

namespace {

auto string_from_view(mln_buffer_view string) -> std::string {
  if (string.size == 0) {
    return {};
  }
  return std::string{static_cast<const char*>(string.data), string.size};
}

// Quotes an ID for a diagnostic, as in `source "water" does not exist`.
auto quote_id(std::string_view id) -> std::string {
  auto result = std::string{"\""};
  result.append(id);
  result.push_back('"');
  return result;
}

auto set_source_error(std::string_view source_id, std::string_view problem)
  -> void {
  const auto message =
    "source " + quote_id(source_id) + " " + std::string{problem};
  mln::core::set_thread_error(message.c_str());
}

auto set_layer_error(std::string_view layer_id, std::string_view problem)
  -> void {
  const auto message =
    "layer " + quote_id(layer_id) + " " + std::string{problem};
  mln::core::set_thread_error(message.c_str());
}

// Finds a source, or sets the diagnostic and returns why it could not.
auto find_source(
  mln::style::Style& style, const std::string& id,
  mln::style::Source*& out_source
) -> mln_status {
  out_source = style.getSource(id);
  if (out_source == nullptr) {
    set_source_error(id, "does not exist");
    return MLN_STATUS_NOT_FOUND;
  }
  return MLN_STATUS_OK;
}

// Finds a source of type T. The kind names T in the diagnostic, as in
// `source "water" is not a GeoJSON source`.
template <typename T>
auto find_source(
  mln::style::Style& style, const std::string& id, std::string_view kind,
  T*& out_source
) -> mln_status {
  out_source = nullptr;
  mln::style::Source* source = nullptr;
  const auto status = find_source(style, id, source);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  out_source = source->as<T>();
  if (out_source == nullptr) {
    set_source_error(id, std::string{"is not "}.append(kind));
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_c_source_type(mln::style::SourceType type) -> mln_style_source_type {
  switch (type) {
    case mln::style::SourceType::Vector:
      return MLN_STYLE_SOURCE_TYPE_VECTOR;
    case mln::style::SourceType::Raster:
      return MLN_STYLE_SOURCE_TYPE_RASTER;
    case mln::style::SourceType::RasterDEM:
      return MLN_STYLE_SOURCE_TYPE_RASTER_DEM;
    case mln::style::SourceType::GeoJSON:
      return MLN_STYLE_SOURCE_TYPE_GEO_JSON;
    case mln::style::SourceType::Video:
      return MLN_STYLE_SOURCE_TYPE_VIDEO;
    case mln::style::SourceType::Annotations:
      return MLN_STYLE_SOURCE_TYPE_ANNOTATIONS;
    case mln::style::SourceType::Image:
      return MLN_STYLE_SOURCE_TYPE_IMAGE;
    case mln::style::SourceType::CustomVector:
      return MLN_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY;
    case mln::style::SourceType::CustomMVTVector:
      return MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR;
  }
  assert(false);
  return MLN_STYLE_SOURCE_TYPE_UNKNOWN;
}

auto to_c_tile_scheme(mln::Tileset::Scheme scheme) -> mln_style_tile_scheme {
  switch (scheme) {
    case mln::Tileset::Scheme::XYZ:
      return MLN_STYLE_TILE_SCHEME_XYZ;
    case mln::Tileset::Scheme::TMS:
      return MLN_STYLE_TILE_SCHEME_TMS;
  }
  assert(false);
  return MLN_STYLE_TILE_SCHEME_XYZ;
}

auto to_c_vector_encoding(mln::Tileset::VectorEncoding encoding)
  -> mln_style_vector_tile_encoding {
  switch (encoding) {
    case mln::Tileset::VectorEncoding::Mapbox:
      return MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
    case mln::Tileset::VectorEncoding::MLT:
      return MLN_STYLE_VECTOR_TILE_ENCODING_MLT;
  }
  assert(false);
  return MLN_STYLE_VECTOR_TILE_ENCODING_MVT;
}

auto to_c_raster_encoding(mln::Tileset::RasterEncoding encoding)
  -> mln_style_raster_dem_encoding {
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

// The engine's tile function names neither the map nor the source, so each
// source's function carries its own.
template <typename Callback>
auto to_native_tile_function(
  Callback callback, void* user_data, mln_map map,
  std::shared_ptr<const std::string> source_id
) -> mln::style::TileFunction {
  if (callback == nullptr) {
    return nullptr;
  }
  return [callback, user_data, map,
          source_id =
            std::move(source_id)](const mln::CanonicalTileID& tile_id) -> void {
    if (mln::core::process_exiting()) return;
    try {
      callback(
        user_data, map,
        mln_buffer_view{.data = source_id->data(), .size = source_id->size()},
        to_c_canonical_tile_id(tile_id)
      );
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("custom geometry source callback threw");
    }
  };
}

auto to_native_custom_geometry_source_options(
  const mln_custom_geometry_source_options& options, mln_map map,
  const std::string& source_id
) -> mln::style::CustomGeometrySource::Options {
  auto id = std::make_shared<const std::string>(source_id);
  auto result = mln::style::CustomGeometrySource::Options{};
  result.fetchTileFunction =
    to_native_tile_function(options.fetch_tile, options.user_data, map, id);
  result.cancelTileFunction = to_native_tile_function(
    options.cancel_tile, options.user_data, map, std::move(id)
  );
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

auto has_custom_vector_source_option(
  const mln_custom_vector_source_options& options, uint32_t field
) -> bool {
  return (options.fields & field) != 0U;
}

auto effective_custom_vector_source_options(
  const mln_custom_vector_source_options& options
) -> mln_custom_vector_source_options {
  auto result = mln::core::custom_vector_source_options_default();
  result.fields = options.fields;
  result.fetch_tile = options.fetch_tile;
  result.cancel_tile = options.cancel_tile;
  result.user_data = options.user_data;
  result.release_user_data = options.release_user_data;
  if (
    has_custom_vector_source_option(
      options, MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM
    )
  ) {
    result.min_zoom = options.min_zoom;
  }
  if (
    has_custom_vector_source_option(
      options, MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM
    )
  ) {
    result.max_zoom = options.max_zoom;
  }
  return result;
}

auto to_native_tile_data_format(mln_style_vector_tile_encoding encoding)
  -> mln::style::TileDataFormat {
  switch (encoding) {
    case MLN_STYLE_VECTOR_TILE_ENCODING_MVT:
      return mln::style::TileDataFormat::MVT;
    case MLN_STYLE_VECTOR_TILE_ENCODING_MLT:
      return mln::style::TileDataFormat::MLT;
    default:
      assert(false);
      return mln::style::TileDataFormat::MVT;
  }
}

auto to_native_custom_vector_source_options(
  const mln_custom_vector_source_options& options, mln_map map,
  const std::string& source_id
) -> mln::style::CustomVectorSource::Options {
  auto id = std::make_shared<const std::string>(source_id);
  auto result = mln::style::CustomVectorSource::Options{};
  result.fetchTileFunction =
    to_native_tile_function(options.fetch_tile, options.user_data, map, id);
  result.cancelTileFunction = to_native_tile_function(
    options.cancel_tile, options.user_data, map, std::move(id)
  );
  result.zoomRange = mln::Range<uint8_t>{
    static_cast<uint8_t>(options.min_zoom),
    static_cast<uint8_t>(options.max_zoom)
  };
  return result;
}

}  // namespace

namespace mln::core {

auto validate_source_can_be_added(
  mln::style::Style& style, const std::string& source_id
) -> mln_status {
  if (style.getSource(source_id) != nullptr) {
    set_source_error(source_id, "already exists");
    return MLN_STATUS_ALREADY_EXISTS;
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
    MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT | MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED;
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
  if (has_style_image_option(*options, MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED)) {
    result.keep_unused = options->keep_unused;
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

auto to_native_text_fit(mln_style_image_text_fit value) -> mln::style::TextFit {
  switch (value) {
    case MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY:
      return mln::style::TextFit::stretchOnly;
    case MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL:
      return mln::style::TextFit::proportional;
    default:
      return mln::style::TextFit::stretchOrShrink;
  }
}

auto from_native_text_fit(mln::style::TextFit value)
  -> mln_style_image_text_fit {
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

auto geojson_source_options_default() noexcept -> mln_geo_json_source_options {
  const auto defaults = mln::style::GeoJSONOptions{};
  return mln_geo_json_source_options{
    .size = sizeof(mln_geo_json_source_options),
    .fields = 0,
    .min_zoom = static_cast<double>(defaults.minzoom),
    .max_zoom = static_cast<double>(defaults.maxzoom),
    .tolerance = defaults.tolerance,
    .cluster_max_zoom = static_cast<double>(defaults.clusterMaxZoom),
    .cluster_properties = {},
    .buffer = defaults.buffer,
    .cluster_radius = defaults.clusterRadius,
    .cluster_min_points = defaults.clusterMinPoints,
    .line_metrics = defaults.lineMetrics,
    .cluster = defaults.cluster
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
    .release_user_data = nullptr,
    .queue = MLN_HANDLE_NULL
  };
}

auto custom_vector_source_options_default() noexcept
  -> mln_custom_vector_source_options {
  return mln_custom_vector_source_options{
    .size = sizeof(mln_custom_vector_source_options),
    .fields = 0,
    .fetch_tile = nullptr,
    .cancel_tile = nullptr,
    .user_data = nullptr,
    .min_zoom = 0,
    .max_zoom = 18,
    .release_user_data = nullptr,
    .queue = MLN_HANDLE_NULL
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
    .sdf = false,
    .keep_unused = false
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
  // The engine scales a tile's geometry to util::EXTENT by the whole number
  // EXTENT / tile_size, and stores the scaled buffer in a uint16_t.
  constexpr auto extent = static_cast<uint32_t>(mln::util::EXTENT);
  if (
    !std::has_single_bit(effective.tile_size) || effective.tile_size > extent
  ) {
    mln::core::set_thread_error(
      "tile_size must be a power of two within [1, 8192]"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto max_buffer =
    std::numeric_limits<uint16_t>::max() / (extent / effective.tile_size);
  if (effective.buffer > max_buffer) {
    const auto message = "buffer must be at most " +
                         std::to_string(max_buffer) + " for a tile_size of " +
                         std::to_string(effective.tile_size);
    mln::core::set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_custom_vector_source_options(
  const mln_custom_vector_source_options* options
) -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_custom_vector_source_options)) {
    mln::core::set_thread_error(
      "mln_custom_vector_source_options.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM) |
    MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_custom_vector_source_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    has_custom_vector_source_option(
      *options, MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM
    )
  ) {
    const auto status =
      validate_custom_geometry_zoom(options->min_zoom, "min_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    has_custom_vector_source_option(
      *options, MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM
    )
  ) {
    const auto status =
      validate_custom_geometry_zoom(options->max_zoom, "max_zoom");
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  const auto effective = effective_custom_vector_source_options(*options);
  if (effective.min_zoom > effective.max_zoom) {
    mln::core::set_thread_error(
      "min_zoom must be less than or equal to max_zoom"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_native_premultiplied_rgba8_image(
  const mln_premultiplied_rgba8_image& image
) -> mln::PremultipliedImage {
  const auto row_bytes = static_cast<size_t>(image.width) * 4U;
  auto result = mln::PremultipliedImage{
    mln::Size{image.width, image.height},
    // Uninitialized storage; every byte is written below. OpenHarmony's
    // libc++ lacks std::make_unique_for_overwrite.
    std::unique_ptr<uint8_t[]>(new uint8_t[row_bytes * image.height])
  };
  if (image.stride == row_bytes) {
    std::memcpy(result.data.get(), image.pixels, result.bytes());
    return result;
  }
  const auto output_stride = result.stride();
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

auto validate_style_image_input(
  const mln_premultiplied_rgba8_image* image,
  const mln_style_image_options* options
) -> mln_status {
  const auto image_status = validate_premultiplied_rgba8_image(image);
  return image_status == MLN_STATUS_OK ? validate_style_image_options(options)
                                       : image_status;
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

auto map_add_style_source_json(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view source_json
) -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(source_id);
  if (style.getSource(id) != nullptr) {
    set_source_error(id, "already exists");
    return MLN_STATUS_ALREADY_EXISTS;
  }

  auto error = mln::style::conversion::Error{};
  auto source =
    mln::style::conversion::convertJSON<std::unique_ptr<mln::style::Source>>(
      string_from_view(source_json), error, id
    );
  if (!source) {
    set_style_conversion_error(("style source " + quote_id(id)).c_str(), error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  style.addSource(std::move(*source));
  return MLN_STATUS_OK;
}

auto map_remove_style_source(MapObject& live, mln_buffer_view source_id)
  -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(source_id);
  mln::style::Source* source = nullptr;
  const auto status = find_source(style, id, source);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  auto removed = style.removeSource(id);
  if (!removed) {
    // The source exists, so the engine refused it only because a layer's
    // source ID matches, the same test as this search.
    const auto layers = style.getLayers();
    const auto user = std::ranges::find_if(
      layers, [&id](const mln::style::Layer* layer) -> bool {
        return layer->getSourceID() == id;
      }
    );
    set_source_error(id, "is used by layer " + quote_id((*user)->getID()));
    return MLN_STATUS_INVALID_STATE;
  }
  // The detached source is dropped before the release runs, so the style no
  // longer holds the callbacks that read the host's state.
  removed.reset();
  release_callback_source(live, id);
  cancel_geojson_source_update(live, id, "the source was removed");
  // A later source with this ID starts with no state, as in MapLibre GL JS.
  if (live.feature_state.remove_source(id)) {
    map_native(live).triggerRepaint();
  }
  return MLN_STATUS_OK;
}

namespace {

auto style_source_record(const mln::style::Source& source)
  -> StyleSourceRecord {
  auto record = StyleSourceRecord{};
  record.id = source.getID();
  auto& info = record.info;
  info.type = to_c_source_type(source.getType());
  info.is_volatile = source.isVolatile();
  if (auto attribution = source.getAttribution()) {
    record.attribution = std::move(*attribution);
  }
  if (auto url = source_url(source)) {
    record.url = std::move(*url);
  }
  if (const auto* geojson = source.as<mln::style::GeoJSONSource>()) {
    info.fields |= MLN_STYLE_SOURCE_INFO_GEO_JSON;
    info.geo_json = from_native_geojson_source_options(
      geojson->getOptions(), record.cluster_properties
    );
  }

  const auto* tile_source = tile_source_from_source(source);
  if (tile_source == nullptr) {
    return record;
  }

  info.fields |= MLN_STYLE_SOURCE_INFO_TILE_SIZE;
  info.tile_size = tile_source->getTileSize();
  if (const auto* vector_source = source.as<mln::style::VectorSource>()) {
    info.fields |= MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING;
    info.vector_encoding = to_c_vector_encoding(vector_source->getEncoding());
  }

  const auto* tileset = inline_tileset(*tile_source);
  if (tileset == nullptr) {
    return record;
  }

  info.fields |= MLN_STYLE_SOURCE_INFO_TILE_JSON;
  info.tile_json.min_zoom = tileset->zoomRange.min;
  info.tile_json.max_zoom = tileset->zoomRange.max;
  info.tile_json.scheme = to_c_tile_scheme(tileset->scheme);
  record.tile_urls = tileset->tiles;
  if (tileset->bounds) {
    info.fields |= MLN_STYLE_SOURCE_INFO_BOUNDS;
    info.bounds = from_native_lat_lng_bounds(*tileset->bounds);
  }
  if (tileset->vectorEncoding) {
    info.fields |= MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING;
    info.vector_encoding = to_c_vector_encoding(*tileset->vectorEncoding);
  }
  if (tileset->rasterEncoding) {
    info.fields |= MLN_STYLE_SOURCE_INFO_RASTER_ENCODING;
    info.raster_encoding = to_c_raster_encoding(*tileset->rasterEncoding);
  }
  return record;
}

}  // namespace

auto map_get_style_source(
  MapObject& live, mln_buffer_view source_id, StyleSourceRecord& out_source
) -> bool {
  const auto* source =
    map_native(live).getStyle().getSource(string_from_view(source_id));
  if (source == nullptr) {
    return false;
  }
  out_source = style_source_record(*source);
  return true;
}

auto map_list_style_sources(
  MapObject& live, std::vector<StyleSourceRecord>& out_sources
) -> mln_status {
  const auto sources = map_native(live).getStyle().getSources();
  out_sources.reserve(sources.size());
  for (const auto* source : sources) {
    out_sources.push_back(style_source_record(*source));
  }
  return MLN_STATUS_OK;
}

auto map_add_geojson_source(
  MapObject& live, mln_buffer_view source_id,
  const mln::Immutable<mln::style::GeoJSONOptions>& options,
  mln_buffer_view attribution
) -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto source_options = mln::makeMutable<mln::style::GeoJSONOptions>(*options);
  if (attribution.size > 0) {
    source_options->attribution = string_from_view(attribution);
  }
  style.addSource(
    std::make_unique<mln::style::GeoJSONSource>(
      id, mln::Immutable<mln::style::GeoJSONOptions>{std::move(source_options)}
    )
  );
  return MLN_STATUS_OK;
}

auto map_set_geojson_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status {
  const auto id = string_from_view(source_id);
  mln::style::GeoJSONSource* geojson_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), id, "a GeoJSON source", geojson_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }

  geojson_source->setURL(string_from_view(url));
  cancel_geojson_source_update(live, id, "the source's URL replaced its data");
  return MLN_STATUS_OK;
}

auto map_find_geojson_source(
  MapObject& live, const std::string& source_id, GeoJsonSourceTarget& out_target
) -> mln_status {
  mln::style::GeoJSONSource* geojson_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), source_id, "a GeoJSON source", geojson_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  out_target.options.emplace(geojson_source->impl().getOptions());
  out_target.synchronous = geojson_source->baseImpl->isUpdateSynchronous();
  return MLN_STATUS_OK;
}

auto map_install_geojson_source_data(
  MapObject& live, const std::string& source_id,
  std::shared_ptr<mln::style::GeoJSONData> data
) -> mln_status {
  mln::style::GeoJSONSource* geojson_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), source_id, "a GeoJSON source", geojson_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  geojson_source->setGeoJSONData(std::move(data));
  return MLN_STATUS_OK;
}

auto map_set_geojson_source_synchronous_tiling(
  MapObject& live, mln_buffer_view source_id, bool enabled
) -> mln_status {
  const auto id = string_from_view(source_id);
  mln::style::GeoJSONSource* geojson_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), id, "a GeoJSON source", geojson_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }

  geojson_source->setOverrideSynchronousUpdate(enabled);
  return MLN_STATUS_OK;
}

auto map_set_style_source_volatile(
  MapObject& live, mln_buffer_view source_id, bool is_volatile
) -> mln_status {
  const auto id = string_from_view(source_id);
  mln::style::Source* source = nullptr;
  const auto status = find_source(map_native(live).getStyle(), id, source);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  source->setVolatile(is_volatile);
  return MLN_STATUS_OK;
}

namespace {

// Tracks the source before the style takes it, so the two cannot disagree. A
// throw while tracking leaves no source in the style, and a style that rejects
// the source untracks it again; either way the add failed and the caller still
// owns user_data. Tracking afterwards would leave a live source whose release
// never runs.
auto add_callback_source(
  MapObject& live, const std::string& source_id, CallbackSourceKind kind,
  CallbackSourceRelease release, void* user_data,
  std::unique_ptr<mln::style::Source> source
) -> void {
  track_callback_source(live, source_id, kind, release, user_data);
  try {
    map_native(live).getStyle().addSource(std::move(source));
  } catch (...) {
    untrack_callback_source(live, source_id);
    throw;
  }
}

// The engine's custom geometry tile draws only a feature collection, and an
// empty tile for anything else.
auto to_feature_collection(mln::GeoJSON geojson) -> mln::FeatureCollection {
  if (geojson.is<mln::FeatureCollection>()) {
    return std::move(geojson.get<mln::FeatureCollection>());
  }
  auto features = mln::FeatureCollection{};
  if (geojson.is<mln::GeoJSONFeature>()) {
    features.push_back(std::move(geojson.get<mln::GeoJSONFeature>()));
  } else {
    features.emplace_back(std::move(geojson.get<mln::Geometry<double>>()));
  }
  return features;
}

}  // namespace

auto map_add_custom_geometry_source(
  MapObject& live, mln_buffer_view source_id,
  const mln_custom_geometry_source_options* options
) -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_custom_geometry_source_options(*options);
  add_callback_source(
    live, id, CallbackSourceKind::CustomGeometry, effective.release_user_data,
    effective.user_data,
    std::make_unique<mln::style::CustomGeometrySource>(
      id, to_native_custom_geometry_source_options(effective, live.self, id)
    )
  );
  return MLN_STATUS_OK;
}

auto lookup_custom_geometry_source(
  MapObject& live, mln_buffer_view source_id,
  mln::style::CustomGeometrySource*& out_source
) -> mln_status {
  const auto id = string_from_view(source_id);
  return find_source(
    map_native(live).getStyle(), id, "a custom geometry source", out_source
  );
}

auto map_set_custom_geometry_source_tile_data(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view data
) -> mln_status {
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }
  auto geojson = to_native_geojson(data);
  if (!geojson) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  mln::style::CustomGeometrySource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_geometry_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }
  custom_source->setTileData(
    to_native_canonical_tile_id(tile_id),
    to_feature_collection(std::move(*geojson))
  );
  return MLN_STATUS_OK;
}

auto map_set_custom_geometry_source_tile_error(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message
) -> mln_status {
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }

  mln::style::CustomGeometrySource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_geometry_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }
  custom_source->setTileError(
    to_native_canonical_tile_id(tile_id),
    std::make_exception_ptr(std::runtime_error(string_from_view(message)))
  );
  return MLN_STATUS_OK;
}

auto map_invalidate_custom_geometry_source_tile(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id
) -> mln_status {
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }

  mln::style::CustomGeometrySource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_geometry_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }
  custom_source->invalidateTile(to_native_canonical_tile_id(tile_id));
  return MLN_STATUS_OK;
}

auto map_invalidate_custom_geometry_source_region(
  MapObject& live, mln_buffer_view source_id, mln_lat_lng_bounds bounds
) -> mln_status {
  const auto bounds_status = validate_lat_lng_bounds(bounds);
  if (bounds_status != MLN_STATUS_OK) {
    return bounds_status;
  }

  mln::style::CustomGeometrySource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_geometry_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }
  custom_source->invalidateRegion(to_native_lat_lng_bounds(bounds));
  return MLN_STATUS_OK;
}

auto lookup_custom_vector_source(
  MapObject& live, mln_buffer_view source_id,
  mln::style::CustomVectorSource*& out_source
) -> mln_status {
  const auto id = string_from_view(source_id);
  return find_source(
    map_native(live).getStyle(), id, "a custom vector source", out_source
  );
}

auto map_add_custom_vector_source(
  MapObject& live, mln_buffer_view source_id,
  const mln_custom_vector_source_options* options
) -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  const auto effective = effective_custom_vector_source_options(*options);
  add_callback_source(
    live, id, CallbackSourceKind::CustomVector, effective.release_user_data,
    effective.user_data,
    std::make_unique<mln::style::CustomVectorSource>(
      id, to_native_custom_vector_source_options(effective, live.self, id)
    )
  );
  return MLN_STATUS_OK;
}

auto map_set_custom_vector_source_tile_data(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_style_vector_tile_encoding encoding, mln_buffer_view data
) -> mln_status {
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }

  mln::style::CustomVectorSource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_vector_source(live, source_id, custom_source);
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
    to_native_tile_data_format(encoding)
  );
  return MLN_STATUS_OK;
}

auto map_set_custom_vector_source_tile_error(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id,
  mln_buffer_view message
) -> mln_status {
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }

  mln::style::CustomVectorSource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_vector_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }

  custom_source->setTileError(
    to_native_canonical_tile_id(tile_id),
    std::make_exception_ptr(std::runtime_error(string_from_view(message)))
  );
  return MLN_STATUS_OK;
}

auto map_invalidate_custom_vector_source_tile(
  MapObject& live, mln_buffer_view source_id, mln_canonical_tile_id tile_id
) -> mln_status {
  const auto tile_status = validate_canonical_tile_id(tile_id);
  if (tile_status != MLN_STATUS_OK) {
    return tile_status;
  }

  mln::style::CustomVectorSource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_vector_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }
  custom_source->invalidateTile(to_native_canonical_tile_id(tile_id));
  return MLN_STATUS_OK;
}

auto map_invalidate_custom_vector_source_region(
  MapObject& live, mln_buffer_view source_id, mln_lat_lng_bounds bounds
) -> mln_status {
  const auto bounds_status = validate_lat_lng_bounds(bounds);
  if (bounds_status != MLN_STATUS_OK) {
    return bounds_status;
  }

  mln::style::CustomVectorSource* custom_source = nullptr;
  const auto source_status =
    lookup_custom_vector_source(live, source_id, custom_source);
  if (source_status != MLN_STATUS_OK) {
    return source_status;
  }
  custom_source->invalidateRegion(to_native_lat_lng_bounds(bounds));
  return MLN_STATUS_OK;
}

auto make_style_image(
  mln_buffer_view image_id, const mln_premultiplied_rgba8_image& image,
  const mln_style_image_options* options
) -> mln::style::Image {
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

  return mln::style::Image{
    string_from_view(image_id),
    to_native_premultiplied_rgba8_image(image),
    effective.pixel_ratio,
    effective.sdf,
    to_native_image_stretches(effective.stretch_x, effective.stretch_x_count),
    to_native_image_stretches(effective.stretch_y, effective.stretch_y_count),
    content,
    text_fit_width,
    text_fit_height
  };
}

auto style_image_keeps_unused(const mln_style_image_options* options) -> bool {
  return effective_style_image_options(options).keep_unused;
}

auto map_set_style_image(
  MapObject& live, mln::style::Image&& image, bool keep_unused
) -> mln_status {
  // The set changes before the style, so a failed allocation leaves the style
  // unchanged.
  if (keep_unused) {
    live.kept_style_images.insert(image.getID());
  } else {
    live.kept_style_images.erase(image.getID());
  }
  map_native(live).getStyle().addImage(
    std::make_unique<mln::style::Image>(std::move(image.baseImpl))
  );
  return MLN_STATUS_OK;
}

auto map_remove_style_image(MapObject& live, mln_buffer_view image_id)
  -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(image_id);
  if (!style.getImage(id).has_value()) {
    const auto message = "style image " + quote_id(id) + " does not exist";
    set_thread_error(message.c_str());
    return MLN_STATUS_NOT_FOUND;
  }
  style.removeImage(id);
  live.kept_style_images.erase(id);
  return MLN_STATUS_OK;
}

auto map_get_style_image(
  MapObject& live, mln_buffer_view image_id, StyleImageRecord& out_image
) -> bool {
  const auto id = string_from_view(image_id);
  const auto found = map_native(live).getStyle().getImage(id);
  if (!found) {
    return false;
  }
  out_image.keep_unused = live.kept_style_images.contains(id);

  // Image's copy shares the style's immutable image data. Image declares no
  // assignment, so the record constructs its copy in place.
  const auto& image = out_image.image.emplace(*found);
  out_image.fields =
    (image.getContent() ? MLN_STYLE_IMAGE_INFO_CONTENT : 0U) |
    (image.getTextFitWidth() ? MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH : 0U) |
    (image.getTextFitHeight() ? MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT : 0U);
  for (const auto& [stretches, out] : {
         std::pair{&image.getStretchX(), &out_image.stretch_x},
         std::pair{&image.getStretchY(), &out_image.stretch_y},
       }) {
    out->reserve(stretches->size());
    for (const auto& [from, to] : *stretches) {
      out->push_back(mln_image_stretch{.from = from, .to = to});
    }
  }
  return true;
}

auto style_image_info(const StyleImageRecord& record) noexcept
  -> mln_style_image_info {
  const auto& image = *record.image;
  const auto& pixels = image.getImage();
  const auto& content = image.getContent();
  const auto text_fit = [](const std::optional<mln::style::TextFit>& value) {
    return value ? from_native_text_fit(*value)
                 : MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK;
  };
  return mln_style_image_info{
    .width = pixels.size.width,
    .height = pixels.size.height,
    .fields = record.fields,
    .pixels = {.data = pixels.data.get(), .size = pixels.bytes()},
    .stretch_x = record.stretch_x.data(),
    .stretch_x_count = record.stretch_x.size(),
    .stretch_y = record.stretch_y.data(),
    .stretch_y_count = record.stretch_y.size(),
    .content =
      content ? mln_image_content{.left = content->left,
                                  .top = content->top,
                                  .right = content->right,
                                  .bottom = content->bottom}
              : mln_image_content{.left = 0, .top = 0, .right = 0, .bottom = 0},
    .text_fit_width = text_fit(image.getTextFitWidth()),
    .text_fit_height = text_fit(image.getTextFitHeight()),
    .pixel_ratio = image.getPixelRatio(),
    .sdf = image.isSdf(),
    .keep_unused = record.keep_unused
  };
}

auto map_add_image_source_image(
  MapObject& live, mln_buffer_view source_id, const mln_lat_lng* coordinates,
  mln::PremultipliedImage image
) -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(source_id);
  const auto add_status = validate_source_can_be_added(style, id);
  if (add_status != MLN_STATUS_OK) {
    return add_status;
  }

  auto source = std::make_unique<mln::style::ImageSource>(
    id, to_native_image_source_coordinates(coordinates)
  );
  source->setImage(std::move(image));
  style.addSource(std::move(source));
  return MLN_STATUS_OK;
}

auto map_set_image_source_url(
  MapObject& live, mln_buffer_view source_id, mln_buffer_view url
) -> mln_status {
  const auto id = string_from_view(source_id);
  mln::style::ImageSource* image_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), id, "an image source", image_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  image_source->setURL(string_from_view(url));
  return MLN_STATUS_OK;
}

auto map_set_image_source_image(
  MapObject& live, mln_buffer_view source_id, mln::PremultipliedImage image
) -> mln_status {
  const auto id = string_from_view(source_id);
  mln::style::ImageSource* image_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), id, "an image source", image_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  image_source->setImage(std::move(image));
  return MLN_STATUS_OK;
}

auto map_set_image_source_coordinates(
  MapObject& live, mln_buffer_view source_id, const mln_lat_lng* coordinates
) -> mln_status {
  const auto id = string_from_view(source_id);
  mln::style::ImageSource* image_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), id, "an image source", image_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  image_source->setCoordinates(to_native_image_source_coordinates(coordinates));
  return MLN_STATUS_OK;
}

auto map_get_image_source_coordinates(
  MapObject& live, mln_buffer_view source_id,
  std::vector<mln_lat_lng>& out_coordinates
) -> mln_status {
  out_coordinates.clear();
  const auto id = string_from_view(source_id);
  mln::style::ImageSource* image_source = nullptr;
  const auto status = find_source(
    map_native(live).getStyle(), id, "an image source", image_source
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }

  const auto coordinates =
    from_native_image_source_coordinates(image_source->getCoordinates());
  out_coordinates.assign(coordinates.begin(), coordinates.end());
  return MLN_STATUS_OK;
}

auto map_add_style_layer_json(
  MapObject& live, mln_buffer_view layer_json, mln_buffer_view before_layer_id
) -> mln_status {
  auto& style = map_native(live).getStyle();
  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (style.getLayer(*before) == nullptr) {
      const auto message =
        "before_layer_id " + quote_id(*before) + " does not exist";
      set_thread_error(message.c_str());
      return MLN_STATUS_NOT_FOUND;
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
    set_layer_error(id, "already exists");
    return MLN_STATUS_ALREADY_EXISTS;
  }
  if (
    (*layer)->getTypeInfo()->source ==
    mln::style::LayerTypeInfo::Source::Required
  ) {
    const auto* source = style.getSource((*layer)->getSourceID());
    if (source == nullptr) {
      set_layer_error(
        id, "uses source " + quote_id((*layer)->getSourceID()) +
              ", which does not exist"
      );
      return MLN_STATUS_NOT_FOUND;
    }
    if (!source->supportsLayerType((*layer)->getTypeInfo())) {
      const auto& type = (*layer)->getTypeInfo()->type;
      set_source_error(
        (*layer)->getSourceID(),
        "does not support " + std::string{type} + " layer " + quote_id(id)
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }

  style.addLayer(std::move(*layer), before);
  return MLN_STATUS_OK;
}

auto map_remove_style_layer(MapObject& live, mln_buffer_view layer_id)
  -> mln_status {
  const auto id = string_from_view(layer_id);
  auto removed = map_native(live).getStyle().removeLayer(id);
  if (removed == nullptr) {
    set_layer_error(id, "does not exist");
    return MLN_STATUS_NOT_FOUND;
  }
  return MLN_STATUS_OK;
}

namespace {

auto style_layer_record(const mln::style::Layer& layer) -> StyleLayerRecord {
  return {
    .id = layer.getID(),
    .type = layer.getTypeInfo()->type,
    .source_id = layer.getSourceID(),
    .source_layer = layer.getSourceLayer(),
    .min_zoom = static_cast<double>(layer.getMinZoom()),
    .max_zoom = static_cast<double>(layer.getMaxZoom()),
    .visibility = layer.getVisibility() == mln::style::VisibilityType::None
                    ? MLN_STYLE_LAYER_VISIBILITY_NONE
                    : MLN_STYLE_LAYER_VISIBILITY_VISIBLE
  };
}

}  // namespace

auto map_get_style_layer(
  MapObject& live, mln_buffer_view layer_id, StyleLayerRecord& out_layer
) -> bool {
  const auto* layer =
    map_native(live).getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) {
    return false;
  }
  out_layer = style_layer_record(*layer);
  return true;
}

auto map_list_style_layers(
  MapObject& live, std::vector<StyleLayerRecord>& layers
) -> mln_status {
  const auto style_layers = map_native(live).getStyle().getLayers();
  layers.reserve(style_layers.size());
  for (const auto* layer : style_layers) {
    layers.push_back(style_layer_record(*layer));
  }
  return MLN_STATUS_OK;
}

auto map_move_style_layer(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view before_layer_id
) -> mln_status {
  auto& style = map_native(live).getStyle();
  const auto id = string_from_view(layer_id);
  if (style.getLayer(id) == nullptr) {
    set_layer_error(id, "does not exist");
    return MLN_STATUS_NOT_FOUND;
  }

  auto before = std::optional<std::string>{};
  if (before_layer_id.size > 0) {
    before = string_from_view(before_layer_id);
    if (*before == id) {
      return MLN_STATUS_OK;
    }
    if (style.getLayer(*before) == nullptr) {
      const auto message =
        "before_layer_id " + quote_id(*before) + " does not exist";
      set_thread_error(message.c_str());
      return MLN_STATUS_NOT_FOUND;
    }
  }

  auto layer = style.removeLayer(id);
  style.addLayer(std::move(layer), before);
  return MLN_STATUS_OK;
}

auto map_get_style_layer_json(
  MapObject& live, mln_buffer_view layer_id, std::string& out_layer
) -> bool {
  const auto* layer =
    map_native(live).getStyle().getLayer(string_from_view(layer_id));
  if (layer == nullptr) return false;
  out_layer = serialize_json_value(layer->serialize());
  return true;
}

auto map_set_global_state_property(
  MapObject& live, mln_buffer_view property_name, mln_buffer_view value
) -> mln_status {
  auto native_value = to_native_json_value(value);
  if (!native_value) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  map_native(live).getStyle().setGlobalStateProperty(
    string_from_view(property_name), *native_value
  );
  return MLN_STATUS_OK;
}

auto map_get_global_state(MapObject& live) -> std::string {
  return serialize_json_value(
    mln::Value{map_native(live).getStyle().getGlobalState()}
  );
}

auto map_set_style_light_property(
  MapObject& live, mln_buffer_view property_name, mln_buffer_view value
) -> mln_status {
  const auto name = string_from_view(property_name);
  const auto context = "style light property " + quote_id(name);
  auto document = mln::JSDocument{};
  if (!parse_json_document(value, context.c_str(), document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto error = map_native(live).getStyle().getLight()->setProperty(
    name, mln::style::conversion::Convertible{
            static_cast<const mln::JSValue*>(&document)
          }
  );
  if (error) {
    set_style_conversion_error(context.c_str(), *error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto map_get_style_light_property(
  MapObject& live, mln_buffer_view property_name, std::string& out_value,
  bool& out_found
) -> mln_status {
  out_found = false;
  const auto property = map_native(live).getStyle().getLight()->getProperty(
    string_from_view(property_name)
  );
  if (property.getKind() == mln::style::StyleProperty::Kind::Undefined) {
    return MLN_STATUS_OK;
  }
  out_value = serialize_json_value(property.getValue());
  out_found = true;
  return MLN_STATUS_OK;
}

namespace {

// The style spec's root properties, other than `sources` and `layers`, which
// have calls of their own.
constexpr auto style_root_properties = std::to_array<std::string_view>({
  "version",
  "name",
  "metadata",
  "center",
  "centerAltitude",
  "zoom",
  "bearing",
  "pitch",
  "roll",
  "state",
  "light",
  "sky",
  "projection",
  "terrain",
  "sprite",
  "glyphs",
  "font-faces",
  "transition",
});

auto require_style_root_property(std::string_view name) -> bool {
  if (
    std::ranges::find(style_root_properties, name) !=
    style_root_properties.end()
  ) {
    return true;
  }
  const auto message =
    quote_id(name) + " is not a style root property that this call takes";
  set_thread_error(message.c_str());
  return false;
}

// Writes the members that are set, in the given order, so the text is stable.
template <typename Members>
auto object_json(const Members& members) -> std::optional<std::string> {
  auto buffer = rapidjson::StringBuffer{};
  auto writer = rapidjson::Writer<rapidjson::StringBuffer>{buffer};
  auto empty = true;
  writer.StartObject();
  for (const auto& [name, value] : members) {
    if (!value) continue;
    empty = false;
    writer.Key(name);
    mln::style::conversion::stringify(writer, *value);
  }
  writer.EndObject();
  if (empty) return std::nullopt;
  return std::string{buffer.GetString(), buffer.GetSize()};
}

auto light_json(const mln::style::Light& light) -> std::optional<std::string> {
  static constexpr auto names = std::to_array<const char*>({
    "anchor",
    "position",
    "color",
    "intensity",
    "anchor-transition",
    "position-transition",
    "color-transition",
    "intensity-transition",
  });
  auto members =
    std::vector<std::pair<const char*, std::optional<mln::Value>>>{};
  for (const auto* name : names) {
    auto property = light.getProperty(name);
    members.emplace_back(
      name, property.getKind() == mln::style::StyleProperty::Kind::Undefined
              ? std::nullopt
              : std::optional{property.getValue()}
    );
  }
  return object_json(members);
}

auto transition_json(const mln::style::TransitionOptions& transition)
  -> std::optional<std::string> {
  const auto milliseconds = [](
                              const std::optional<mln::Duration>& duration
                            ) -> std::optional<mln::Value> {
    if (!duration) return std::nullopt;
    return mln::Value{static_cast<int64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(*duration).count()
    )};
  };
  const auto members =
    std::to_array<std::pair<const char*, std::optional<mln::Value>>>({
      {"duration", milliseconds(transition.duration)},
      {"delay", milliseconds(transition.delay)},
    });
  return object_json(members);
}

}  // namespace

auto map_set_style_root_property(
  MapObject& live, mln_buffer_view property_name, mln_buffer_view value
) -> mln_status {
  const auto name = string_from_view(property_name);
  if (!require_style_root_property(name)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto context = "style root property " + quote_id(name);
  auto document = mln::JSDocument{};
  if (!parse_json_document(value, context.c_str(), document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto convertible = mln::style::conversion::Convertible{
    static_cast<const mln::JSValue*>(&document)
  };
  auto& style = map_native(live).getStyle();

  if (name == "light") {
    if (document.IsNull()) {
      style.setLight(std::make_unique<mln::style::Light>());
      return MLN_STATUS_OK;
    }
    auto error = mln::style::conversion::Error{};
    auto light =
      mln::style::conversion::convert<mln::style::Light>(convertible, error);
    if (!light) {
      set_style_conversion_error(context.c_str(), error);
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    style.setLight(std::make_unique<mln::style::Light>(std::move(light->impl)));
    return MLN_STATUS_OK;
  }

  if (name == "transition") {
    // Placement transitions aren't part of the style spec's transition, so
    // the setting keeps its current value.
    const auto placement =
      style.getTransitionOptions().enablePlacementTransitions;
    if (document.IsNull()) {
      // The transition that MapLibre gives a style without one.
      style.setTransitionOptions(
        mln::style::TransitionOptions{
          mln::util::DEFAULT_TRANSITION_DURATION, std::nullopt, placement
        }
      );
      return MLN_STATUS_OK;
    }
    auto error = mln::style::conversion::Error{};
    auto transition =
      mln::style::conversion::convert<mln::style::TransitionOptions>(
        convertible, error
      );
    if (!transition) {
      set_style_conversion_error(context.c_str(), error);
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    transition->enablePlacementTransitions = placement;
    style.setTransitionOptions(*transition);
    return MLN_STATUS_OK;
  }

  // MapLibre Native reads the other root properties only when it loads a
  // style, and has no globe, sky, or terrain to apply them to.
  const auto message = context + " can't change while the style is loaded";
  set_thread_error(message.c_str());
  return MLN_STATUS_UNSUPPORTED;
}

auto map_get_style_root_property(
  MapObject& live, mln_buffer_view property_name, std::string& out_value,
  bool& out_found
) -> mln_status {
  out_found = false;
  const auto name = string_from_view(property_name);
  if (!require_style_root_property(name)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto& style = map_native(live).getStyle();

  if (name == "light") {
    auto light = light_json(*style.getLight());
    if (light) {
      out_value = std::move(*light);
      out_found = true;
    }
    return MLN_STATUS_OK;
  }
  if (name == "transition") {
    auto transition = transition_json(style.getTransitionOptions());
    if (transition) {
      out_value = std::move(*transition);
      out_found = true;
    }
    return MLN_STATUS_OK;
  }

  // The other root properties can't change after the style loads, so the
  // loaded document holds their current values.
  auto document = mln::JSDocument{};
  const auto json = style.getJSON();
  document.Parse<0>(json.data(), json.size());
  if (document.HasParseError() || !document.IsObject()) {
    return MLN_STATUS_OK;
  }
  const auto member = document.FindMember(name.c_str());
  if (member == document.MemberEnd()) {
    return MLN_STATUS_OK;
  }
  auto buffer = rapidjson::StringBuffer{};
  auto writer = rapidjson::Writer<rapidjson::StringBuffer>{buffer};
  member->value.Accept(writer);
  out_value.assign(buffer.GetString(), buffer.GetSize());
  out_found = true;
  return MLN_STATUS_OK;
}

auto map_set_style_transition_options(
  MapObject& live, const mln_style_transition_options* options
) -> mln_status {
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

  map_native(live).getStyle().setTransitionOptions(native);
  return MLN_STATUS_OK;
}

auto map_get_style_transition_options(MapObject& live)
  -> mln_style_transition_options {
  const auto native = map_native(live).getStyle().getTransitionOptions();
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
  return result;
}

namespace {

// MapLibre's setProperty path otherwise logs a warning and ignores the field.
auto require_layer_takes_source(
  const mln::style::Layer& layer, const char* field
) -> bool {
  if (
    layer.getTypeInfo()->source == mln::style::LayerTypeInfo::Source::Required
  ) {
    return true;
  }
  const auto message = std::string{layer.getTypeInfo()->type} + " layer " +
                       quote_id(layer.getID()) + " does not take a " + field;
  set_thread_error(message.c_str());
  return false;
}

}  // namespace

auto map_set_layer_property(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view property_name,
  mln_buffer_view value
) -> mln_status {
  const auto id = string_from_view(layer_id);
  const auto name = string_from_view(property_name);
  const auto context =
    "property " + quote_id(name) + " of layer " + quote_id(id);
  auto document = mln::JSDocument{};
  if (!parse_json_document(value, context.c_str(), document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* layer = map_native(live).getStyle().getLayer(id);
  if (layer == nullptr) {
    set_layer_error(id, "does not exist");
    return MLN_STATUS_NOT_FOUND;
  }

  if (
    (name == "source" || name == "source-layer") &&
    !require_layer_takes_source(*layer, name.c_str())
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (document.IsNull()) {
    if (name == "filter") {
      layer->setFilter(mln::style::Filter{});
      return MLN_STATUS_OK;
    }
    if (name == "minzoom") {
      layer->setMinZoom(-std::numeric_limits<float>::infinity());
      return MLN_STATUS_OK;
    }
    if (name == "maxzoom") {
      layer->setMaxZoom(std::numeric_limits<float>::infinity());
      return MLN_STATUS_OK;
    }
  }

  auto error = layer->setProperty(
    name, mln::style::conversion::Convertible{
            static_cast<const mln::JSValue*>(&document)
          }
  );
  if (error) {
    set_style_conversion_error(context.c_str(), *error);
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

auto map_get_layer_property(
  MapObject& live, mln_buffer_view layer_id, mln_buffer_view property_name,
  std::string& out_value, bool& out_found
) -> mln_status {
  out_found = false;
  const auto id = string_from_view(layer_id);
  auto* layer = map_native(live).getStyle().getLayer(id);
  if (layer == nullptr) {
    set_layer_error(id, "does not exist");
    return MLN_STATUS_NOT_FOUND;
  }

  const auto name = string_from_view(property_name);
  auto root = mln::Value{};
  if (name == "filter") {
    if (!layer->getFilter()) {
      return MLN_STATUS_OK;
    }
    root = layer->getFilter().serialize();
  } else if (name == "source" || name == "source-layer") {
    auto source =
      name == "source" ? layer->getSourceID() : layer->getSourceLayer();
    if (source.empty()) {
      return MLN_STATUS_OK;
    }
    root = std::move(source);
  } else if (name == "minzoom" || name == "maxzoom") {
    const auto zoom =
      name == "minzoom" ? layer->getMinZoom() : layer->getMaxZoom();
    const auto unbounded = name == "minzoom"
                             ? -std::numeric_limits<float>::infinity()
                             : std::numeric_limits<float>::infinity();
    if (zoom == unbounded) {
      return MLN_STATUS_OK;
    }
    root = zoom;
  } else {
    const auto property = layer->getProperty(name);
    if (property.getKind() == mln::style::StyleProperty::Kind::Undefined) {
      return MLN_STATUS_OK;
    }
    root = property.getValue();
  }
  out_value = serialize_json_value(root);
  out_found = true;
  return MLN_STATUS_OK;
}

}  // namespace mln::core
