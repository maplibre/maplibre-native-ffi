#pragma once

#include <exception>

#include "c_api/autorelease_pool.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

namespace mln::c_api {

// Runs one C entry point and reports its diagnostic through out_diagnostic.
// Native code records the message in the calling thread's scratch buffer,
// which is cleared on entry and copied out before return.
template <bool WithAutoreleasePool = true, typename Function>
auto status_boundary(mln_diagnostic* out_diagnostic, Function function) noexcept
  -> mln_status {
  mln::core::clear_thread_error();
  auto status = MLN_STATUS_NATIVE_ERROR;
  try {
    if constexpr (WithAutoreleasePool) {
      status = with_autorelease_pool(function);
    } else {
      status = function();
    }
  } catch (const std::exception& exception) {
    mln::core::set_thread_error(exception);
  } catch (...) {
    mln::core::set_thread_error("unknown native exception");
  }
  mln::core::write_diagnostic(
    out_diagnostic,
    status == MLN_STATUS_OK ? "" : mln::core::thread_last_error_message()
  );
  return status;
}

}  // namespace mln::c_api
