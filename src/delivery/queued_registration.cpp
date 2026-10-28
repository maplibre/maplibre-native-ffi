#include <memory>
#include <string>

#include "delivery/queued_registration.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::delivery {

auto resolve_registration_queue(
  const char* registration, mln_delivery_queue queue, bool has_callback,
  std::shared_ptr<DeliveryQueueState>& out_queue
) -> mln_status {
  if (!has_callback) {
    core::set_thread_error(
      (std::string{registration} + " requires a callback").c_str()
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (queue == MLN_HANDLE_NULL) return MLN_STATUS_OK;
  out_queue = resolve_delivery_queue(queue);
  if (out_queue == nullptr) return core::recorded_handle_fault_status();
  return MLN_STATUS_OK;
}

}  // namespace mln::delivery
