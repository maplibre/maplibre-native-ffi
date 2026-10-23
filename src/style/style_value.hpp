#pragma once

#include <optional>

#include <mln/style/conversion.hpp>
#include <mln/style/filter.hpp>
#include <mln/tile/tile_id.hpp>

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/style.h"

namespace mln::core {

auto to_native_style_filter(const mln_buffer_view* filter)
  -> std::optional<mln::style::Filter>;
auto validate_canonical_tile_id(mln_canonical_tile_id tile_id) -> mln_status;
auto to_native_canonical_tile_id(mln_canonical_tile_id tile_id)
  -> mln::CanonicalTileID;
auto set_style_conversion_error(
  const char* context, const mln::style::conversion::Error& error
) -> void;

}  // namespace mln::core
