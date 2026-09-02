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

// Prepared GeoJSON source data: a parsed document tiled or clustered into the
// index mln::style::GeoJSONSource consumes, together with the options the
// index was built with. Immutable after creation, so any thread may create,
// read, or destroy one; installing on a map stays an owner-thread call.
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

auto validate_geojson_source_options(const mln_geojson_source_options* options)
  -> mln_status;
auto effective_geojson_source_options(const mln_geojson_source_options* options)
  -> mln_geojson_source_options;
auto to_native_geojson_source_options(const mln_geojson_source_options& options)
  -> std::optional<mln::Immutable<mln::style::GeoJSONOptions>>;

// Compares every scalar option; cluster_properties expressions carry no
// equality, so they stay outside the comparison.
auto geojson_source_options_equal(
  const mln::style::GeoJSONOptions& left,
  const mln::style::GeoJSONOptions& right
) -> bool;

auto geojson_source_data_create(
  mln_buffer_view data, const mln_geojson_source_options* options,
  mln_geojson_source_data* out_data
) -> mln_status;
auto geojson_source_data_destroy(mln_geojson_source_data data) -> void;

}  // namespace mln::core
