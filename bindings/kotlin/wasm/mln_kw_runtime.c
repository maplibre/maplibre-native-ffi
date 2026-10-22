#include <emscripten/emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "mln_kw_runtime.h"

// What a page task carries in its one pointer-sized argument: a token shifted
// left by two with the kind in the low bits, or an mln_kw_thread_start, which
// malloc aligns to at least 8, with its kind in the low bits. Tokens stay below
// 2^30, so a shifted token fits. Token 0 is the delivery queue's wake.
// WasmDelivery.kt receives the same kinds.
enum {
  MLN_KW_WAKE = 1,
  MLN_KW_RELEASE = 2,
  MLN_KW_THREAD_START = 3,
  MLN_KW_KIND_MASK = 3,
};

struct mln_kw_thread_start {
  uint32_t token;
  int32_t status;
  int32_t (*run)(const void* arguments);
  _Alignas(8) unsigned char arguments[MLN_KW_THREAD_START_ARGUMENTS_SIZE];
};

// Defined in mln_kw_library.js: hands one wake to the function that
// Maplibre.loadModule installs. Runs on the page only.
extern void mln_kw_js_deliver(uint32_t kind, uint32_t token, int32_t status);

// The page runs every task of the system proxying queue inside each futex wait
// of a C call that contends a lock, which would run Kotlin inside that call.
// The page runs this queue only from its event loop.
static em_proxying_queue* page_queue;

static void deliver(void* argument) {
  uintptr_t message = (uintptr_t)argument;
  uint32_t kind = (uint32_t)(message & MLN_KW_KIND_MASK);
  if (kind == MLN_KW_THREAD_START) {
    mln_kw_thread_start* start =
      (mln_kw_thread_start*)(message & ~(uintptr_t)MLN_KW_KIND_MASK);
    mln_kw_js_deliver(kind, start->token, start->status);
    free(start);
    return;
  }
  mln_kw_js_deliver(kind, (uint32_t)(message >> 2), 0);
}

/**
 * Posts one task to the page. The page's task queue grows only once its
 * earlier tasks have not run, so a post rarely allocates. A post fails only
 * without memory or once the page has exited, and the failure is logged,
 * because a caller that waits for the wake waits until a later one.
 */
static void post(uintptr_t message) {
  if (!emscripten_proxy_async(
        page_queue, emscripten_main_runtime_thread_id(), deliver, (void*)message
      )) {
    emscripten_log(EM_LOG_ERROR, "maplibre: a wake could not reach the page");
  }
}

/** The callback of every mln_wake that Kotlin registers, and of the queue's. */
static void wake(void* user_data) {
  post(((uintptr_t)user_data << 2) | MLN_KW_WAKE);
}

/** The release_user_data of every mln_wake that Kotlin registers. */
static void release(void* user_data) {
  post(((uintptr_t)user_data << 2) | MLN_KW_RELEASE);
}

static void nothing(void* unused) { (void)unused; }

/**
 * Creates the page's task queue, with a first task that does nothing so that
 * the page's queue exists before a native thread posts, and the delivery queue
 * whose wake asks the page to drain it. Maplibre.loadModule calls it before any
 * other call and keeps the queue for the module's life.
 */
EMSCRIPTEN_KEEPALIVE int32_t
mln_kw_install(mln_delivery_queue* out_queue, mln_diagnostic* out_diagnostic) {
  if (page_queue == NULL) {
    page_queue = em_proxying_queue_create();
    if (page_queue == NULL) {
      return MLN_STATUS_NATIVE_ERROR;
    }
    if (!emscripten_proxy_async(
          page_queue, emscripten_main_runtime_thread_id(), nothing, NULL
        )) {
      em_proxying_queue_destroy(page_queue);
      page_queue = NULL;
      return MLN_STATUS_NATIVE_ERROR;
    }
  }
  mln_delivery_queue_options options = {
    .size = sizeof options,
    .wake = {.callback = wake, .user_data = NULL},
  };
  return mln_delivery_queue_create(&options, out_queue, out_diagnostic);
}

/** The function-table index of the wake callback, for UpcallStubs. */
EMSCRIPTEN_KEEPALIVE uintptr_t mln_kw_wake_function(void) {
  return (uintptr_t)wake;
}

/** The function-table index of the wake release, for UpcallStubs. */
EMSCRIPTEN_KEEPALIVE uintptr_t mln_kw_wake_release_function(void) {
  return (uintptr_t)release;
}

/**
 * [size] bytes, at least one, aligned for any C type, or null when the heap is
 * full. NativeMemory clears what it must.
 */
EMSCRIPTEN_KEEPALIVE void* mln_kw_allocate(size_t size) {
  return malloc(size > 0 ? size : 1);
}

EMSCRIPTEN_KEEPALIVE void mln_kw_free(void* address) { free(address); }

// ---------------------------------------------------------------------------
// Thread starts

/** A thread start for the Kotlin waiter [token], or null without memory. */
EMSCRIPTEN_KEEPALIVE mln_kw_thread_start* mln_kw_thread_start_create(
  uint32_t token
) {
  mln_kw_thread_start* start = calloc(1, sizeof *start);
  if (start != NULL) {
    start->token = token;
  }
  return start;
}

/** Frees a thread start whose thread never started. */
EMSCRIPTEN_KEEPALIVE void mln_kw_thread_start_destroy(
  mln_kw_thread_start* start
) {
  free(start);
}

static void* thread_start_main(void* argument) {
  mln_kw_thread_start* start = argument;
  start->status = start->run(start->arguments);
  post((uintptr_t)start | MLN_KW_THREAD_START);
  return NULL;
}

int32_t mln_kw_start_thread(
  mln_kw_thread_start* start, int32_t (*run)(const void* arguments),
  const void* arguments, size_t size
) {
  memcpy(start->arguments, arguments, size);
  start->run = run;
  // Nothing joins the thread, so a page that never blocks can start it. It
  // runs once a pool worker takes it.
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
  pthread_t thread;
  int result = pthread_create(&thread, &attributes, thread_start_main, start);
  pthread_attr_destroy(&attributes);
  return result == 0 ? MLN_STATUS_OK : MLN_STATUS_NATIVE_ERROR;
}
