#include <optional>
#include <string>
#include <vector>

#include <mbgl/style/conversion/geojson.hpp>
#include <mbgl/style/conversion/stringify.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/geojson.hpp>
#include <mbgl/util/geometry.hpp>
#include <mbgl/util/rapidjson.hpp>

#include <mapbox/geojson.hpp>
#include <mapbox/geojson/rapidjson.hpp>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "geojson/geojson.hpp"

#include "diagnostics/diagnostics.hpp"

namespace {

auto parse_geojson(mln_buffer_view bytes, const char* name)
  -> std::optional<mbgl::GeoJSON> {
  if (!mln::core::validate_bytes(bytes, name)) {
    return std::nullopt;
  }
  auto error = mbgl::style::conversion::Error{};
  auto converted = mbgl::style::conversion::parseGeoJSON(
    std::string{static_cast<const char*>(bytes.data), bytes.size}, error
  );
  if (!converted) {
    auto message = std::string{name} + " is invalid: " + error.message;
    mln::core::set_thread_error(message.c_str());
  }
  return converted;
}

}  // namespace

namespace mln::core {

auto validate_bytes(mln_buffer_view bytes, const char* name) -> bool {
  if (bytes.size == 0) {
    auto message = std::string{name} + " must not be empty";
    set_thread_error(message.c_str());
    return false;
  }
  if (bytes.data == nullptr) {
    auto message = std::string{name} + " data must not be null";
    set_thread_error(message.c_str());
    return false;
  }
  return true;
}

auto parse_json_document(
  mln_buffer_view bytes, const char* name, mbgl::JSDocument& out_document
) -> bool {
  if (!validate_bytes(bytes, name)) {
    return false;
  }
  const auto json =
    std::string{static_cast<const char*>(bytes.data), bytes.size};
  out_document.Parse<0>(json.c_str());
  if (out_document.HasParseError()) {
    auto message = std::string{name} + " is not valid JSON: " +
                   rapidjson::GetParseError_En(out_document.GetParseError());
    set_thread_error(message.c_str());
    return false;
  }
  return true;
}

auto to_native_geometry(mln_buffer_view geometry)
  -> std::optional<mbgl::Geometry<double>> {
  auto converted = parse_geojson(geometry, "geometry");
  if (!converted) {
    return std::nullopt;
  }
  if (!converted->is<mbgl::Geometry<double>>()) {
    set_thread_error("geometry must contain one GeoJSON Geometry");
    return std::nullopt;
  }
  return converted->get<mbgl::Geometry<double>>();
}

auto to_native_json_value(mln_buffer_view value) -> std::optional<mbgl::Value> {
  auto document = mbgl::JSDocument{};
  if (!parse_json_document(value, "JSON value", document)) {
    return std::nullopt;
  }
  return mapbox::geojson::convert<mbgl::Value>(document);
}

auto to_native_feature(mln_buffer_view feature)
  -> std::optional<mbgl::GeoJSONFeature> {
  auto converted = parse_geojson(feature, "feature");
  if (!converted) {
    return std::nullopt;
  }
  if (!converted->is<mbgl::GeoJSONFeature>()) {
    set_thread_error("feature must contain one GeoJSON Feature");
    return std::nullopt;
  }
  return converted->get<mbgl::GeoJSONFeature>();
}

auto to_native_geojson(mln_buffer_view geojson)
  -> std::optional<mbgl::GeoJSON> {
  return parse_geojson(geojson, "GeoJSON");
}

auto serialize_json_value(const mbgl::Value& value) -> std::string {
  auto buffer = rapidjson::StringBuffer{};
  auto writer = rapidjson::Writer<rapidjson::StringBuffer>{buffer};
  mbgl::style::conversion::stringify(writer, value);
  return {buffer.GetString(), buffer.GetSize()};
}

auto serialize_geojson(const mbgl::GeoJSON& geojson) -> std::string {
  return mapbox::geojson::stringify(geojson);
}

auto serialize_feature_collection(const mbgl::FeatureCollection& features)
  -> std::string {
  return mapbox::geojson::stringify(features);
}

auto geometry_lat_lngs(const mbgl::Geometry<double>& geometry)
  -> std::vector<mbgl::LatLng> {
  auto result = std::vector<mbgl::LatLng>{};
  mbgl::forEachPoint(geometry, [&](const mbgl::Point<double>& point) -> void {
    result.emplace_back(point.y, point.x);
  });
  return result;
}

}  // namespace mln::core
