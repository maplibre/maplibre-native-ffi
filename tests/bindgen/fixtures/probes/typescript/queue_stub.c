/*
 * The delivery queue and owner tokens that the TypeScript addon runtime calls,
 * for a probe that runs on one thread. A drained batch steps its deliveries by
 * a stride wider than the probe's mln_delivery, as a later C API may, so the
 * runtime must read each one at the reported stride.
 */
#include <stdlib.h>
#include <string.h>

#include "maplibre_native_c/binding_support.h"
#include "maplibre_native_c/delivery.h"

// A delivery as a later C API might lay it out, with a member after it.
typedef struct wide_delivery {
  mln_delivery delivery;
  uint64_t later;
} wide_delivery;

typedef struct probe_queue {
  mln_wake wake;
  wide_delivery* items;
  size_t count;
} probe_queue;

typedef struct probe_batch {
  wide_delivery* items;
  size_t count;
} probe_batch;

mln_status mln_delivery_queue_create(
  const mln_delivery_queue_options* options, mln_delivery_queue* out_queue,
  mln_diagnostic* out_diagnostic
) {
  (void)out_diagnostic;
  probe_queue* queue = calloc(1, sizeof *queue);
  if (queue == NULL) return MLN_STATUS_NATIVE_ERROR;
  queue->wake = options->wake;
  *out_queue = (mln_delivery_queue)(uintptr_t)queue;
  return MLN_STATUS_OK;
}

void mln_delivery_queue_release(mln_delivery_queue handle) {
  probe_queue* queue = (probe_queue*)(uintptr_t)handle;
  if (queue == NULL) return;
  if (queue->wake.release_user_data != NULL) {
    queue->wake.release_user_data(queue->wake.user_data);
  }
  free(queue->items);
  free(queue);
}

void mln_probe_queue_complete(
  const mln_completion* completion, const mln_completion_result* result
) {
  probe_queue* queue = (probe_queue*)(uintptr_t)completion->queue;
  wide_delivery* items =
    realloc(queue->items, (queue->count + 1) * sizeof *items);
  if (items == NULL) abort();
  queue->items = items;
  wide_delivery* item = &items[queue->count++];
  memset(item, 0, sizeof *item);
  item->delivery.type = MLN_DELIVERY_TYPE_COMPLETION;
  item->delivery.user_data = completion->user_data;
  item->delivery.payload.completion = *result;
  item->later = 0xcdcdcdcdcdcdcdcdULL;
  if (queue->count == 1) {
    queue->wake.callback(queue->wake.user_data);
  }
}

mln_status mln_delivery_queue_drain(
  mln_delivery_queue handle, mln_delivery_batch* out_batch,
  mln_delivery_batch_view* out_view, mln_diagnostic* out_diagnostic
) {
  (void)out_diagnostic;
  probe_queue* queue = (probe_queue*)(uintptr_t)handle;
  if (queue->count == 0) return MLN_STATUS_NOT_READY;
  probe_batch* batch = malloc(sizeof *batch);
  if (batch == NULL) return MLN_STATUS_NATIVE_ERROR;
  batch->items = queue->items;
  batch->count = queue->count;
  queue->items = NULL;
  queue->count = 0;
  *out_batch = (mln_delivery_batch)(uintptr_t)batch;
  out_view->delivery_size = sizeof(wide_delivery);
  out_view->deliveries = &batch->items[0].delivery;
  out_view->delivery_count = batch->count;
  return MLN_STATUS_OK;
}

mln_status mln_delivery_batch_take(
  mln_delivery_batch batch, size_t index, mln_diagnostic* out_diagnostic
) {
  (void)out_diagnostic;
  probe_batch* owned = (probe_batch*)(uintptr_t)batch;
  return index < owned->count ? MLN_STATUS_OK : MLN_STATUS_INVALID_ARGUMENT;
}

void mln_delivery_batch_release(mln_delivery_batch batch) {
  probe_batch* owned = (probe_batch*)(uintptr_t)batch;
  if (owned == NULL) return;
  free(owned->items);
  free(owned);
}

void* mln_binding_owner_token_create(uint64_t handle) {
  uint64_t* token = malloc(sizeof *token);
  if (token != NULL) *token = handle;
  return token;
}

void mln_binding_owner_token_destroy(void* token) { free(token); }

void mln_binding_owner_finalize(void* token) { free(token); }

mln_status mln_binding_render_session_abandon_at_exit(
  mln_render_session session, mln_render_abandon_result* out_result,
  mln_diagnostic* out_diagnostic
) {
  (void)session, (void)out_result, (void)out_diagnostic;
  return MLN_STATUS_OK;
}
