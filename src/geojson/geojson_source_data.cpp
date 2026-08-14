#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/style/conversion/geojson_options.hpp>  // IWYU pragma: keep
#include <mbgl/style/conversion/json.hpp>
#include <mbgl/style/conversion_impl.hpp>
#include <mbgl/util/geometry.hpp>
#include <mbgl/util/rapidjson.hpp>

#include "geojson/geojson_source_data.hpp"

#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "map/map.hpp"
#include "style/style_value.hpp"

namespace {

auto has_geojson_source_option(
  const mln_geojson_source_options& options, uint32_t field
) -> bool {
  return (options.fields & field) != 0U;
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
auto validate_clustered_geojson(const mbgl::GeoJSON& geojson) -> bool {
  if (!geojson.is<mbgl::FeatureCollection>()) {
    const auto message =
      std::string{
        "clustered GeoJSON data requires a feature collection; "
        "the data is "
      } +
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
      "clustered GeoJSON data requires point geometry on every feature; "
      "feature " +
      std::to_string(index) + " has " +
      std::string{geojson_geometry_type_name(geometry)} + " geometry";
    mln::core::set_thread_error(message.c_str());
    return false;
  }
  return true;
}

}  // namespace

namespace mln::core {

auto geojson_source_data_table() -> HandleTable<GeoJsonSourceDataObject>& {
  return handle_table<GeoJsonSourceDataObject>();
}

auto effective_geojson_source_options(const mln_geojson_source_options* options)
  -> mln_geojson_source_options {
  auto result = geojson_source_options_default();
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
      *options, MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING
    )
  ) {
    result.synchronous_tiling = options->synchronous_tiling;
  }
  return result;
}

auto validate_geojson_source_options(const mln_geojson_source_options* options)
  -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_geojson_source_options)) {
    set_thread_error("mln_geojson_source_options.size is too small");
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
    MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
  if ((options->fields & ~known_fields) != 0U) {
    set_thread_error("mln_geojson_source_options.fields contains unknown bits");
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
      set_thread_error(message.c_str());
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if (effective.min_zoom > effective.max_zoom) {
    set_thread_error("min_zoom must be less than or equal to max_zoom");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!std::isfinite(effective.tolerance) || effective.tolerance < 0.0) {
    set_thread_error("tolerance must be finite and non-negative");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.tile_size == 0 || effective.tile_size > 65535U) {
    set_thread_error("tile_size must be within [1, 65535]");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.buffer > 65535U) {
    set_thread_error("buffer must be at most 65535");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (effective.cluster_radius > 65535U) {
    set_thread_error("cluster_radius must be at most 65535");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    has_geojson_source_option(
      *options, MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
    ) &&
    effective.cluster_properties.size == 0
  ) {
    set_thread_error(
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
  native.synchronousUpdate = options.synchronous_tiling;

  if (options.cluster_properties.size != 0) {
    auto document = mbgl::JSDocument{};
    if (!parse_json_document(
          options.cluster_properties, "cluster_properties", document
        )) {
      return std::nullopt;
    }
    if (!document.IsObject()) {
      set_thread_error("cluster_properties must contain a JSON object");
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
      set_style_conversion_error("GeoJSON source options", error);
      return std::nullopt;
    }
    native.clusterProperties = std::move(converted->clusterProperties);
  }

  return mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(native));
}

auto geojson_source_options_equal(
  const mbgl::style::GeoJSONOptions& left,
  const mbgl::style::GeoJSONOptions& right
) -> bool {
  return left.minzoom == right.minzoom && left.maxzoom == right.maxzoom &&
         left.tileSize == right.tileSize && left.buffer == right.buffer &&
         left.tolerance == right.tolerance &&
         left.lineMetrics == right.lineMetrics &&
         left.cluster == right.cluster &&
         left.clusterRadius == right.clusterRadius &&
         left.clusterMaxZoom == right.clusterMaxZoom &&
         left.clusterMinPoints == right.clusterMinPoints &&
         left.synchronousUpdate == right.synchronousUpdate;
}

auto geojson_source_data_create(
  mln_buffer_view data, const mln_geojson_source_options* options,
  mln_geojson_source_data* out_data
) -> mln_status {
  if (out_data == nullptr) {
    set_thread_error("out_data must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_data != MLN_HANDLE_NULL) {
    set_thread_error("*out_data must be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto options_status = validate_geojson_source_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  auto geojson = to_native_geojson(data);
  if (!geojson) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto effective = effective_geojson_source_options(options);
  if (effective.cluster && !validate_clustered_geojson(*geojson)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto native_options = to_native_geojson_source_options(effective);
  if (!native_options) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto index = mbgl::style::GeoJSONData::create(
    *geojson, mbgl::Scheduler::GetSequenced(), *native_options
  );
  auto object =
    std::make_shared<GeoJsonSourceDataObject>(GeoJsonSourceDataObject{
      .data = std::move(index),
      .options = std::move(*native_options),
    });
  *out_data = geojson_source_data_table().insert(std::move(object));
  return MLN_STATUS_OK;
}

auto geojson_source_data_destroy(mln_geojson_source_data data) -> void {
  if (data == MLN_HANDLE_NULL) {
    return;
  }
  geojson_source_data_table().remove(data);
}

}  // namespace mln::core
