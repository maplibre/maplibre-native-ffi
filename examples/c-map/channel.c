#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "channel.h"

void command_list_deinit(command_list* list) {
  free(list->items);
  *list = (command_list){};
}

void command_queue_init(command_queue* queue) {
  *queue = (command_queue){};
  if (mtx_init(&queue->lock, mtx_plain) != thrd_success) {
    abort();
  }
}

void command_queue_deinit(command_queue* queue) {
  command_list_deinit(&queue->pending);
  mtx_destroy(&queue->lock);
}

void command_queue_push(command_queue* queue, camera_command command) {
  mtx_lock(&queue->lock);
  command_list* pending = &queue->pending;
  if (pending->len == pending->cap) {
    const size_t cap = pending->cap == 0 ? 16 : pending->cap * 2;
    camera_command* items =
      realloc(pending->items, cap * sizeof(camera_command));
    if (items == nullptr) {
      fputs("camera command queue out of memory\n", stderr);
      abort();
    }
    pending->items = items;
    pending->cap = cap;
  }
  pending->items[pending->len++] = command;
  mtx_unlock(&queue->lock);
}

void command_queue_drain_into(command_queue* queue, command_list* out) {
  out->len = 0;
  mtx_lock(&queue->lock);
  const command_list drained = queue->pending;
  queue->pending = *out;
  *out = drained;
  mtx_unlock(&queue->lock);
}

void render_request_init(render_request* request) {
  atomic_store_explicit(&request->value, true, memory_order_release);
}

void render_request_set(render_request* request) {
  atomic_store_explicit(&request->value, true, memory_order_release);
}

bool render_request_consume(render_request* request) {
  return atomic_exchange_explicit(&request->value, false, memory_order_acq_rel);
}

void map_channel_init(map_channel* channel) {
  *channel = (map_channel){
    .map = MLN_HANDLE_NULL,
    .wake = MLN_HANDLE_NULL,
  };
  if (mtx_init(&channel->lock, mtx_plain) != thrd_success) {
    abort();
  }
}

void map_channel_deinit(map_channel* channel) { mtx_destroy(&channel->lock); }

void map_channel_publish(
  map_channel* channel, mln_map map, mln_wake_source wake
) {
  mtx_lock(&channel->lock);
  channel->map = map;
  channel->wake = wake;
  atomic_store_explicit(&channel->published, true, memory_order_release);
  mtx_unlock(&channel->lock);
}

void map_channel_wake_runtime_loop(map_channel* channel) {
  if (!atomic_load_explicit(&channel->published, memory_order_acquire)) {
    return;
  }
  mtx_lock(&channel->lock);
  if (channel->wake != MLN_HANDLE_NULL) {
    mln_wake_source_signal(channel->wake);
  }
  mtx_unlock(&channel->lock);
}

bool map_channel_try_map(map_channel* channel, mln_map* out_map) {
  if (!atomic_load_explicit(&channel->published, memory_order_acquire)) {
    return false;
  }
  mtx_lock(&channel->lock);
  *out_map = channel->map;
  mtx_unlock(&channel->lock);
  return *out_map != MLN_HANDLE_NULL;
}

void map_channel_request_shutdown(map_channel* channel) {
  atomic_store_explicit(&channel->shutdown, true, memory_order_release);
  // Release the pump so shutdown is observed now rather than after the
  // parking bound expires.
  map_channel_wake_runtime_loop(channel);
}

bool map_channel_shutdown_requested(map_channel* channel) {
  return atomic_load_explicit(&channel->shutdown, memory_order_acquire);
}

void map_channel_await_shutdown(map_channel* channel) {
  while (!map_channel_shutdown_requested(channel)) {
    thrd_sleep(&(struct timespec){.tv_nsec = 1000 * 1000}, nullptr);
  }
}

void map_channel_fail(map_channel* channel, app_error error) {
  mtx_lock(&channel->lock);
  if (channel->failure == APP_OK) {
    channel->failure = error;
  }
  atomic_store_explicit(&channel->failed, true, memory_order_release);
  mtx_unlock(&channel->lock);
}

bool map_channel_failure(map_channel* channel, app_error* out_error) {
  if (!atomic_load_explicit(&channel->failed, memory_order_acquire)) {
    return false;
  }
  mtx_lock(&channel->lock);
  *out_error = channel->failure;
  mtx_unlock(&channel->lock);
  return true;
}
