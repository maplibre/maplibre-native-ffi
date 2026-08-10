#pragma once

#include <optional>
#include <string>
#include <vector>

#include <mbgl/util/feature.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/geojson.hpp>
#include <mbgl/util/geometry.hpp>
#include <mbgl/util/rapidjson.hpp>

#include "maplibre_native_c/base.h"

namespace mln::core {

auto validate_bytes(mln_buffer_view bytes, const char* name) -> bool;
auto parse_json_document(
  mln_buffer_view bytes, const char* name, mbgl::JSDocument& out_document
) -> bool;
auto to_native_geometry(mln_buffer_view geometry)
  -> std::optional<mbgl::Geometry<double>>;
auto to_native_json_value(mln_buffer_view value) -> std::optional<mbgl::Value>;
auto to_native_feature(mln_buffer_view feature)
  -> std::optional<mbgl::GeoJSONFeature>;
auto to_native_geojson(mln_buffer_view geojson) -> std::optional<mbgl::GeoJSON>;
auto serialize_json_value(const mbgl::Value& value) -> std::string;
auto serialize_geojson(const mbgl::GeoJSON& geojson) -> std::string;
auto serialize_feature_collection(const mbgl::FeatureCollection& features)
  -> std::string;
auto geometry_lat_lngs(const mbgl::Geometry<double>& geometry)
  -> std::vector<mbgl::LatLng>;

}  // namespace mln::core
