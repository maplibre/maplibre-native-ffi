/*
 * The delivery queue and owner tokens that the TypeScript addon runtime calls,
 * for a probe that runs on one thread. The queue runs each queued completion
 * in dispatch, as native code does for a completion that names a queue.
 */
#include <stdlib.h>
#include <string.h>

#include "maplibre_native_c/binding_support.h"
#include "maplibre_native_c/delivery.h"

typedef struct probe_call {
  mln_completion_callback callback;
  void* user_data;
  mln_user_data_release release_user_data;
  mln_completion_result result;
} probe_call;

typedef struct probe_queue {
  mln_wake wake;
  probe_call* items;
  size_t count;
} probe_queue;

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
  probe_call* items = realloc(queue->items, (queue->count + 1) * sizeof *items);
  if (items == NULL) abort();
  queue->items = items;
  items[queue->count++] = (probe_call){
    .callback = completion->callback,
    .user_data = completion->user_data,
    .release_user_data = completion->release_user_data,
    .result = *result,
  };
  if (queue->count == 1) {
    queue->wake.callback(queue->wake.user_data);
  }
}

mln_status mln_delivery_queue_dispatch(
  mln_delivery_queue handle, mln_diagnostic* out_diagnostic
) {
  (void)out_diagnostic;
  probe_queue* queue = (probe_queue*)(uintptr_t)handle;
  if (queue->count == 0) return MLN_STATUS_NOT_READY;
  probe_call* items = queue->items;
  size_t count = queue->count;
  queue->items = NULL;
  queue->count = 0;
  for (size_t index = 0; index < count; index++) {
    items[index].callback(items[index].user_data, &items[index].result);
    if (items[index].release_user_data != NULL) {
      items[index].release_user_data(items[index].user_data);
    }
  }
  free(items);
  return MLN_STATUS_OK;
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
