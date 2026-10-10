#pragma once

// Occupies a render session's driver for as long as a case needs.

#include <memory>

#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "support/test_support.h"

namespace mln::native_tests {

// A driver operation that parks inside the driver call until the case
// releases it. A core worker runs it as soon as it is queued; a caller driver
// runs it when the case services driver work. While it is parked the driver
// call is in flight, before the work publishes anything.
struct DriverBlocker {
  // Shared with the parked operation, so a case that fails while the driver
  // is parked does not leave it waiting on a dead stack frame.
  std::shared_ptr<mln_test_gate> gate = std::make_shared<mln_test_gate>();
  mln_test_completion completion{};

  // Queues the operation and reports the submission status.
  auto submit(mln_render_session session) -> mln_status {
    mln_test_gate_init(gate.get());
    completion = mln_test_completion_default(0);
    const auto status = mln::core::enqueue_driver_operation(
      session,
      [gate = gate](mln_render_session_object&) {
        mln_test_gate_park(gate.get());
        return MLN_STATUS_OK;
      },
      &completion.descriptor,
      mln::core::valueless_completion<&mln_render_session_reduce_memory_use>()
    );
    submitted = status == MLN_STATUS_OK;
    if (!submitted) {
      mln_test_completion_reject(&completion);
      mln_test_completion_destroy(&completion);
    }
    return status;
  }

  // Releases the driver and reports the operation's terminal status, servicing
  // a caller driver until it arrives.
  auto finish(const mln_test_render_fixture& fixture) -> mln_status {
    mln_test_gate_release(gate.get());
    if (!submitted) return MLN_STATUS_INVALID_STATE;
    const auto status =
      mln_test_render_fixture_finish_operation(&fixture, &completion);
    mln_test_completion_destroy(&completion);
    submitted = false;
    return status;
  }

  bool submitted = false;
};

}  // namespace mln::native_tests
