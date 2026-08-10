#pragma once

#include <optional>

#include <mbgl/style/conversion.hpp>
#include <mbgl/style/filter.hpp>

#include "maplibre_native_c/base.h"

namespace mln::core {

auto to_native_style_filter(const mln_buffer_view* filter)
  -> std::optional<mbgl::style::Filter>;
auto set_style_conversion_error(
  const char* context, const mbgl::style::conversion::Error& error
) -> void;

}  // namespace mln::core
