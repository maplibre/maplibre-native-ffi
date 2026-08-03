#include <stdio.h>
#include <stdlib.h>

#include "channel.h"

#include "util.h"

void command_list_deinit(command_list* list) {
  free(list->items);
  *list = (command_list){};
}

void command_queue_init(command_queue* queue) {
  *queue = (command_queue){};
  queue->lock = SDL_CreateMutex();
  if (queue->lock == nullptr) {
    abort();
  }
}

void command_queue_deinit(command_queue* queue) {
  command_list_deinit(&queue->pending);
  SDL_DestroyMutex(queue->lock);
}

void command_queue_push(command_queue* queue, camera_command command) {
  SDL_LockMutex(queue->lock);
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
  SDL_UnlockMutex(queue->lock);
}

void command_queue_drain_into(command_queue* queue, command_list* out) {
  out->len = 0;
  SDL_LockMutex(queue->lock);
  const command_list drained = queue->pending;
  queue->pending = *out;
  *out = drained;
  SDL_UnlockMutex(queue->lock);
}

void render_request_init(render_request* request) {
  // Starts set, so the render loop draws a first frame without waiting for
  // the runtime loop to request one.
  render_request_set(request);
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
  channel->lock = SDL_CreateMutex();
  if (channel->lock == nullptr) {
    abort();
  }
}

void map_channel_deinit(map_channel* channel) {
  SDL_DestroyMutex(channel->lock);
}

void map_channel_publish(
  map_channel* channel, mln_map map, mln_wake_source wake
) {
  SDL_LockMutex(channel->lock);
  channel->map = map;
  channel->wake = wake;
  atomic_store_explicit(&channel->published, true, memory_order_release);
  SDL_UnlockMutex(channel->lock);
}

void map_channel_wake_runtime_loop(map_channel* channel) {
  if (!atomic_load_explicit(&channel->published, memory_order_acquire)) {
    return;
  }
  SDL_LockMutex(channel->lock);
  if (channel->wake != MLN_HANDLE_NULL) {
    mln_wake_source_signal(channel->wake);
  }
  SDL_UnlockMutex(channel->lock);
}

bool map_channel_try_map(map_channel* channel, mln_map* out_map) {
  if (!atomic_load_explicit(&channel->published, memory_order_acquire)) {
    return false;
  }
  SDL_LockMutex(channel->lock);
  *out_map = channel->map;
  SDL_UnlockMutex(channel->lock);
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
    sleep_milliseconds(1);
  }
}

void map_channel_fail(map_channel* channel, app_error error) {
  SDL_LockMutex(channel->lock);
  if (channel->failure == APP_OK) {
    channel->failure = error;
  }
  atomic_store_explicit(&channel->failed, true, memory_order_release);
  SDL_UnlockMutex(channel->lock);
}

bool map_channel_failure(map_channel* channel, app_error* out_error) {
  if (!atomic_load_explicit(&channel->failed, memory_order_acquire)) {
    return false;
  }
  SDL_LockMutex(channel->lock);
  *out_error = channel->failure;
  SDL_UnlockMutex(channel->lock);
  return true;
}
