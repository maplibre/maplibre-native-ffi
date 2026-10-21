// Fixtures of the Kotlin/Wasm suite, which only the test module links.
//
// Each fixture calls the C functions behind a wake registration that Kotlin
// wrote, as native would, from the page or from a thread of its own, so a test
// can watch what the binding's wake shim delivers.
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <maplibre_native_c.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int start_detached(void* (*run)(void*), void* argument) {
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
  pthread_t thread;
  int result = pthread_create(&thread, &attributes, run, argument);
  pthread_attr_destroy(&attributes);
  return result;
}

/** Calls [wake]'s callback on the calling thread. */
EMSCRIPTEN_KEEPALIVE void mln_kw_test_wake(const mln_wake* wake) {
  wake->callback(wake->user_data);
}

/** Releases [wake]'s user_data on the calling thread, as native's final call.
 */
EMSCRIPTEN_KEEPALIVE void mln_kw_test_release(const mln_wake* wake) {
  wake->release_user_data(wake->user_data);
}

// ---------------------------------------------------------------------------
// Contention probe
//
// A holder thread takes a mutex, and the page then calls an export that
// contends it. The page waits for the mutex in a futex spin, which runs the
// system proxying queue, so a task that the holder posts to that queue runs
// inside the export and proves that the export spun. The holder wakes a
// registration during the spin, and releases both registrations and then the
// mutex once the probe task ran. The holder gives up each wait after its
// deadline and still releases everything, so a page that never spins makes the
// export return 0 rather than block the page forever.

static pthread_mutex_t contended = PTHREAD_MUTEX_INITIALIZER;
// Set by the page just before it contends the mutex, and cleared after.
static volatile uint32_t contending;
// Set by the system-queue task that runs while the page contends.
static volatile uint32_t spun;

typedef struct holder {
  mln_wake held;
  mln_wake during;
} holder;

static void note_spin(void* unused) {
  (void)unused;
  __atomic_store_n(
    &spun, __atomic_load_n(&contending, __ATOMIC_SEQ_CST), __ATOMIC_SEQ_CST
  );
  emscripten_futex_wake(&spun, 1);
}

static void wait_until_set(volatile uint32_t* flag) {
  while (__atomic_load_n(flag, __ATOMIC_SEQ_CST) == 0) {
    emscripten_futex_wait(flag, 0, INFINITY);
  }
}

// How long the holder waits for the page to contend, which is longer than the
// test's wait for the held wake, and then for the probe task.
#define CONTEND_WAIT_MS 30000.0
#define SPIN_WAIT_MS 2000.0

/** Waits until [flag] is set, for at most [ms]. Returns whether it was set. */
static int wait_until_set_within(volatile uint32_t* flag, double ms) {
  double deadline = emscripten_get_now() + ms;
  while (__atomic_load_n(flag, __ATOMIC_SEQ_CST) == 0) {
    double remaining = deadline - emscripten_get_now();
    if (remaining <= 0) {
      return 0;
    }
    emscripten_futex_wait(flag, 0, remaining);
  }
  return 1;
}

static void* hold(void* argument) {
  holder* wakes = argument;
  pthread_mutex_lock(&contended);
  wakes->held.callback(wakes->held.user_data);
  if (wait_until_set_within(&contending, CONTEND_WAIT_MS)) {
    wakes->during.callback(wakes->during.user_data);
    emscripten_proxy_async(
      emscripten_proxy_get_system_queue(), emscripten_main_runtime_thread_id(),
      note_spin, NULL
    );
    // A probe task that runs after this gives up finds the page no longer
    // contending, so it leaves spun at 0.
    wait_until_set_within(&spun, SPIN_WAIT_MS);
  }
  // Release both wakes before the mutex, so their releases are posted before
  // the page's contended call returns and before any wake the page posts next.
  wakes->held.release_user_data(wakes->held.user_data);
  wakes->during.release_user_data(wakes->during.user_data);
  free(wakes);
  pthread_mutex_unlock(&contended);
  return NULL;
}

/**
 * Starts the holder thread, which takes the mutex and then wakes [held].
 * Once the page contends the mutex, the holder wakes [during], and it
 * releases both wakes before it releases the mutex. Returns 0 when the thread
 * started.
 */
EMSCRIPTEN_KEEPALIVE int32_t
mln_kw_test_contention_hold(const mln_wake* held, const mln_wake* during) {
  holder* wakes = malloc(sizeof *wakes);
  if (wakes == NULL) {
    return -1;
  }
  *wakes = (holder){.held = *held, .during = *during};
  __atomic_store_n(&contending, 0, __ATOMIC_SEQ_CST);
  __atomic_store_n(&spun, 0, __ATOMIC_SEQ_CST);
  int result = start_detached(hold, wakes);
  if (result != 0) {
    free(wakes);
  }
  return result;
}

/**
 * Contends the holder's mutex from the page, and returns whether the probe
 * task ran inside this call, so the call spun. Call it after the holder has
 * woken its held registration. Returns 0 when the holder gave up waiting for
 * the probe task.
 */
EMSCRIPTEN_KEEPALIVE int32_t mln_kw_test_contention_contend(void) {
  __atomic_store_n(&contending, 1, __ATOMIC_SEQ_CST);
  emscripten_futex_wake(&contending, 1);
  pthread_mutex_lock(&contended);
  pthread_mutex_unlock(&contended);
  __atomic_store_n(&contending, 0, __ATOMIC_SEQ_CST);
  return (int32_t)__atomic_load_n(&spun, __ATOMIC_SEQ_CST);
}

// ---------------------------------------------------------------------------
// Pool hold
//
// Emscripten pre-spawns a pool of Workers, and pthread_create takes one at
// once. Once the pool is empty, pthread_create asks the page for a new Worker,
// which starts only after the page yields. The hold takes every Worker that
// the pool has, with threads that wait until the page releases them.

int32_t mln_kw_test_js_pool_workers(int32_t which);

// Set by the page to release the held threads.
static volatile uint32_t pool_released;

static void* hold_pool_worker(void* unused) {
  (void)unused;
  wait_until_set(&pool_released);
  return NULL;
}

/** The Workers that wait in the pool, or every Worker when [all] is set. */
EMSCRIPTEN_KEEPALIVE int32_t mln_kw_test_pool_workers(int32_t all) {
  return mln_kw_test_js_pool_workers(all);
}

/**
 * Starts a waiting thread on every Worker in the pool, so the pool is empty
 * when this returns. Returns the number of threads started, or -1 when a
 * thread did not start. Call mln_kw_test_pool_release afterwards in either
 * case.
 */
EMSCRIPTEN_KEEPALIVE int32_t mln_kw_test_pool_hold(void) {
  __atomic_store_n(&pool_released, 0, __ATOMIC_SEQ_CST);
  const int32_t count = mln_kw_test_js_pool_workers(0);
  for (int32_t index = 0; index < count; index++) {
    if (start_detached(hold_pool_worker, NULL) != 0) {
      return -1;
    }
  }
  return count;
}

/**
 * Releases the threads that mln_kw_test_pool_hold started. Their Workers
 * return to the pool once the page yields.
 */
EMSCRIPTEN_KEEPALIVE void mln_kw_test_pool_release(void) {
  __atomic_store_n(&pool_released, 1, __ATOMIC_SEQ_CST);
  emscripten_futex_wake(&pool_released, INT32_MAX);
}

// ---------------------------------------------------------------------------
// WebGL canvases
//
// The browser builds keep their own GPU fixtures, since WebGL contexts come
// from JavaScript; see tests/graphics/README.md. The page creates the context
// that its sessions share, and these fixtures add the canvases that a session's
// core worker takes over. The JavaScript halves are in test_support.js.

void mln_kw_test_register_transferred_canvas(
  const char* selector, int32_t width, int32_t height, void* shared
);
void mln_kw_test_unregister_canvas(const char* name);
void mln_kw_test_add_dom_canvas(
  const char* selector, int32_t width, int32_t height, void* shared
);
void mln_kw_test_remove_dom_canvas(const char* selector);

static uint32_t canvas_serial;

/** Makes [context] current on the calling thread. Returns 0 on success. */
EMSCRIPTEN_KEEPALIVE int32_t mln_kw_test_webgl_make_current(int32_t context) {
  return emscripten_webgl_make_context_current(context);
}

/** The canvas that a session's core worker takes over. */
typedef enum mln_kw_test_canvas_kind {
  /** An OffscreenCanvas registered for the transfer, as a worker would be. */
  MLN_KW_TEST_CANVAS_OFFSCREEN = 0,
  /** A canvas element in the document, as a page would show. */
  MLN_KW_TEST_CANVAS_ELEMENT = 1,
} mln_kw_test_canvas_kind;

typedef struct mln_kw_test_canvas {
  /** The selector a WebGL context descriptor names. The first member. */
  char selector[32];
  uint32_t kind;
  /**
   * The width, height, and owning thread that Emscripten keeps for a
   * transferred canvas, at canvasSharedPtr.
   */
  struct {
    int32_t width;
    int32_t height;
    pthread_t owner;
  } shared;
} mln_kw_test_canvas;

_Static_assert(
  sizeof(((mln_kw_test_canvas*)0)->shared) == 12,
  "Emscripten's canvasSharedPtr block is 12 bytes"
);

/**
 * Creates a canvas of [kind] and [width] x [height] that pthread_create
 * transfers to a session's core worker. The returned address is its
 * NUL-terminated selector. Returns null when allocation failed.
 */
EMSCRIPTEN_KEEPALIVE mln_kw_test_canvas* mln_kw_test_canvas_create(
  uint32_t kind, int32_t width, int32_t height
) {
  mln_kw_test_canvas* canvas = calloc(1, sizeof *canvas);
  if (canvas == NULL) {
    return NULL;
  }
  (void)snprintf(
    canvas->selector, sizeof canvas->selector, "#mln-kw-canvas-%u",
    ++canvas_serial
  );
  canvas->kind = kind;
  canvas->shared.width = width;
  canvas->shared.height = height;
  if (kind == MLN_KW_TEST_CANVAS_ELEMENT) {
    mln_kw_test_add_dom_canvas(
      canvas->selector, width, height, &canvas->shared
    );
  } else {
    mln_kw_test_register_transferred_canvas(
      canvas->selector, width, height, &canvas->shared
    );
  }
  return canvas;
}

/**
 * Removes [canvas] from the page and frees it. Call it once the session that
 * took the canvas over has closed.
 */
EMSCRIPTEN_KEEPALIVE void mln_kw_test_canvas_destroy(
  mln_kw_test_canvas* canvas
) {
  if (canvas->kind == MLN_KW_TEST_CANVAS_ELEMENT) {
    mln_kw_test_remove_dom_canvas(canvas->selector);
  } else {
    mln_kw_test_unregister_canvas(canvas->selector);
  }
  free(canvas);
}
