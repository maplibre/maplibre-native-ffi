#define MLN_BUILDING_C

#include "maplibre_native_c/operation.h"

#include "c_api/boundary.hpp"
#include "operation/operation.hpp"

extern "C" MLN_API auto mln_operation_poll(
  mln_operation operation, bool* out_completed
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::poll_operation(operation, out_completed);
  });
}

extern "C" MLN_API auto mln_operation_wait(
  mln_operation operation, int64_t timeout_ms, bool* out_completed
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::wait_operation(operation, timeout_ms, out_completed);
  });
}

extern "C" MLN_API auto mln_operation_cancel(mln_operation operation) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::cancel_operation(operation);
  });
}

extern "C" MLN_API auto mln_operation_get_status(
  mln_operation operation, mln_status* out_status
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::get_operation_status(operation, out_status);
  });
}

extern "C" MLN_API auto mln_operation_copy_diagnostic(
  mln_operation operation, char* out_diagnostic, size_t diagnostic_capacity,
  size_t* out_diagnostic_size
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::copy_operation_diagnostic(
      operation, out_diagnostic, diagnostic_capacity, out_diagnostic_size
    );
  });
}

extern "C" MLN_API auto mln_operation_finish(mln_operation operation) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::finish_operation(operation);
  });
}

extern "C" MLN_API void mln_operation_release(
  mln_operation operation
) noexcept {
  mln::core::release_operation(operation);
}
