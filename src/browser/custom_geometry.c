// The browser module's asynchronous custom geometry source callbacks.
//
// A custom geometry source asks its host for a tile and is told the answer
// later, through `mln_map_set_custom_geometry_source_tile_data`. MapLibre makes
// that request from the source's tile loader, which is an actor on a background
// scheduler, so `fetch_tile` and `cancel_tile` arrive on a MapLibre worker --
// and a browser host's callback body is on the page, inside a WebAssembly
// instance no worker can enter. An Emscripten `addFunction` trampoline belongs
// to the agent that installed it, so calling one from a worker pthread reaches
// nothing (emscripten-core#21273), which is the constraint `log_queue.c` and
// `sync_callback.c` both exist because of.
//
// So the call travels to the page, exactly as the resource provider's does. The
// difference is that this one does not wait, and that difference is the whole
// design.
//
// **Asynchronous, because nothing is waiting.**
// `mln_custom_geometry_source_tile_callback` returns void. The worker has
// nothing to receive, so the task is posted with `emscripten_proxy_async` and
// the worker returns immediately. `emscripten_proxy_sync` would be wrong twice
// over. These callbacks fire per tile, on arbitrary MapLibre workers, so
// blocking one for the length of a host callback is a cost the provider's
// once-per-request decision does not have; and `CustomTileLoader::fetchTile`
// holds the loader's data mutex across the call, so a blocked worker would hold
// it too, and every other operation on that source -- setting a tile's data,
// invalidating one, dropping one -- would queue behind the page until the host
// returned.
//
// **The tile id is copied.** An asynchronous task outlives the frame that
// posted it: the worker returns as soon as the task is enqueued, and its stack
// is gone long before the page runs it. The notification below is therefore
// heap-allocated and holds the tile id by value, copied out of the by-value
// parameter while the calling frame still owns it. Nothing here points into a
// worker's stack. The task frees its own notification on the page, and a post
// that fails frees it on the worker, so each one is released exactly once by
// whichever side ended up owning it.
//
// **A dedicated proxying queue, and not the one `sync_callback.c` uses.** The
// system queue is processed inside system functions at points its own header
// compares to a signal handler, and host code is not safe to run at an
// arbitrary system-function boundary. A queue created here is executed only
// when the main runtime thread processes this queue's own notification, which
// arrives as a mailbox message and runs as an ordinary event-loop task. It is
// separate from the synchronous callbacks' queue so that a slow tile callback
// delays only other tile callbacks: a provider's answer is being waited on by a
// blocked worker, and this one is not.
//
// **This adds no edge to the wait graph, and that is what a host may spend.**
// `sync_callback.c` describes a graph whose every wait points from a worker to
// the page, which is safe only while nothing points back. Nothing here waits at
// all -- not the worker, which returns as soon as the task is enqueued, and not
// the page, which runs the task from its event loop. So a host is free to wait
// inside one of these callbacks, including on the thread that owns its runtime,
// which a host inside one of `sync_callback.c`'s is not.
//
// The one rule that still applies is where the delivery below lands: it is an
// ordinary proxied task, so the host function is entered on a stack that a
// browser will not let suspend. A host that wants to suspend therefore carries
// the notification onto a stack of its own before running its body -- the
// Kotlin binding queues it and drains the queue under `WebAssembly.promising`
// -- which costs an event-loop turn and is invisible here, because this side
// was told nothing about when the call would happen and receives no answer from
// it either way. A host that would rather answer later calls
// `mln_map_set_custom_geometry_source_tile_data` from wherever it likes
// afterwards, which is what the C API's asynchronous shape already allows on
// every platform.
//
// **What this guarantees, and what it does not.** A notification native emitted
// is either delivered to the installed host or dropped; it is never delivered
// to a registration that has retired, because the host pointer is read on the
// page at delivery rather than captured on the worker at posting, and the page
// is one agent, so clearing cannot interleave with delivering. Notifications
// posted by one MapLibre thread are delivered in the order that thread posted
// them. Nothing orders notifications posted by different threads, and nothing
// pairs a cancel with the fetch it cancels: `cancel_tile` is best-effort in
// MapLibre itself -- it can arrive before the fetch's notification has been
// delivered, can repeat for one tile, and may never arrive at all -- so a host
// treats it as a hint to stop work rather than as the end of a request.
//
// **A host installs before it registers and clears after.** The callbacks
// registered in the descriptor are the thunks below, so native holds pointers
// into this module for as long as the source exists, and the host's own
// function pointer is reached through the atomic here. The order that keeps
// behavior exact is: install, add the source, remove the source, then clear the
// host pointer. Clearing early is safe -- notifications are dropped -- but the
// host stops hearing about tiles a source it still owns is asking for.

#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/style.h"

/**
 * Which of the two callbacks a notification carries.
 *
 * The host is one function rather than two, so that one table entry serves both
 * callbacks and a registration is reached the same way whichever arrives. These
 * values are part of the contract with the host and are mirrored by the
 * binding's `CustomGeometryBridge`.
 */
enum mln_browser_custom_geometry_kind {
  MLN_BROWSER_CUSTOM_GEOMETRY_FETCH = 0,
  MLN_BROWSER_CUSTOM_GEOMETRY_CANCEL = 1,
};

/**
 * The host function a notification is delivered to, on the main runtime thread.
 *
 * The tile id arrives as its three components rather than as a pointer, because
 * the notification that carried it is freed as the call returns and a host that
 * read it afterwards would read released memory. `kind` is one of
 * mln_browser_custom_geometry_kind.
 */
typedef void (*mln_browser_custom_geometry_host)(
  void* user_data, uint32_t kind, uint32_t z, uint32_t x, uint32_t y
);

// Written by the host from the page and read on the page in every delivery, and
// read once on a worker to skip the allocation when nobody is listening. An
// atomic because those are different threads, even though the decision that
// matters is the one made on the page.
static _Atomic(mln_browser_custom_geometry_host) tile_host = NULL;

static pthread_once_t queue_once = PTHREAD_ONCE_INIT;
static em_proxying_queue* tile_queue = NULL;

static void mln_browser_custom_geometry_create_queue(void) {
  tile_queue = em_proxying_queue_create();
}

// Created once, on whichever thread first asks. `pthread_once` is also what
// publishes the pointer: a worker that reaches the queue through this function
// is ordered after the creation, so it never reads a half-built queue.
static em_proxying_queue* mln_browser_custom_geometry_queue(void) {
  pthread_once(&queue_once, mln_browser_custom_geometry_create_queue);
  return tile_queue;
}

typedef struct mln_browser_tile_notification {
  void* user_data;
  uint32_t kind;
  mln_canonical_tile_id tile_id;
} mln_browser_tile_notification;

// Runs on the main runtime thread, where the host's trampoline is valid, and
// owns the notification it is handed.
//
// The host pointer is loaded here rather than on the posting thread, which is
// what makes a late notification safe: a host that cleared its registration
// cleared this pointer from the page, and the page cannot be clearing it while
// this is running.
static void mln_browser_custom_geometry_deliver(void* argument) {
  mln_browser_tile_notification* notification = argument;
  const mln_browser_custom_geometry_host host =
    atomic_load_explicit(&tile_host, memory_order_acquire);
  if (host != NULL) {
    host(
      notification->user_data, notification->kind, notification->tile_id.z,
      notification->tile_id.x, notification->tile_id.y
    );
  }
  free(notification);
}

// Copies one callback into a task for the page, and returns without waiting.
//
// Every failure path here drops the notification rather than reporting it,
// because there is nobody to report it to: the caller is MapLibre's tile
// loader, which has no channel for a delivery failure and treats an unanswered
// fetch as a tile that never arrives. A dropped fetch therefore leaves that
// tile empty until the source is invalidated or the tile is requested again.
static void mln_browser_custom_geometry_post(
  uint32_t kind, void* user_data, mln_canonical_tile_id tile_id
) {
  if (atomic_load_explicit(&tile_host, memory_order_acquire) == NULL) {
    return;
  }
  em_proxying_queue* queue = mln_browser_custom_geometry_queue();
  if (queue == NULL) {
    return;
  }
  mln_browser_tile_notification* notification =
    malloc(sizeof(mln_browser_tile_notification));
  if (notification == NULL) {
    return;
  }
  notification->user_data = user_data;
  notification->kind = kind;
  notification->tile_id = tile_id;
  const pthread_t main_thread = emscripten_main_runtime_thread_id();
  if (pthread_equal(pthread_self(), main_thread)) {
    // Already where the host lives. MapLibre reaches these callbacks from its
    // own threads rather than from the page, so this is not the path a tile
    // takes; it is here because the C API places no thread rule on them, and a
    // direct call is what every other platform's binding does anyway.
    mln_browser_custom_geometry_deliver(notification);
    return;
  }
  if (!emscripten_proxy_async(
        queue, main_thread, mln_browser_custom_geometry_deliver, notification
      )) {
    free(notification);
  }
}

static void mln_browser_custom_geometry_fetch_callback(
  void* user_data, mln_canonical_tile_id tile_id
) {
  mln_browser_custom_geometry_post(
    MLN_BROWSER_CUSTOM_GEOMETRY_FETCH, user_data, tile_id
  );
}

static void mln_browser_custom_geometry_cancel_callback(
  void* user_data, mln_canonical_tile_id tile_id
) {
  mln_browser_custom_geometry_post(
    MLN_BROWSER_CUSTOM_GEOMETRY_CANCEL, user_data, tile_id
  );
}

/**
 * Installs the host function that receives custom geometry tile notifications.
 *
 * `host` is a function pointer valid on the main runtime thread, which for a
 * browser host is a trampoline it added on the page. It is called with the
 * user_data the host registered in mln_custom_geometry_source_options, so one
 * host function can serve every source a page has added.
 *
 * Passing null clears it, after which every notification is dropped. Returns
 * false when the proxying queue could not be created, in which case nothing is
 * installed.
 *
 * **Call this from the main runtime thread**, before adding the first source,
 * and clear it only after the last source has been removed.
 */
MLN_API bool mln_browser_custom_geometry_install(
  mln_browser_custom_geometry_host host
) MLN_NOEXCEPT {
  if (host != NULL && mln_browser_custom_geometry_queue() == NULL) {
    return false;
  }
  atomic_store_explicit(&tile_host, host, memory_order_release);
  return true;
}

/**
 * Returns the callback to register as `fetch_tile`.
 *
 * It is compiled into this module, so it is callable from every MapLibre
 * thread. It posts to whatever mln_browser_custom_geometry_install() last
 * installed, on the main runtime thread, and returns without waiting.
 */
MLN_API mln_custom_geometry_source_tile_callback
mln_browser_custom_geometry_fetch_thunk(void) MLN_NOEXCEPT {
  return mln_browser_custom_geometry_fetch_callback;
}

/** Returns the callback to register as `cancel_tile`, on the same terms. */
MLN_API mln_custom_geometry_source_tile_callback
mln_browser_custom_geometry_cancel_thunk(void) MLN_NOEXCEPT {
  return mln_browser_custom_geometry_cancel_callback;
}
