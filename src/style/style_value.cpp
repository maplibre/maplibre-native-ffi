#include <optional>
#include <string>

#include <mln/style/conversion.hpp>
#include <mln/style/conversion/filter.hpp>
#include <mln/style/conversion/json.hpp>
#include <mln/style/filter.hpp>

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

auto set_style_conversion_error(
  const char* context, const mln::style::conversion::Error& error
) -> void {
  auto message = std::string{context} + ": " + error.message;
  set_thread_error(message.c_str());
}

}  // namespace mln::core
