#include "render/worker_condition.hpp"

#if defined(MLN_RENDER_BACKEND_WEBGPU)
#include <climits>

#include <emscripten/em_js.h>
#include <emscripten/threading.h>

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
#if defined(MLN_RENDER_BACKEND_WEBGPU)
  notifications_.fetch_add(1, std::memory_order_acq_rel);
  emscripten_futex_wake(&notifications_, INT_MAX);
#endif
  condition_.notify_one();
}

void WorkerCondition::notify_all() noexcept {
#if defined(MLN_RENDER_BACKEND_WEBGPU)
  notifications_.fetch_add(1, std::memory_order_acq_rel);
  emscripten_futex_wake(&notifications_, INT_MAX);
#endif
  condition_.notify_all();
}

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
