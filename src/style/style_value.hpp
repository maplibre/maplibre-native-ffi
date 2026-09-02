#pragma once

#include <optional>

#include <mln/style/conversion.hpp>
#include <mln/style/filter.hpp>

#include "maplibre_native_c/base.h"

namespace mln::core {

auto to_native_style_filter(const mln_buffer_view* filter)
  -> std::optional<mln::style::Filter>;
auto set_style_conversion_error(
  const char* context, const mln::style::conversion::Error& error
) -> void;

}  // namespace mln::core
