#include <cstdint>
#include <optional>
#include <string>

#include <mln/style/conversion.hpp>
#include <mln/style/conversion/filter.hpp>
#include <mln/style/conversion/json.hpp>
#include <mln/style/filter.hpp>
#include <mln/tile/tile_id.hpp>

#include "style/style_value.hpp"

#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"

namespace mln::core {

auto to_native_style_filter(const mln_buffer_view* filter)
  -> std::optional<mln::style::Filter> {
  if (filter == nullptr) {
    return mln::style::Filter{};
  }
  if (!validate_bytes(*filter, "style filter")) {
    return std::nullopt;
  }
  auto error = mln::style::conversion::Error{};
  auto converted = mln::style::conversion::convertJSON<mln::style::Filter>(
    std::string{static_cast<const char*>(filter->data), filter->size}, error
  );
  if (!converted) {
    set_style_conversion_error("style filter", error);
  }
  return converted;
}

auto validate_canonical_tile_id(mln_canonical_tile_id tile_id) -> mln_status {
  if (tile_id.z > 32U) {
    set_thread_error("tile_id.z must be within [0, 32]");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinate_limit = uint64_t{1} << tile_id.z;
  if (tile_id.x >= coordinate_limit || tile_id.y >= coordinate_limit) {
    set_thread_error("tile_id x and y must be within zoom bounds");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_native_canonical_tile_id(mln_canonical_tile_id tile_id)
  -> mln::CanonicalTileID {
  return mln::CanonicalTileID{
    static_cast<uint8_t>(tile_id.z), tile_id.x, tile_id.y
  };
}

auto set_style_conversion_error(
  const char* context, const mln::style::conversion::Error& error
) -> void {
  auto message = std::string{context} + ": " + error.message;
  set_thread_error(message.c_str());
}

}  // namespace mln::core
