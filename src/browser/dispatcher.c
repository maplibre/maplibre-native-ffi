// The owner-thread dispatcher.
//
// A browser host cannot own the thread its map runs on. MapLibre blocks --
// `waitForEmpty` drains a queue, `Thread<>` makes synchronous cross-thread
// calls, teardown joins -- and the page may not block, so a runtime created on
// the page would deadlock the first time any of those happened. WebAssembly
// garbage-collected references also cannot cross agents, so a Kotlin host
// cannot create a thread even if it wanted to.
//
// So this owns one. A dispatcher is a pthread with a queue: the host submits a
// call by index and argument slots, and the pthread performs it through the
// same generated table a direct call uses. Everything the C API says about
// owner threads then holds without the host owning anything -- the runtime is
// created on this thread, every call reaches it here, and it is destroyed here.
//
// Answers come back through a ring the host polls rather than through a
// callback this thread invokes. Calling the host from here would mean proxying
// to the main runtime thread and running host code on Emscripten's system
// queue, which is reserved for work safe at any system-function boundary; host
// code is not that.
//
// The host never blocks. It submits, drains the ring on whatever cadence it
// already runs, and parks its caller in between with its own mechanism -- for a
// Kotlin/Wasm host, a JavaScript promise resolved when the token comes back.

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "browser/dispatch_table.h"
#include "maplibre_native_c/base.h"

// How many calls may be outstanding at once. A caller is parked until its
// answer arrives, so this is a bound on concurrent callers rather than on
// throughput.
#define MLN_BROWSER_COMPLETION_CAPACITY 256

typedef struct mln_browser_call {
  uint32_t index;
  const mln_browser_slot* slots;
  uint32_t slot_count;
  mln_browser_slot* result;
  uint32_t token;
  struct mln_browser_call* next;
} mln_browser_call;

typedef struct mln_browser_dispatcher {
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t ready;
  mln_browser_call* head;
  mln_browser_call* tail;
  bool stopping;
  bool started;
  // Set when the host stopped without joining, which makes releasing the
  // dispatcher the thread's own last act.
  bool detached;

  // Finished calls, oldest first. The host polls this rather than being called
  // back, and that is the whole point: a callback would have to reach the
  // host's agent, which from this thread means proxying to the main runtime
  // thread and running arbitrary host code on Emscripten's system queue -- a
  // context reserved for work that is safe at any system-function boundary,
  // which host callbacks are not. A ring the host drains on its own task has
  // none of that, and costs the same event-loop turn the callback would have.
  uint32_t completed_tokens[MLN_BROWSER_COMPLETION_CAPACITY];
  uint8_t completed_ok[MLN_BROWSER_COMPLETION_CAPACITY];
  uint32_t completed_head;
  uint32_t completed_size;
  // Accepted but not yet collected. Submission is what this bounds, so a call
  // that was accepted always has somewhere for its answer to go: dropping a
  // completion strands its caller forever and cannot even say whose call it
  // was, while refusing the submission tells the caller immediately, at the one
  // point it can still do something about it.
  uint32_t outstanding;
} mln_browser_dispatcher;

static void* mln_browser_dispatcher_main(void* argument) {
  mln_browser_dispatcher* dispatcher = argument;
  while (true) {
    pthread_mutex_lock(&dispatcher->mutex);
    while (dispatcher->head == NULL && !dispatcher->stopping) {
      // Legal here and nowhere the host lives: this is a worker.
      pthread_cond_wait(&dispatcher->ready, &dispatcher->mutex);
    }
    if (dispatcher->head == NULL && dispatcher->stopping) {
      const bool detached = dispatcher->detached;
      pthread_mutex_unlock(&dispatcher->mutex);
      if (detached) {
        // Stopped by a host that cannot join, so the thread is what releases
        // the dispatcher; nothing refers to it after the stop call returned.
        pthread_cond_destroy(&dispatcher->ready);
        pthread_mutex_destroy(&dispatcher->mutex);
        free(dispatcher);
      }
      return NULL;
    }
    mln_browser_call* call = dispatcher->head;
    dispatcher->head = call->next;
    if (dispatcher->head == NULL) {
      dispatcher->tail = NULL;
    }
    pthread_mutex_unlock(&dispatcher->mutex);

    const bool ok = mln_browser_invoke_here(
      call->index, call->slots, call->slot_count, call->result
    );
    const uint32_t token = call->token;
    free(call);

    pthread_mutex_lock(&dispatcher->mutex);
    // Cannot overflow: submission refuses once `outstanding` reaches capacity,
    // so every accepted call already has a slot reserved for its answer.
    const uint32_t slot =
      (dispatcher->completed_head + dispatcher->completed_size) %
      MLN_BROWSER_COMPLETION_CAPACITY;
    dispatcher->completed_tokens[slot] = token;
    dispatcher->completed_ok[slot] = ok ? 1u : 0u;
    dispatcher->completed_size++;
    pthread_mutex_unlock(&dispatcher->mutex);
  }
}

/**
 * Creates a dispatcher and the thread that owns whatever runs on it.
 *
 * Returns null when the thread cannot be created.
 *
 * Answers are collected with mln_browser_dispatcher_take_completion(). There is
 * no readiness signal and no callback: a host drains on whatever cadence it
 * already runs. Waiting for something to call it would wait forever.
 *
 * **One thread owns a dispatcher's lifetime.** Creating, submitting, polling,
 * and stopping are not serialized against each other here -- the mutex protects
 * the fields, not the object -- so a stop that races a submit can free the
 * dispatcher while that submit is still reaching for the lock. A browser host
 * is a single JavaScript agent, which is what makes that safe; a host with more
 * than one thread must serialize them itself.
 */
MLN_API mln_browser_dispatcher* mln_browser_dispatcher_create(
  void
) MLN_NOEXCEPT {
  mln_browser_dispatcher* dispatcher = calloc(1, sizeof(*dispatcher));
  if (dispatcher == NULL) {
    return NULL;
  }
  if (pthread_mutex_init(&dispatcher->mutex, NULL) != 0) {
    free(dispatcher);
    return NULL;
  }
  if (pthread_cond_init(&dispatcher->ready, NULL) != 0) {
    pthread_mutex_destroy(&dispatcher->mutex);
    free(dispatcher);
    return NULL;
  }
  if (
    pthread_create(
      &dispatcher->thread, NULL, mln_browser_dispatcher_main, dispatcher
    ) != 0
  ) {
    pthread_cond_destroy(&dispatcher->ready);
    pthread_mutex_destroy(&dispatcher->mutex);
    free(dispatcher);
    return NULL;
  }
  dispatcher->started = true;
  return dispatcher;
}

/**
 * Places one call on the dispatcher's thread.
 *
 * `slots` and `result` stay the host's, and neither they nor anything a slot
 * points at may be read, written, or released until the completion arrives. The
 * worker does not merely read them: `result`, a struct-return destination, and
 * any output parameter are *written* on the dispatcher's thread, so a host that
 * treats slot storage as input-only will race its own outputs. Where the entry
 * point borrows for longer than the call, that longer requirement applies on
 * top of this one.
 *
 * **A callback reachable from a submitted call must be callable on this
 * thread.** Native invokes a resource provider, transform, or custom-geometry
 * callback from the dispatcher's thread or a MapLibre worker, and an Emscripten
 * `addFunction` trampoline installed on the page reaches nothing from there --
 * the same constraint `log_queue.c` exists because of. A host passes
 * native-side rules or a queueing shim, not a page-agent function pointer.
 *
 * `token` comes back with that completion so a host can match an answer to its
 * question, and **must be unique among outstanding calls**. Nothing here checks
 * that: two outstanding calls sharing a token would resolve whichever the host
 * mapped last, resuming one caller before its result was written and stranding
 * the other.
 *
 * Returns false when the dispatcher is stopping, when as many calls are already
 * outstanding as there are completion slots, or when the call cannot be queued
 * -- in which case no completion is reported for `token` and the caller must
 * not park on it. Refusing here is deliberate: a call that is accepted always
 * has a slot waiting for its answer, so a completion is never dropped after the
 * point where the caller could still have done something about it.
 */
MLN_API bool mln_browser_dispatcher_submit(
  mln_browser_dispatcher* dispatcher, uint32_t index,
  const mln_browser_slot* slots, uint32_t slot_count, mln_browser_slot* result,
  uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || result == NULL) {
    return false;
  }
  mln_browser_call* call = calloc(1, sizeof(*call));
  if (call == NULL) {
    return false;
  }
  call->index = index;
  call->slots = slots;
  call->slot_count = slot_count;
  call->result = result;
  call->token = token;

  pthread_mutex_lock(&dispatcher->mutex);
  if (
    dispatcher->stopping ||
    dispatcher->outstanding == MLN_BROWSER_COMPLETION_CAPACITY
  ) {
    pthread_mutex_unlock(&dispatcher->mutex);
    free(call);
    return false;
  }
  dispatcher->outstanding++;
  // Published before the wake, so an owner that wakes, finds the queue, and
  // re-checks always sees this call rather than parking again beside it.
  if (dispatcher->tail == NULL) {
    dispatcher->head = call;
  } else {
    dispatcher->tail->next = call;
  }
  dispatcher->tail = call;
  pthread_cond_signal(&dispatcher->ready);
  pthread_mutex_unlock(&dispatcher->mutex);
  return true;
}

/**
 * Stops the dispatcher's thread and releases it, waiting for the thread to
 * exit.
 *
 * Calls already queued run first, but their completions become unreachable once
 * this returns, so the same precondition as mln_browser_dispatcher_stop()
 * applies: a host destroys what it owns, drains its outstanding calls, and only
 * then destroys the dispatcher.
 *
 * This **joins**, which Emscripten implements by spinning. A page host must not
 * call it and uses mln_browser_dispatcher_stop() instead; only a host on a
 * thread that may block joins.
 */
MLN_API void mln_browser_dispatcher_destroy(
  mln_browser_dispatcher* dispatcher
) MLN_NOEXCEPT {
  if (dispatcher == NULL) {
    return;
  }
  pthread_mutex_lock(&dispatcher->mutex);
  dispatcher->stopping = true;
  pthread_cond_signal(&dispatcher->ready);
  pthread_mutex_unlock(&dispatcher->mutex);
  if (dispatcher->started) {
    pthread_join(dispatcher->thread, NULL);
  }
  pthread_cond_destroy(&dispatcher->ready);
  pthread_mutex_destroy(&dispatcher->mutex);
  free(dispatcher);
}

/**
 * Stops the dispatcher's thread without waiting for it.
 *
 * The thread drains what is queued, releases the dispatcher, and exits. The
 * caller must not touch the dispatcher after this returns.
 *
 * **Stop with nothing outstanding and nothing live.** A call still in flight
 * has argument and result storage the worker may still be reading, and its
 * answer becomes uncollectable the moment this returns. An owner-affine handle
 * -- a runtime, a map, a render session -- is worse: this thread is the only
 * one that may destroy it, so stopping while one is live loses it for good. A
 * host destroys what it owns, drains its outstanding calls first, and only then
 * stops.
 *
 * This exists because `mln_browser_dispatcher_destroy` joins, and Emscripten
 * implements a join by spinning -- which a page may not do, and which a build
 * that forbids blocking on the main thread aborts on outright. A page host
 * stops and forgets; only a host that may block joins.
 */
MLN_API void mln_browser_dispatcher_stop(
  mln_browser_dispatcher* dispatcher
) MLN_NOEXCEPT {
  if (dispatcher == NULL) {
    return;
  }
  // Detached before the thread is told to stop, and through a local handle.
  // Both matter: once stopping is signalled the thread may exit and free the
  // dispatcher, so reading `dispatcher->thread` afterwards is a use-after-free,
  // and detaching a thread that has already exited falls through to a join --
  // which is exactly what this exists to avoid.
  const pthread_t thread = dispatcher->thread;
  pthread_detach(thread);
  pthread_mutex_lock(&dispatcher->mutex);
  dispatcher->stopping = true;
  dispatcher->detached = true;
  pthread_cond_signal(&dispatcher->ready);
  pthread_mutex_unlock(&dispatcher->mutex);
}

/**
 * Takes one finished call's token, or reports that none is waiting.
 *
 * `out_ok` reports whether the entry point was *invoked*, not whether the
 * submission was accepted -- everything reported here came from an accepted
 * submission. It is false when the index or the slot count was rejected, and
 * `result` is then unwritten, so a host must not read a result it did not get a
 * true for.
 *
 * The host drains this from its own task and resolves whatever it parked on
 * that token.
 */
MLN_API bool mln_browser_dispatcher_take_completion(
  mln_browser_dispatcher* dispatcher, uint32_t* out_token, uint32_t* out_ok
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_token == NULL || out_ok == NULL) {
    return false;
  }
  bool taken = false;
  pthread_mutex_lock(&dispatcher->mutex);
  if (dispatcher->completed_size > 0) {
    *out_token = dispatcher->completed_tokens[dispatcher->completed_head];
    *out_ok = dispatcher->completed_ok[dispatcher->completed_head];
    dispatcher->completed_head =
      (dispatcher->completed_head + 1) % MLN_BROWSER_COMPLETION_CAPACITY;
    dispatcher->completed_size--;
    dispatcher->outstanding--;
    taken = true;
  }
  pthread_mutex_unlock(&dispatcher->mutex);
  return taken;
}
