// The one path from a MapLibre thread into Kotlin.
//
// A Kotlin/Wasm module lives in one JavaScript realm, and a function installed
// with Emscripten's addFunction belongs to the agent that installed it
// (emscripten-core#21273), so MapLibre's worker, network, and logging threads
// reach nothing when they call one. Every native callback Kotlin wants
// therefore lands in this file, on whichever thread produced it, and is queued
// for the thread Kotlin runs on. Kotlin drains the ring inside its pump loop.
//
// callback_adapter.h does the hard half: it copies each borrowed payload into a
// native-owned record and answers MapLibre on the host's behalf. This file adds
// the queue and the wake.

#include <emscripten.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c/callback_adapter.h"
#include "mln_kotlin.h"

// Bounded, because an unbounded queue turns a logging burst into the page's
// memory ceiling. A full ring drops its oldest record and counts the drop, so
// Kotlin reports lost records rather than believing it saw them all.
#define MLN_KOTLIN_RING_CAPACITY 1024


// Kotlin reads these offsets from a hand-written layout.
_Static_assert(sizeof(mln_kotlin_record) == 20, "record layout changed");
_Static_assert(offsetof(mln_kotlin_record, payload) == 16, "payload moved");

static pthread_mutex_t ring_mutex = PTHREAD_MUTEX_INITIALIZER;
static mln_kotlin_record ring[MLN_KOTLIN_RING_CAPACITY];
static uint32_t ring_head;
static uint32_t ring_size;
static uint64_t ring_dropped;
static mln_wake_source ring_wake = MLN_HANDLE_NULL;

// Releases a record the ring evicted. A queued request that is merely destroyed
// leaves the native loader waiting for a completion that never arrives, so a
// dropped one is failed the way the adapter fails a request it cannot copy.
static void mln_kotlin_discard(const mln_kotlin_record* record) {
  if (record->kind == MLN_KOTLIN_RECORD_LOG) {
    mln_adapter_log_record_destroy(record->payload);
    return;
  }
  if (record->kind != MLN_KOTLIN_RECORD_RESOURCE_REQUEST) {
    return;
  }
  mln_adapter_queued_resource_request* request = record->payload;
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_OTHER,
    .error_message = "maplibre kotlin callback queue overflowed",
  };
  (void)mln_resource_request_complete(request->handle, &response);
  mln_resource_request_release(request->handle);
  mln_adapter_resource_provider_request_destroy(request);
}

// Runs on whichever MapLibre thread produced the callback, so it queues and
// returns. The wake source is read and signalled under the ring lock, which is
// the lock mln_kotlin_set_wake() takes, so no thread signals a source Kotlin
// has already cleared.
static void mln_kotlin_push(mln_kotlin_record record) {
  mln_kotlin_record evicted = {0};
  bool dropped = false;
  pthread_mutex_lock(&ring_mutex);
  if (ring_size == MLN_KOTLIN_RING_CAPACITY) {
    evicted = ring[ring_head];
    ring_head = (ring_head + 1) % MLN_KOTLIN_RING_CAPACITY;
    ring_size -= 1;
    ring_dropped += 1;
    dropped = true;
  }
  ring[(ring_head + ring_size) % MLN_KOTLIN_RING_CAPACITY] = record;
  ring_size += 1;
  if (ring_wake != MLN_HANDLE_NULL) {
    (void)mln_wake_source_signal(ring_wake);
  }
  pthread_mutex_unlock(&ring_mutex);
  // Outside the lock, because completing a request is not work that a thread
  // trying to log should wait behind.
  if (dropped) {
    mln_kotlin_discard(&evicted);
  }
}

static void mln_kotlin_on_log_record(void* record) {
  mln_kotlin_push((mln_kotlin_record){
    .kind =
      record == NULL ? MLN_KOTLIN_RECORD_LOG_RETIRED : MLN_KOTLIN_RECORD_LOG,
    .payload = record,
  });
}

static void mln_kotlin_on_resource_request(void* request) {
  mln_kotlin_push((mln_kotlin_record){
    .kind = request == NULL ? MLN_KOTLIN_RECORD_RESOURCE_PROVIDER_RETIRED
                            : MLN_KOTLIN_RECORD_RESOURCE_REQUEST,
    .payload = request,
  });
}

static void mln_kotlin_on_tile_fetch(
  void* user_data, mln_canonical_tile_id tile_id
) {
  mln_kotlin_push((mln_kotlin_record){
    .kind = MLN_KOTLIN_RECORD_TILE_FETCH,
    .tile_z = tile_id.z,
    .tile_x = tile_id.x,
    .tile_y = tile_id.y,
    .payload = user_data,
  });
}

static void mln_kotlin_on_tile_cancel(
  void* user_data, mln_canonical_tile_id tile_id
) {
  mln_kotlin_push((mln_kotlin_record){
    .kind = MLN_KOTLIN_RECORD_TILE_CANCEL,
    .tile_z = tile_id.z,
    .tile_x = tile_id.x,
    .tile_y = tile_id.y,
    .payload = user_data,
  });
}

/**
 * Takes the oldest queued record, or reports that the ring is empty.
 *
 * Call this from the thread Kotlin runs on, and release each record's payload
 * with the adapter function that its kind names.
 */
EMSCRIPTEN_KEEPALIVE bool mln_kotlin_take_record(mln_kotlin_record* out) {
  if (out == NULL) {
    return false;
  }
  bool taken = false;
  pthread_mutex_lock(&ring_mutex);
  if (ring_size > 0) {
    *out = ring[ring_head];
    ring_head = (ring_head + 1) % MLN_KOTLIN_RING_CAPACITY;
    ring_size -= 1;
    taken = true;
  }
  pthread_mutex_unlock(&ring_mutex);
  return taken;
}

/** Counts every record the ring has dropped since the module started. */
EMSCRIPTEN_KEEPALIVE uint64_t mln_kotlin_dropped_records(void) {
  pthread_mutex_lock(&ring_mutex);
  const uint64_t dropped = ring_dropped;
  pthread_mutex_unlock(&ring_mutex);
  return dropped;
}

/**
 * Sets the wake source that a queued record signals, or clears it with
 * MLN_HANDLE_NULL.
 *
 * A signal releases a thread parked in mln_runtime_pump(), so a host that pumps
 * with an infinite timeout returns as soon as a record lands. Clear the source
 * before destroying it: a MapLibre thread signals under the lock this call
 * takes, so a cleared source is one no such thread still holds.
 */
EMSCRIPTEN_KEEPALIVE void mln_kotlin_set_wake(mln_wake_source source) {
  pthread_mutex_lock(&ring_mutex);
  ring_wake = source;
  pthread_mutex_unlock(&ring_mutex);
}

// One state for the module's lifetime. mln_adapter_log_record_listener takes no
// user data and the adapter treats this address as the registration's identity,
// so a second state would leave the compiled-in listener above unable to say
// which registration produced a record it was handed.
static mln_adapter_log_callback_state log_state = {
  .listener = mln_kotlin_on_log_record,
  .consume = 0,
};

/**
 * Installs the queueing log callback, reporting consume for every record.
 *
 * Reinstalling keeps the same registration identity, so a record queued across
 * the call still belongs to this registration and nothing is retired.
 *
 * Returns the status of mln_adapter_log_set_callback().
 */
EMSCRIPTEN_KEEPALIVE mln_status mln_kotlin_log_install(uint32_t consume) {
  log_state.consume = consume;
  return mln_adapter_log_set_callback(&log_state);
}

/**
 * Clears the log callback, after which one retirement record is queued.
 *
 * Returns the status of mln_adapter_log_set_callback().
 */
EMSCRIPTEN_KEEPALIVE mln_status mln_kotlin_log_clear(void) {
  return mln_adapter_log_set_callback(NULL);
}

// A WebAssembly function has a table index only once something takes its
// address, so the getters below are how Kotlin obtains one for a callback it
// stores in a native struct.

/** The mln_resource_transform.callback that applies a rewrite rule table. */
EMSCRIPTEN_KEEPALIVE mln_resource_transform_callback
mln_kotlin_rewrite_transform_callback(void) {
  return mln_adapter_resource_transform_rewrite_callback;
}

/** The mln_resource_provider.callback a queued provider registers. */
EMSCRIPTEN_KEEPALIVE mln_resource_provider_callback
mln_kotlin_queued_provider_callback(void) {
  return mln_adapter_queued_resource_provider_callback;
}

/** The mln_adapter_queued_resource_provider.listener that feeds the ring. */
EMSCRIPTEN_KEEPALIVE mln_adapter_queued_resource_request_listener
mln_kotlin_resource_request_listener(void) {
  return mln_kotlin_on_resource_request;
}

/** The mln_custom_geometry_source_options.fetch_tile that feeds the ring. */
EMSCRIPTEN_KEEPALIVE mln_custom_geometry_source_tile_callback
mln_kotlin_tile_fetch_callback(void) {
  return mln_kotlin_on_tile_fetch;
}

/** The mln_custom_geometry_source_options.cancel_tile that feeds the ring. */
EMSCRIPTEN_KEEPALIVE mln_custom_geometry_source_tile_callback
mln_kotlin_tile_cancel_callback(void) {
  return mln_kotlin_on_tile_cancel;
}
