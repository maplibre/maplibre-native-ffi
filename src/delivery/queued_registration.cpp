#include <memory>
#include <string>

#include "delivery/queued_registration.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::delivery {

auto resolve_registration_queue(
  const char* registration, mln_delivery_queue queue, bool has_callback,
  bool has_release, std::shared_ptr<DeliveryQueueState>& out_queue
) -> mln_status {
  const auto queued = queue != MLN_HANDLE_NULL;
  if (!has_callback && !queued) {
    core::set_thread_error((std::string{registration} +
                            " requires a callback or a delivery queue")
                             .c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!queued) return MLN_STATUS_OK;
  if (!has_callback && has_release) {
    core::set_thread_error((std::string{"a drained "} + registration +
                            " must not carry release_user_data")
                             .c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_queue = resolve_delivery_queue(queue);
  if (out_queue == nullptr) return core::recorded_handle_fault_status();
  return MLN_STATUS_OK;
}

}  // namespace mln::delivery
