#pragma once

#include <memory>
#include <optional>

#include <mln/style/sources/geojson_source.hpp>
#include <mln/util/geojson.hpp>
#include <mln/util/immutable.hpp>

#include "handles/handle_table.hpp"
#include "maplibre_native_c/base.h"
#include "maplibre_native_c/style.h"

namespace mln::core {

// Immutable after creation, so any thread may read or destroy the prepared
// index. Installing it on a map remains ordered through a completion call.
struct GeoJsonSourceDataObject {
  std::shared_ptr<mln::style::GeoJSONData> data;
  mln::Immutable<mln::style::GeoJSONOptions> options;
};

template <>
struct HandleTraits<GeoJsonSourceDataObject> {
  static constexpr auto kind = HandleKind::GeoJsonSourceData;
  static constexpr auto leasable = true;
};

auto geojson_source_data_table() -> HandleTable<GeoJsonSourceDataObject>&;

auto validate_geojson_source_options(const mln_geo_json_source_options* options)
  -> mln_status;
auto effective_geojson_source_options(
  const mln_geo_json_source_options* options
) -> mln_geo_json_source_options;
auto to_native_geojson_source_options(
  const mln_geo_json_source_options& options
) -> std::optional<mln::Immutable<mln::style::GeoJSONOptions>>;

// Compares every scalar option; cluster_properties expressions carry no
// equality, so they stay outside the comparison.
auto geojson_source_options_equal(
  const mln::style::GeoJSONOptions& left,
  const mln::style::GeoJSONOptions& right
) -> bool;

auto geojson_source_data_create(
  mln_buffer_view data, const mln_geo_json_source_options* options,
  mln_geo_json_source_data* out_data
) -> mln_status;
auto geojson_source_data_release(mln_geo_json_source_data data) -> void;

}  // namespace mln::core
