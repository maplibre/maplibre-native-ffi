#ifndef MLN_NATIVE_TESTS_DELIVERY_H
#define MLN_NATIVE_TESTS_DELIVERY_H

// Helpers that the queue delivery cases share: a queue whose wake counts its
// calls, a log of the calls that a case's queues dispatch, waits for the next
// dispatched calls, and completions that a queue dispatches to the log.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "env.h"
#include "maplibre_native_c.h"
#include "resources.h"
#include "status.h"
#include "unity.h"
#include "wait.h"

// A queue and what its wake observed. The wake runs on native threads, so the
// counters are atomic.
typedef struct mln_test_delivery_queue {
  mln_delivery_queue handle;
  atomic_int wakes;
  atomic_int wake_releases;
} mln_test_delivery_queue;

static inline void mln_test_delivery_count_wake(void* user_data) {
  mln_test_delivery_queue* queue = (mln_test_delivery_queue*)user_data;
  atomic_fetch_add(&queue->wakes, 1);
  mln_test_pulse();
}

static inline void mln_test_delivery_count_wake_release(void* user_data) {
  mln_test_delivery_queue* queue = (mln_test_delivery_queue*)user_data;
  atomic_fetch_add(&queue->wake_releases, 1);
  mln_test_pulse();
}

// Creates a queue whose wake counts into queue, which must outlive it, and
// whose release passes the user_data it abandons to abandon.
static inline void mln_test_delivery_queue_create_abandoning(
  mln_test_delivery_queue* queue, mln_user_data_release abandon
) {
  atomic_store(&queue->wakes, 0);
  atomic_store(&queue->wake_releases, 0);
  queue->handle = MLN_HANDLE_NULL;
  const mln_delivery_queue_options options = {
    .size = sizeof(mln_delivery_queue_options),
    .wake =
      {
        .callback = mln_test_delivery_count_wake,
        .user_data = queue,
        .release_user_data = mln_test_delivery_count_wake_release,
      },
    .release_abandoned_user_data = abandon,
  };
  MLN_TEST_OK(mln_delivery_queue_create(&options, &queue->handle, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, queue->handle);
}

// Creates a queue whose release passes on none of the user_data it abandons.
static inline void mln_test_delivery_queue_create(
  mln_test_delivery_queue* queue
) {
  mln_test_delivery_queue_create_abandoning(queue, NULL);
}

// Releases the queue and waits for its wake's release. A push that is still
// in its wake call releases the wake when that call returns, so the counters
// must stay alive until then.
static inline void mln_test_delivery_queue_release(
  mln_test_delivery_queue* queue
) {
  mln_delivery_queue_release(queue->handle);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue->wake_releases, 1));
}

// The calls that this file's queues dispatched, in the order they ran. A
// case dispatches on its own thread, so nothing guards the log. Callbacks
// record instead of asserting, since a failed assertion would unwind through
// the dispatch that runs them.
enum { mln_test_max_calls = 128 };

typedef struct mln_test_call {
  // 'c' for a completion and 'r' for a release; a case adds its own kinds.
  char kind;
  void* user_data;
  mln_status status;
  uint64_t generation;
} mln_test_call;

static struct {
  size_t count;
  mln_test_call calls[mln_test_max_calls];
} mln_test_calls;

static inline void mln_test_calls_reset(void) { mln_test_calls.count = 0; }

static inline void mln_test_record_call(
  char kind, void* user_data, mln_status status, uint64_t generation
) {
  if (mln_test_calls.count < mln_test_max_calls) {
    mln_test_calls.calls[mln_test_calls.count] = (mln_test_call){
      .kind = kind,
      .user_data = user_data,
      .status = status,
      .generation = generation,
    };
  }
  mln_test_calls.count += 1;
}

static inline const mln_test_call* mln_test_call_at(size_t index) {
  TEST_ASSERT_LESS_THAN_size_t(mln_test_calls.count, index);
  TEST_ASSERT_LESS_THAN_size_t(mln_test_max_calls, index);
  return &mln_test_calls.calls[index];
}

static inline size_t mln_test_count_calls(char kind, void* user_data) {
  size_t total = 0;
  for (size_t index = 0;
       index < mln_test_calls.count && index < mln_test_max_calls; ++index) {
    const mln_test_call* call = &mln_test_calls.calls[index];
    if (call->kind == kind && call->user_data == user_data) total += 1;
  }
  return total;
}

static inline void mln_test_record_completion(
  void* user_data, const mln_completion_result* result
) {
  mln_test_record_call('c', user_data, result->status, result->generation);
}

static inline void mln_test_record_release(void* user_data) {
  mln_test_record_call('r', user_data, MLN_STATUS_OK, 0);
}

typedef struct mln_test_call_wait {
  mln_delivery_queue queue;
  size_t count;
  mln_status failure;
} mln_test_call_wait;

static inline bool mln_test_dispatched_calls(void* context) {
  mln_test_call_wait* wait = (mln_test_call_wait*)context;
  const mln_status status = mln_delivery_queue_dispatch(wait->queue, NULL);
  if (status != MLN_STATUS_OK && status != MLN_STATUS_NOT_READY) {
    wait->failure = status;
    return true;
  }
  return mln_test_calls.count >= wait->count;
}

// Dispatches queue as its wake pulses until the log holds `count` calls.
static inline void mln_test_dispatch_until_calls(
  mln_delivery_queue queue, size_t count
) {
  mln_test_call_wait wait = {
    .queue = queue, .count = count, .failure = MLN_STATUS_OK
  };
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_await(
      mln_test_dispatched_calls, &wait, mln_test_deadline_default(),
      "dispatched calls"
    ),
    "the queue did not run the calls"
  );
  MLN_TEST_OK(wait.failure);
}

// A completion that queue dispatches to mln_test_record_completion with
// user_data.
static inline mln_completion mln_test_queue_completion(
  mln_delivery_queue queue, void* user_data
) {
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = mln_test_record_completion,
    .user_data = user_data,
    .queue = queue,
  };
}

// Reports whether a map handle is still live. Disposal consumes a handle before
// it returns, so a disposed map answers as stale at once.
static inline bool mln_test_map_is_live(mln_map map) {
  return mln_test_map_request_repaint(map) == MLN_STATUS_OK;
}

#endif
