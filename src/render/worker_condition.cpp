#include "render/worker_condition.hpp"

#if defined(__EMSCRIPTEN__)
#include <climits>

#include <emscripten/atomic.h>
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>
#endif

#if defined(MLN_RENDER_BACKEND_WEBGPU)
#include <emscripten/em_js.h>

// Suspends the calling thread through JSPI until `*word` no longer holds
// `seen`, so the thread's event loop runs meanwhile.
// clang-format off
EM_JS_DEPS(mln_worker_condition, "$polyfillWaitAsync");
EM_ASYNC_JS(void, mln_wait_for_notification, (const void* word, int32_t seen), {
  const waited = Atomics.waitAsync(HEAP32, word >> 2, seen);
  if (waited.async) await waited.value;
});
// clang-format on
#endif

namespace mln::core {

void WorkerCondition::notify_one() noexcept {
#if defined(__EMSCRIPTEN__)
  notifications_.fetch_add(1, std::memory_order_acq_rel);
  emscripten_futex_wake(&notifications_, INT_MAX);
#endif
  condition_.notify_one();
}

void WorkerCondition::notify_all() noexcept {
#if defined(__EMSCRIPTEN__)
  notifications_.fetch_add(1, std::memory_order_acq_rel);
  emscripten_futex_wake(&notifications_, INT_MAX);
#endif
  condition_.notify_all();
}

#if defined(__EMSCRIPTEN__)
auto WorkerCondition::notifications() const noexcept -> int32_t {
  return notifications_.load(std::memory_order_acquire);
}

void WorkerCondition::resume_after_notification(
  int32_t seen, void (*resume)(void*), void* user_data
) {
  resume_ = resume;
  resume_data_ = user_data;
  const auto resumed = [](void* opaque) {
    auto* condition = static_cast<WorkerCondition*>(opaque);
    condition->resume_(condition->resume_data_);
  };
  const auto waited = emscripten_atomic_wait_async(
    &notifications_, static_cast<uint32_t>(seen),
    [](int32_t*, uint32_t, ATOMICS_WAIT_RESULT_T, void* opaque) {
      auto* condition = static_cast<WorkerCondition*>(opaque);
      condition->resume_(condition->resume_data_);
    },
    this, EMSCRIPTEN_WAIT_ASYNC_INFINITY
  );
  // A notification since `seen` leaves nothing to wait for. The resume still
  // runs from the event loop, as a waited one does.
  if (waited == ATOMICS_WAIT_NOT_EQUAL) emscripten_async_call(resumed, this, 0);
}
#endif

void WorkerCondition::wait_on_core_worker_once(
  std::unique_lock<std::mutex>& lock
) {
#if defined(MLN_RENDER_BACKEND_WEBGPU)
  // Read under the lock, so a notification after the caller checked its
  // predicate changes the count and the wait returns at once.
  const auto seen = notifications_.load(std::memory_order_acquire);
  lock.unlock();
  mln_wait_for_notification(&notifications_, seen);
  lock.lock();
#else
  condition_.wait(lock);
#endif
}

}  // namespace mln::core
