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
  mln_buffer_view bytes, const char* name, mln::JSDocument& out_document
) -> bool;
auto to_native_geometry(mln_buffer_view geometry)
  -> std::optional<mln::Geometry<double>>;
auto to_native_json_value(mln_buffer_view value) -> std::optional<mln::Value>;
auto to_native_feature(mln_buffer_view feature)
  -> std::optional<mln::GeoJSONFeature>;
auto to_native_geojson(mln_buffer_view geojson) -> std::optional<mln::GeoJSON>;
auto serialize_json_value(const mln::Value& value) -> std::string;
auto serialize_geojson(const mln::GeoJSON& geojson) -> std::string;
auto serialize_feature_collection(const mln::FeatureCollection& features)
  -> std::string;
auto geometry_lat_lngs(const mln::Geometry<double>& geometry)
  -> std::vector<mln::LatLng>;

}  // namespace mln::core
