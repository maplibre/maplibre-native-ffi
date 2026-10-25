#pragma once

#include <exception>

#include "c_api/autorelease_pool.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/forbid_reentry.hpp"
#include "maplibre_native_c.h"

namespace mln::c_api {

// A zeroed struct with its size set, which is every option struct's default.
// Value-initializing first leaves no member out of a designated initializer.
template <typename Struct>
auto sized() noexcept -> Struct {
  auto value = Struct{};
  value.size = sizeof(Struct);
  return value;
}

// Runs native work and reports its diagnostic through out_diagnostic. Native
// code records the message in the calling thread's scratch buffer, which is
// cleared on entry and copied out before return. Native cleanup calls this
// directly, because a release that returns no status can reach it inside a
// host callback that forbids reentry, and refusing the cleanup would leak.
template <bool WithAutoreleasePool = true, typename Function>
auto native_boundary(mln_diagnostic* out_diagnostic, Function function) noexcept
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

// Runs one status-returning C entry point through native_boundary. Inside a
// host callback that forbids reentry, the entry point returns
// MLN_STATUS_INVALID_STATE without running, and the scratch buffer keeps what
// the native call that runs the callback recorded.
template <bool WithAutoreleasePool = true, typename Function>
auto status_boundary(mln_diagnostic* out_diagnostic, Function function) noexcept
  -> mln_status {
  if (const auto* refusal = mln::core::ForbidReentry::active()) {
    mln::core::write_diagnostic(out_diagnostic, refusal);
    return MLN_STATUS_INVALID_STATE;
  }
  return native_boundary<WithAutoreleasePool>(out_diagnostic, function);
}

}  // namespace mln::c_api
