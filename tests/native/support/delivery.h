#ifndef MLN_NATIVE_TESTS_DELIVERY_H
#define MLN_NATIVE_TESTS_DELIVERY_H

// Helpers that the queue delivery cases share: a queue whose wake counts its
// calls, drains that keep their views, waits for the next drained batch,
// indexed access by the delivery stride, and queue-mode completions.

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

// Creates a queue whose wake counts into queue, which must outlive it.
static inline void mln_test_delivery_queue_create(
  mln_test_delivery_queue* queue
) {
  atomic_store(&queue->wakes, 0);
  atomic_store(&queue->wake_releases, 0);
  queue->handle = MLN_HANDLE_NULL;
  const mln_delivery_queue_options options = {
    .size = sizeof(mln_delivery_queue_options),
    .wake = {
      .callback = mln_test_delivery_count_wake,
      .user_data = queue,
      .release_user_data = mln_test_delivery_count_wake_release,
    },
  };
  MLN_TEST_OK(mln_delivery_queue_create(&options, &queue->handle, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, queue->handle);
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

// One drained batch and the view that the drain wrote.
typedef struct mln_test_delivery_drain {
  mln_delivery_queue queue;
  mln_delivery_batch batch;
  mln_delivery_batch_view view;
} mln_test_delivery_drain;

// Drains queue into drain, checking the view on success.
static inline mln_status mln_test_delivery_drain_into(
  mln_delivery_queue queue, mln_test_delivery_drain* drain
) {
  drain->queue = queue;
  drain->batch = MLN_HANDLE_NULL;
  drain->view = (mln_delivery_batch_view){
    .size = sizeof(mln_delivery_batch_view),
  };
  const mln_status status =
    mln_delivery_queue_drain(queue, &drain->batch, &drain->view, NULL);
  if (status == MLN_STATUS_OK) {
    TEST_ASSERT_TRUE(drain->view.delivery_size >= sizeof(mln_delivery));
  }
  return status;
}

static inline bool mln_test_delivery_try_drain(void* context) {
  mln_test_delivery_drain* drain = (mln_test_delivery_drain*)context;
  const mln_status status = mln_test_delivery_drain_into(drain->queue, drain);
  if (status == MLN_STATUS_NOT_READY) return false;
  MLN_TEST_OK(status);
  return true;
}

// Waits for the queue to hold a delivery and drains it. The wait re-checks on
// every pulse, which the counting wake sends.
static inline mln_test_delivery_drain mln_test_delivery_await_batch(
  mln_delivery_queue queue
) {
  mln_test_delivery_drain drain = {.queue = queue};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_await(
      mln_test_delivery_try_drain, &drain, mln_test_deadline_default(),
      "delivery batch"
    ),
    "no delivery arrived"
  );
  return drain;
}

// Steps to one delivery by the stride that native reports.
static inline const mln_delivery* mln_test_delivery_at(
  const mln_delivery_batch_view* view, size_t index
) {
  TEST_ASSERT_TRUE(index < view->delivery_count);
  return (const mln_delivery*)((const unsigned char*)view->deliveries +
                               index * view->delivery_size);
}

// Waits for one batch that holds exactly one delivery and returns it.
static inline mln_delivery_batch mln_test_delivery_await_single(
  mln_delivery_queue queue, const mln_delivery** out_delivery
) {
  const mln_test_delivery_drain drain = mln_test_delivery_await_batch(queue);
  TEST_ASSERT_EQUAL_size_t(1, drain.view.delivery_count);
  *out_delivery = mln_test_delivery_at(&drain.view, 0);
  return drain.batch;
}

// A completion that delivers to queue with user_data.
static inline mln_completion mln_test_queue_completion(
  mln_delivery_queue queue, void* user_data
) {
  return (mln_completion){
    .size = sizeof(mln_completion),
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
