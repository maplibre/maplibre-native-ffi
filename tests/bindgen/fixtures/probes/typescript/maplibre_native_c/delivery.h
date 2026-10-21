/*
 * The queue delivery declarations that the TypeScript addon runtime calls, for
 * a probe whose completions queue_stub.c delivers. The real declarations are
 * in include/maplibre_native_c/delivery.h and wake.h; the probe declares only
 * the completion and retirement types.
 */
#ifndef MLN_PROBE_DELIVERY_H
#define MLN_PROBE_DELIVERY_H

#include "api.h"

typedef void (*mln_wake_callback)(void* user_data);
typedef struct mln_wake {
  mln_wake_callback callback;
  void* user_data;
  mln_user_data_release release_user_data;
} mln_wake;

typedef unsigned long long mln_delivery_batch;

typedef enum mln_delivery_type : uint32_t {
  MLN_DELIVERY_TYPE_COMPLETION = 1,
  MLN_DELIVERY_TYPE_RETIRED = 4,
} mln_delivery_type;

typedef union mln_delivery_payload {
  mln_completion_result completion;
} mln_delivery_payload;

typedef struct mln_delivery {
  uint32_t type;
  uint32_t reserved;
  void* user_data;
  mln_delivery_payload payload;
} mln_delivery;

typedef struct mln_delivery_batch_view {
  uint32_t size;
  uint32_t delivery_size;
  const mln_delivery* deliveries;
  size_t delivery_count;
} mln_delivery_batch_view;

typedef struct mln_delivery_queue_options {
  uint32_t size;
  mln_wake wake;
} mln_delivery_queue_options;

mln_status mln_delivery_queue_create(
  const mln_delivery_queue_options* options, mln_delivery_queue* out_queue,
  mln_diagnostic* out_diagnostic
);
mln_status mln_delivery_queue_drain(
  mln_delivery_queue queue, mln_delivery_batch* out_batch,
  mln_delivery_batch_view* out_view, mln_diagnostic* out_diagnostic
);
mln_status mln_delivery_batch_take(
  mln_delivery_batch batch, size_t index, mln_diagnostic* out_diagnostic
);
void mln_delivery_batch_release(mln_delivery_batch batch);

/**
 * Queues a copy of a completion's result, as native code does for a
 * completion in queue mode, and wakes the queue when it was empty. The probe's
 * results point at static storage, so the copy borrows them.
 */
void mln_probe_queue_complete(
  const mln_completion* completion, const mln_completion_result* result
);

#endif
