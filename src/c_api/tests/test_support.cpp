#include <any>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "test_support.h"

#include "c_api/boundary.hpp"
#include "handles/handle_table.hpp"
#include "notification/notification.hpp"
#include "operation/operation.hpp"

struct operation_cancel_state {
  std::atomic_uint count = 0;
  atomic_bool* entered = nullptr;
  const atomic_bool* release = nullptr;
};

struct mln_test_operation_control {
  std::shared_ptr<mln::core::OperationObject> operation;
  std::shared_ptr<operation_cancel_state> cancel_state;
};

struct mln_test_endpoint_control {
  std::shared_ptr<mln::core::NotificationEndpoint> endpoint;
};

extern "C" mln_status mln_test_operation_create(
  mln_notification_source source, bool cancellable,
  mln_operation* out_operation, mln_test_operation_control** out_control
) {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_control == nullptr || *out_control != nullptr) {
      mln::core::set_thread_error("out_control must point to null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto notification_source =
      mln::core::notification_source_from_handle(source);
    if (notification_source == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto control = std::make_unique<mln_test_operation_control>();
    control->cancel_state = std::make_shared<operation_cancel_state>();
    const auto cancel_state = control->cancel_state;
    auto cancel = mln::core::OperationObject::CancelCallback{};
    if (cancellable) {
      cancel = [cancel_state]() noexcept {
        cancel_state->count.fetch_add(1, std::memory_order_relaxed);
        if (cancel_state->entered == nullptr) {
          return;
        }
        cancel_state->entered->store(true, std::memory_order_release);
        while (!cancel_state->release->load(std::memory_order_acquire)) {
          std::this_thread::yield();
        }
      };
    }
    const auto status = mln::core::register_operation(
      notification_source, UINT32_C(0xFFFF), cancellable, std::move(cancel),
      out_operation, control->operation
    );
    if (status != MLN_STATUS_OK) {
      return status;
    }
    *out_control = control.release();
    return MLN_STATUS_OK;
  });
}

extern "C" void mln_test_operation_complete(
  mln_test_operation_control* control, mln_status status, const char* diagnostic
) {
  if (control == nullptr || control->operation == nullptr) {
    return;
  }
  control->operation->complete(
    status, diagnostic == nullptr ? std::string{} : std::string{diagnostic},
    std::any{std::monostate{}}
  );
}

extern "C" unsigned int mln_test_operation_cancel_count(
  const mln_test_operation_control* control
) {
  return control == nullptr
           ? 0
           : control->cancel_state->count.load(std::memory_order_relaxed);
}

extern "C" void mln_test_operation_block_cancel(
  mln_test_operation_control* control, atomic_bool* entered,
  const atomic_bool* release
) {
  if (control == nullptr) {
    return;
  }
  control->cancel_state->entered = entered;
  control->cancel_state->release = release;
}

extern "C" void mln_test_operation_control_destroy(
  mln_test_operation_control* control
) {
  delete control;
}

extern "C" mln_status mln_test_endpoint_create(
  mln_notification_source source, uint64_t id, uint32_t kind, bool sticky,
  mln_test_endpoint_control** out_control
) {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_control == nullptr || *out_control != nullptr) {
      mln::core::set_thread_error("out_control must point to null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto notification_source =
      mln::core::notification_source_from_handle(source);
    if (notification_source == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto endpoint = notification_source->associate(id, kind, sticky);
    if (endpoint == nullptr) {
      return MLN_STATUS_INVALID_STATE;
    }
    auto control = std::make_unique<mln_test_endpoint_control>();
    control->endpoint = std::move(endpoint);
    *out_control = control.release();
    return MLN_STATUS_OK;
  });
}

extern "C" void mln_test_endpoint_mark_ready(
  mln_test_endpoint_control* control
) {
  if (control != nullptr && control->endpoint != nullptr) {
    control->endpoint->mark_ready();
  }
}

extern "C" void mln_test_endpoint_clear_ready(
  mln_test_endpoint_control* control
) {
  if (control != nullptr && control->endpoint != nullptr) {
    control->endpoint->clear_ready();
  }
}

extern "C" void mln_test_endpoint_control_destroy(
  mln_test_endpoint_control* control
) {
  delete control;
}

extern "C" void mln_test_hold_notification_ready_drain(
  mln_notification_source source, atomic_bool* entered,
  const atomic_bool* release
) {
  const auto live = mln::core::notification_source_from_handle(source);
  if (live == nullptr) {
    entered->store(true, std::memory_order_release);
    return;
  }
  auto endpoints = std::vector<mln_ready_endpoint>{};
  if (live->begin_ready_drain(endpoints) != MLN_STATUS_OK) {
    entered->store(true, std::memory_order_release);
    return;
  }
  entered->store(true, std::memory_order_release);
  while (!release->load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  live->abort_ready_drain();
}
