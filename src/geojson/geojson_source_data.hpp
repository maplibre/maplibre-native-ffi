#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <mln/style/sources/geojson_source.hpp>
#include <mln/util/geojson.hpp>
#include <mln/util/immutable.hpp>

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/style.h"

namespace mln::core {

auto validate_geojson_source_options(const mln_geo_json_source_options* options)
  -> mln_status;
// Validates `options`, null for the defaults, and converts them for MapLibre.
auto native_geojson_source_options(const mln_geo_json_source_options* options)
  -> std::optional<mln::Immutable<mln::style::GeoJSONOptions>>;
auto effective_geojson_source_options(
  const mln_geo_json_source_options* options
) -> mln_geo_json_source_options;
auto to_native_geojson_source_options(
  const mln_geo_json_source_options& options
) -> std::optional<mln::Immutable<mln::style::GeoJSONOptions>>;

// Reports a source's options with every field set. The result's
// cluster_properties views out_cluster_properties, which holds the JSON.
auto from_native_geojson_source_options(
  const mln::style::GeoJSONOptions& options, std::string& out_cluster_properties
) -> mln_geo_json_source_options;

// GeoJSON data prepared for a source: the index, or why preparing failed.
struct PreparedGeoJsonSourceData {
  mln_status status = MLN_STATUS_OK;
  std::string message;
  std::shared_ptr<mln::style::GeoJSONData> data;
};

// Parses `data` and builds its index with `options`. Any thread may call it,
// and it reports failures in its result instead of the thread's error.
auto prepare_geojson_source_data(
  std::string_view data,
  const mln::Immutable<mln::style::GeoJSONOptions>& options
) -> PreparedGeoJsonSourceData;

}  // namespace mln::core
