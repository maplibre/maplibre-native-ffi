#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <mln/style/conversion/geojson.hpp>
#include <mln/style/conversion/stringify.hpp>
#include <mln/style/conversion_impl.hpp>
#include <mln/style/rapidjson_conversion.hpp>
#include <mln/util/feature.hpp>
#include <mln/util/geo.hpp>
#include <mln/util/geojson.hpp>
#include <mln/util/geometry.hpp>
#include <mln/util/rapidjson.hpp>

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
  -> std::optional<mln::GeoJSON> {
  auto document = mln::JSDocument{};
  if (!mln::core::parse_json_document(bytes, name, document)) {
    return std::nullopt;
  }
  auto error = mln::style::conversion::Error{};
  auto converted = mln::style::conversion::convert<mln::GeoJSON>(
    static_cast<const mln::JSValue*>(&document), error
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
  const auto value =
    std::string_view{static_cast<const char*>(bytes.data), bytes.size};
  if (value.find('\0') != std::string_view::npos) {
    auto message = std::string{name} + " must not contain embedded NUL";
    set_thread_error(message.c_str());
    return false;
  }
  return true;
}

auto parse_json_document(
  mln_buffer_view bytes, const char* name, mln::JSDocument& out_document
) -> bool {
  if (!validate_bytes(bytes, name)) {
    return false;
  }
  out_document.Parse<0>(static_cast<const char*>(bytes.data), bytes.size);
  if (out_document.HasParseError()) {
    auto message = std::string{name} + " is not valid JSON: " +
                   rapidjson::GetParseError_En(out_document.GetParseError());
    set_thread_error(message.c_str());
    return false;
  }
  return true;
}

auto to_native_geometry(mln_buffer_view geometry)
  -> std::optional<mln::Geometry<double>> {
  auto converted = parse_geojson(geometry, "geometry");
  if (!converted) {
    return std::nullopt;
  }
  if (!converted->is<mln::Geometry<double>>()) {
    set_thread_error("geometry must contain one GeoJSON Geometry");
    return std::nullopt;
  }
  return converted->get<mln::Geometry<double>>();
}

auto to_native_json_value(mln_buffer_view value) -> std::optional<mln::Value> {
  auto document = mln::JSDocument{};
  if (!parse_json_document(value, "JSON value", document)) {
    return std::nullopt;
  }
  return mapbox::geojson::convert<mln::Value>(document);
}

auto to_native_feature(mln_buffer_view feature)
  -> std::optional<mln::GeoJSONFeature> {
  auto converted = parse_geojson(feature, "feature");
  if (!converted) {
    return std::nullopt;
  }
  if (!converted->is<mln::GeoJSONFeature>()) {
    set_thread_error("feature must contain one GeoJSON Feature");
    return std::nullopt;
  }
  return converted->get<mln::GeoJSONFeature>();
}

auto to_native_geojson(mln_buffer_view geojson) -> std::optional<mln::GeoJSON> {
  return parse_geojson(geojson, "GeoJSON");
}

auto serialize_json_value(const mln::Value& value) -> std::string {
  auto buffer = rapidjson::StringBuffer{};
  auto writer = rapidjson::Writer<rapidjson::StringBuffer>{buffer};
  mln::style::conversion::stringify(writer, value);
  return {buffer.GetString(), buffer.GetSize()};
}

auto serialize_geojson(const mln::GeoJSON& geojson) -> std::string {
  return mapbox::geojson::stringify(geojson);
}

auto serialize_feature_collection(const mln::FeatureCollection& features)
  -> std::string {
  return mapbox::geojson::stringify(features);
}

auto geometry_lat_lngs(const mln::Geometry<double>& geometry)
  -> std::vector<mln::LatLng> {
  auto result = std::vector<mln::LatLng>{};
  mln::forEachPoint(geometry, [&](const mln::Point<double>& point) -> void {
    result.emplace_back(point.y, point.x);
  });
  return result;
}

}  // namespace mln::core
