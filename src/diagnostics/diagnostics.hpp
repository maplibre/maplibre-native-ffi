#pragma once

#include <exception>

#include "maplibre_native_c/base.h"

namespace mln::core {

auto thread_last_error_message() noexcept -> const char*;
auto clear_thread_error() noexcept -> void;
auto set_thread_error(const char* message) noexcept -> void;
auto set_thread_error(const std::exception& exception) noexcept -> void;
auto write_diagnostic(mln_diagnostic* diagnostic, const char* message) noexcept
  -> void;

}  // namespace mln::core
