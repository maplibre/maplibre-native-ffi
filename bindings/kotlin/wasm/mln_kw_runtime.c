#include <emscripten/emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "mln_kw_runtime.h"

struct mln_kw_thread_start {
  uint32_t token;
  int32_t status;
  int32_t (*run)(const void* arguments);
  _Alignas(8) unsigned char arguments[MLN_KW_THREAD_START_ARGUMENTS_SIZE];
};

// Defined in mln_kw_library.js. Each hands one page task to the function that
// Maplibre.loadModule installs, and runs on the page only.
extern void mln_kw_js_dispatch(void);
extern void mln_kw_js_thread_end(uint32_t token, int32_t status);

// The page runs every task of the system proxying queue inside each futex wait
// of a C call that contends a lock, which would run Kotlin inside that call.
// The page runs this queue only from its event loop.
static em_proxying_queue* page_queue;

// A page task carries null to dispatch the delivery queue, or the thread start
// whose call returned.
static void deliver(void* argument) {
  mln_kw_thread_start* start = argument;
  if (start == NULL) {
    mln_kw_js_dispatch();
    return;
  }
  mln_kw_js_thread_end(start->token, start->status);
  free(start);
}

/**
 * Posts one task to the page. The page's task queue grows only once its
 * earlier tasks have not run, so a post rarely allocates. A post fails only
 * without memory or once the page has exited, and the failure is logged,
 * because a caller that waits for the task waits until a later one.
 */
static void post(mln_kw_thread_start* start) {
  if (!emscripten_proxy_async(
        page_queue, emscripten_main_runtime_thread_id(), deliver, start
      )) {
    emscripten_log(EM_LOG_ERROR, "maplibre: a wake could not reach the page");
  }
}

/** The delivery queue's wake, which asks the page to dispatch it. */
static void wake(void* unused) {
  (void)unused;
  post(NULL);
}

static void nothing(void* unused) { (void)unused; }

/**
 * Creates the page's task queue, with a first task that does nothing so that
 * the page's queue exists before a native thread posts, and the delivery queue
 * whose wake asks the page to dispatch it. Maplibre.loadModule calls it before
 * any other call and keeps the queue for the module's life.
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
    .wake = {.callback = wake},
  };
  return mln_delivery_queue_create(&options, out_queue, out_diagnostic);
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
  post(start);
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
