/*
 * The delivery queue declarations that the TypeScript addon runtime calls, for
 * a probe whose completions queue_stub.c dispatches. The real declarations are
 * in include/maplibre_native_c/delivery.h and wake.h.
 */
#ifndef MLN_PROBE_DELIVERY_H
#define MLN_PROBE_DELIVERY_H

#include "api.h"

typedef void (*mln_wake_callback)(void* user_data);
typedef struct mln_wake {
  mln_wake_callback callback;
  void* user_data;
  mln_user_data_release release_user_data;
  mln_delivery_queue queue;
} mln_wake;

typedef struct mln_delivery_queue_options {
  uint32_t size;
  mln_wake wake;
  mln_user_data_release release_abandoned_user_data;
} mln_delivery_queue_options;

mln_status mln_delivery_queue_create(
  const mln_delivery_queue_options* options, mln_delivery_queue* out_queue,
  mln_diagnostic* out_diagnostic
);
mln_status mln_delivery_queue_dispatch(
  mln_delivery_queue queue, mln_diagnostic* out_diagnostic
);
void mln_delivery_queue_release(mln_delivery_queue queue);

/**
 * Queues a completion's call with its result, as native code does for a
 * completion that names a queue, and wakes the queue when it was empty. The
 * probe's results point at static storage, so the copy borrows them.
 */
void mln_probe_queue_complete(
  const mln_completion* completion, const mln_completion_result* result
);

#endif
