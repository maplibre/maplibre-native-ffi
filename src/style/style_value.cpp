#include <optional>
#include <string>

#include <mbgl/style/conversion.hpp>
#include <mbgl/style/conversion/filter.hpp>
#include <mbgl/style/conversion/json.hpp>
#include <mbgl/style/filter.hpp>

#include "style/style_value.hpp"

#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"

namespace mln::core {

auto to_native_style_filter(const mln_buffer_view* filter)
  -> std::optional<mbgl::style::Filter> {
  if (filter == nullptr) {
    return mbgl::style::Filter{};
  }
  if (!validate_bytes(*filter, "style filter")) {
    return std::nullopt;
  }
  auto error = mbgl::style::conversion::Error{};
  auto converted = mbgl::style::conversion::convertJSON<mbgl::style::Filter>(
    std::string{static_cast<const char*>(filter->data), filter->size}, error
  );
  if (!converted) {
    set_style_conversion_error("style filter", error);
  }
  return converted;
}

auto set_style_conversion_error(
  const char* context, const mbgl::style::conversion::Error& error
) -> void {
  auto message = std::string{context} + ": " + error.message;
  set_thread_error(message.c_str());
}

}  // namespace mln::core
