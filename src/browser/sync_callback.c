// The browser module's synchronous host callbacks.
//
// MapLibre asks a resource provider and a resource transform for an answer
// while it waits, on whichever thread reached the network layer -- a MapLibre
// worker, or the dispatcher's own thread. A browser host cannot answer from
// there. An Emscripten `addFunction` trampoline belongs to the agent that
// installed it, so calling one from a worker pthread reaches nothing
// (emscripten-core#21273), which is the same constraint `log_queue.c` exists
// because of.
//
// Queueing is what solves that for logging, and it cannot solve it here.
// Logging has no answer to give: the record is copied, the decision is fixed,
// and the host reads the copy whenever it likes. A provider has to decide
// PASS_THROUGH or HANDLE as it returns, and a queued provider can only answer
// HANDLE -- for every request, including the ones the host never takes. Nothing
// converts a handled request back to native pass-through, so MapLibre waits for
// a completion that never arrives and the map stops loading.
//
// So the call travels instead of the answer. `emscripten_proxy_sync` enqueues
// one task for the main runtime thread and blocks the calling thread until that
// task has run. Blocking is legal there, because the caller is a worker. On the
// main runtime thread the host's trampoline is the valid function pointer it
// always was, so the host runs, returns its decision, and the worker resumes
// with it. The page reaches its event loop even while a `maplibreScope` is
// parked on a JavaScript Promise Integration suspension, so the task runs and
// the wait ends.
//
// **A dedicated proxying queue rather than the system queue.** Emscripten's
// system queue is processed inside system functions at points its own header
// compares to a signal handler, and host code is not safe to run at an
// arbitrary system-function boundary. A queue created here is executed only
// when the main runtime thread processes this queue's own notification, which
// arrives as a mailbox message and runs as an ordinary event-loop task. The
// notification path is the same one the system queue uses, so the dedicated
// queue costs nothing extra, and it also keeps a slow host callback from
// delaying the low-level runtime work the system queue carries.
//
// **The ordering that keeps this from deadlocking.** The main runtime thread is
// the page, and the page never blocks: an owner-affine call is submitted to the
// dispatcher and parked on a promise, which returns control to the event loop.
// So every wait between these threads points one way, from a worker to the
// page, and a cycle needs an edge back. Three rules keep that edge from
// existing.
//
// - The page must not block. `pthread_join`, a futex wait, and
//   `mln_resource_request_wait_until_retired` all block, and a page that blocks
//   while a worker is inside a proxied callback stops both threads for good.
//   `mln_browser_dispatcher_stop` exists so that teardown has a non-blocking
//   path, and the same rule covers this.
// - The host callback must not wait for a worker. It runs on a stack entered
//   from native rather than through `maplibreScope`, so it may not suspend, and
//   it must not take the binding's module-wide suspension gate -- a parked
//   scope may already hold it, and that scope cannot resume while the callback
//   holds the worker. The Kotlin end enters its callback scope for exactly
//   this, which is what makes a dispatched call from inside a callback report
//   an error instead of parking.
// - The proxied function stays minimal. It calls one registered host function
//   pointer and returns.
//
// Native's own teardown ordering already fits.
// `mln_runtime_set_resource_provider` and `mln_runtime_clear_resource_provider`
// wait for in-flight provider callbacks, and a page that calls them does so
// through the dispatcher: the dispatcher's thread waits, the callback it waits
// for is answered by the page, and the page is parked on a promise rather than
// blocking.
//
// **A host installs before it registers and clears after.** The registered
// callback is the thunk below, so native holds a pointer into this module for
// the runtime's lifetime and the host's own function pointer is reached through
// the atomic here. Clearing it while a request is in flight is safe -- the
// request passes through instead -- but a host that clears before the runtime
// has released the provider will silently pass requests through, so the order
// that keeps behavior exact is: install, register, clear the registration, then
// clear the host pointer.

#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/runtime.h"

// Read on a worker for every call and written by the host, so an atomic rather
// than a plain pointer: the host installs from the page while a MapLibre worker
// may already be inside the thunk.
static _Atomic(mln_resource_provider_callback) provider_host = NULL;
static _Atomic(mln_resource_transform_callback) transform_host = NULL;

static pthread_once_t queue_once = PTHREAD_ONCE_INIT;
static em_proxying_queue* callback_queue = NULL;

static void mln_browser_sync_create_queue(void) {
  callback_queue = em_proxying_queue_create();
}

// Created once, on whichever thread first asks. `pthread_once` is also what
// publishes the pointer: a worker that reaches the queue through this function
// is ordered after the creation, so it never reads a half-built queue.
static em_proxying_queue* mln_browser_sync_queue(void) {
  pthread_once(&queue_once, mln_browser_sync_create_queue);
  return callback_queue;
}

typedef struct mln_browser_provider_call {
  mln_resource_provider_callback host;
  void* user_data;
  const mln_resource_request* request;
  mln_resource_request_handle handle;
  uint32_t decision;
} mln_browser_provider_call;

// Runs on the main runtime thread, where the host's trampoline is valid. It
// calls one function pointer and returns, because everything else this thread
// might do belongs on a task of its own.
static void mln_browser_sync_run_provider(void* argument) {
  mln_browser_provider_call* call = argument;
  call->decision = call->host(call->user_data, call->request, call->handle);
}

// The mln_resource_provider_callback a host registers. `request` and `handle`
// are borrowed for the callback's duration, and the proxied call is
// synchronous, so both are still valid while the host reads them: this frame is
// what holds them alive.
static uint32_t mln_browser_sync_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  const mln_resource_provider_callback host =
    atomic_load_explicit(&provider_host, memory_order_acquire);
  if (host == NULL) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  mln_browser_provider_call call = {
    .host = host,
    .user_data = user_data,
    .request = request,
    .handle = handle,
    .decision = MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
  };
  const pthread_t main_thread = emscripten_main_runtime_thread_id();
  if (pthread_equal(pthread_self(), main_thread)) {
    // Already where the host lives. Proxying to the current thread is not
    // merely wasteful: `emscripten_proxy_sync` asserts against waiting for work
    // it proxied to itself, and an optimized build waits forever instead.
    mln_browser_sync_run_provider(&call);
    return call.decision;
  }
  if (!emscripten_proxy_sync(
        mln_browser_sync_queue(), main_thread, mln_browser_sync_run_provider,
        &call
      )) {
    // The page is gone or the queue could not take the task, so the host never
    // saw this request. Passing it through is the one answer that leaves no
    // obligation behind: the provider took nothing, and native releases the
    // handle and loads the resource itself.
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  return call.decision;
}

typedef struct mln_browser_transform_call {
  mln_resource_transform_callback host;
  void* user_data;
  uint32_t kind;
  const char* url;
  mln_resource_transform_response* out_response;
  mln_status status;
} mln_browser_transform_call;

// Runs on the main runtime thread. The host may call
// `mln_resource_transform_response_set_url` from here: the response carries its
// storage in its own `context` field rather than in thread-local state, and
// every pthread shares this module's memory, so the copy lands in the storage
// the waiting thread owns.
static void mln_browser_sync_run_transform(void* argument) {
  mln_browser_transform_call* call = argument;
  call->status =
    call->host(call->user_data, call->kind, call->url, call->out_response);
}

// The mln_resource_transform_callback a host registers.
static mln_status mln_browser_sync_transform_callback(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  const mln_resource_transform_callback host =
    atomic_load_explicit(&transform_host, memory_order_acquire);
  if (host == NULL) {
    return MLN_STATUS_OK;
  }
  mln_browser_transform_call call = {
    .host = host,
    .user_data = user_data,
    .kind = kind,
    .url = url,
    .out_response = out_response,
    .status = MLN_STATUS_OK,
  };
  const pthread_t main_thread = emscripten_main_runtime_thread_id();
  if (pthread_equal(pthread_self(), main_thread)) {
    mln_browser_sync_run_transform(&call);
    return call.status;
  }
  if (!emscripten_proxy_sync(
        mln_browser_sync_queue(), main_thread, mln_browser_sync_run_transform,
        &call
      )) {
    // Reported rather than hidden behind an OK with no replacement URL. Either
    // way the request keeps the URL it came with, because MapLibre treats a
    // non-OK transform as no rewrite.
    return MLN_STATUS_INVALID_STATE;
  }
  return call.status;
}

/**
 * Installs the host function that answers resource provider callbacks.
 *
 * `host` is a function pointer valid on the main runtime thread, which for a
 * browser host is a trampoline it added on the page. It is called with the
 * user_data the host registered in mln_resource_provider, so one host function
 * can serve several registrations.
 *
 * Passing null clears it, after which every request passes through to native
 * loading. Returns false when the proxying queue could not be created, in which
 * case nothing is installed.
 *
 * **Call this from the main runtime thread**, before registering the provider
 * with a runtime, and clear it only after the registration has been cleared.
 */
MLN_API bool mln_browser_sync_provider_install(
  mln_resource_provider_callback host
) MLN_NOEXCEPT {
  if (host != NULL && mln_browser_sync_queue() == NULL) {
    return false;
  }
  atomic_store_explicit(&provider_host, host, memory_order_release);
  return true;
}

/**
 * Returns the callback to register in mln_resource_provider.
 *
 * It is compiled into this module, so it is callable from every MapLibre
 * thread. It forwards to whatever mln_browser_sync_provider_install() last
 * installed, on the main runtime thread.
 */
MLN_API mln_resource_provider_callback
mln_browser_sync_provider_thunk(void) MLN_NOEXCEPT {
  return mln_browser_sync_provider_callback;
}

/**
 * Installs the host function that answers resource transform callbacks.
 *
 * The rules are mln_browser_sync_provider_install()'s. Passing null clears it,
 * after which every URL is used unchanged.
 */
MLN_API bool mln_browser_sync_transform_install(
  mln_resource_transform_callback host
) MLN_NOEXCEPT {
  if (host != NULL && mln_browser_sync_queue() == NULL) {
    return false;
  }
  atomic_store_explicit(&transform_host, host, memory_order_release);
  return true;
}

/**
 * Returns the callback to register in mln_resource_transform.
 *
 * It forwards to whatever mln_browser_sync_transform_install() last installed,
 * on the main runtime thread.
 */
MLN_API mln_resource_transform_callback
mln_browser_sync_transform_thunk(void) MLN_NOEXCEPT {
  return mln_browser_sync_transform_callback;
}
