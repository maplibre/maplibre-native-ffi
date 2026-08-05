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
//
// **The thread does not park between calls; it returns to its event loop.**
// That is what makes presenting possible at all. A browser composites an
// OffscreenCanvas when the task that drew into it *ends* -- Emscripten's own
// `settings.js` calls it "the implicit swap behavior of WebGL where exiting any
// event callback would automatically perform a flip" -- and a thread sitting in
// `pthread_cond_wait`, which is `Atomics.wait`, never ends the task it was
// started in. Such a thread renders correctly and can never show a frame.
//
// So the thread entry takes an Emscripten runtime keepalive and returns.
// Emscripten keeps the worker alive for as long as that keepalive is held, and
// work reaches it as a proxied task on a module-wide `em_proxying_queue`: a
// submission publishes its call on the queue below and posts one wake, and the
// wake drains everything queued and returns. Ending that task is what
// composites whatever it drew.
//
// **Measured, not assumed.** Putting the blocking loop back and running the
// same suite fails all three of `BrowserPresentationTest`'s cases with the page
// canvas entirely transparent -- "no frame ever reached it" -- while the other
// 145 tests pass, including `rendersABackgroundFrameAndReadsItBack`, which
// reads a texture back rather than presenting it. The surface case is the
// clearest: its own readback of the canvas's default framebuffer still holds
// the right colour in that build, so the map rendered exactly as it does here
// and the frame simply never left the thread that drew it.
//
// **A page canvas is transferred at `pthread_create` and nowhere else.**
// Emscripten hands an OffscreenCanvas to a thread through
// `emscripten_pthread_attr_settransferredcanvases`, which is an attribute of
// the thread's creation, so a canvas a host wants to render onto has to be
// named before this thread starts.
// mln_browser_dispatcher_create_with_canvases() is where a host names them, and
// a host that renders to the page reserves its canvas ids before its first
// call. `webgl_context.c` then creates a context against a transferred canvas
// by name, or against a private OffscreenCanvas of its own when a host is
// headless and reads its frames back instead.
//
// Placing module-local work on this thread is what `submit_task` below is for;
// creating a WebGL context is the first such caller, because a context belongs
// to the thread that created it.
//
// **A diagnostic travels with its completion.** The C API's message is
// thread-local, so a failure produced here belongs to this thread, and the next
// call on this thread overwrites it -- by the time the host resumes there is
// nothing left for it to read, and reading later would report an unrelated
// message rather than none. So the message is copied here, at the moment the
// call finishes, and published beside the status.

#include <emscripten/eventloop.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "browser/dispatcher.h"

#include "browser/dispatch_table.h"
#include "maplibre_native_c/base.h"
#include "maplibre_native_c/diagnostics.h"

// How many calls may be outstanding at once. A caller is parked until its
// answer arrives, so this is a bound on concurrent callers rather than on
// throughput.
#define MLN_BROWSER_COMPLETION_CAPACITY 256

// How much of a failure's message a completion carries, including the
// terminator.
//
// The storage is inline, one buffer per completion slot, so the ring reserves
// 256 * 512 bytes -- 128 KiB, once, with the dispatcher. That is deliberate.
// Submission already reserves a completion slot so that an accepted call always
// has somewhere for its answer to go, and allocating the message instead would
// put an allocation that can fail back inside that guarantee, exactly when
// memory is short and a diagnostic is worth the most.
//
// The C API's own buffer is 4 KiB, so a message can be longer than this and is
// then truncated. Truncation stops at a UTF-8 boundary: a host decodes what
// arrives, and a split sequence would reach it as a replacement character
// rather than as the text native wrote.
#define MLN_BROWSER_DIAGNOSTIC_CAPACITY 512

typedef struct mln_browser_call {
  uint32_t index;
  const mln_browser_slot* slots;
  uint32_t slot_count;
  mln_browser_slot* result;
  // Set for a module-local task and null for a table call, which is what the
  // owner thread branches on. A task carries its own argument and reports no
  // status, so the three fields above are unused when it is set.
  mln_browser_dispatcher_task task;
  void* task_argument;
  uint32_t token;
  struct mln_browser_call* next;
} mln_browser_call;

typedef struct mln_browser_dispatcher {
  pthread_t thread;
  pthread_mutex_t mutex;
  mln_browser_call* head;
  mln_browser_call* tail;
  bool stopping;
  // Set when the host gave up its join, which makes releasing the dispatcher
  // the thread's own last act. Both teardown entry points can set it: a stop
  // always does, and a destroy does when the wake it needs cannot be posted and
  // an older one is still outstanding.
  bool detached;
  // Wakes posted to the owner thread and not yet finished. There is no
  // condition variable to signal any more, so this is what says whether another
  // task may still touch this dispatcher: the last wake to leave, with the
  // queue empty and the dispatcher stopping, is the one that drops the
  // keepalive and -- when the host could not join -- frees this. Counting is
  // what makes that exact; a wake still in flight would otherwise read a
  // dispatcher the previous one had already released.
  uint32_t wakes;

  // Finished calls, oldest first. The host polls this rather than being called
  // back, and that is the whole point: a callback would have to reach the
  // host's agent, which from this thread means proxying to the main runtime
  // thread and running arbitrary host code on Emscripten's system queue -- a
  // context reserved for work that is safe at any system-function boundary,
  // which host callbacks are not. A ring the host drains on its own task has
  // none of that, and costs the same event-loop turn the callback would have.
  uint32_t completed_tokens[MLN_BROWSER_COMPLETION_CAPACITY];
  uint8_t completed_ok[MLN_BROWSER_COMPLETION_CAPACITY];
  // The message the call left behind, copied on the thread that produced it.
  // Empty for a call that did not fail.
  char completed_diagnostics[MLN_BROWSER_COMPLETION_CAPACITY]
                            [MLN_BROWSER_DIAGNOSTIC_CAPACITY];
  uint32_t completed_head;
  uint32_t completed_size;
  // Accepted but not yet collected. Submission is what this bounds, so a call
  // that was accepted always has somewhere for its answer to go: dropping a
  // completion strands its caller forever and cannot even say whose call it
  // was, while refusing the submission tells the caller immediately, at the one
  // point it can still do something about it.
  uint32_t outstanding;
} mln_browser_dispatcher;

// Copies at most `capacity` bytes of `message`, terminator included, cutting on
// a UTF-8 boundary. A null or empty message writes an empty string, which is
// what says the call it belongs to did not fail.
static void mln_browser_copy_diagnostic(
  char* destination, uint32_t capacity, const char* message
) {
  if (destination == NULL || capacity == 0) {
    return;
  }
  if (message == NULL) {
    destination[0] = '\0';
    return;
  }
  uint32_t length = 0;
  while (message[length] != '\0' && length + 1 < capacity) {
    length++;
  }
  if (message[length] != '\0') {
    // `length` indexes the first byte left behind. When that byte continues a
    // sequence, the sequence started inside what would be copied, so the whole
    // of it is dropped rather than half written.
    while (length > 0 && ((unsigned char)message[length] & 0xC0u) == 0x80u) {
      length--;
    }
  }
  memcpy(destination, message, length);
  destination[length] = '\0';
}

// Dispatchers that have been told to stop and whose release has not happened
// yet.
//
// A stop is fire-and-forget: it posts a wake and returns, and that wake is what
// drops the keepalive and frees the dispatcher. So a host that stops has no way
// of its own to learn that the thread got there -- it may not join, and the
// dispatcher it would otherwise ask is the thing being freed. The count lives
// here instead, in the module, because it has to outlive every dispatcher it
// describes.
//
// Counted up wherever a dispatcher becomes the thread's to release, which is
// wherever `detached` is set, and counted down by whichever call performs that
// release. Sequential consistency is what makes it readable from the host's
// thread: a host that reads zero also sees everything the release did.
static atomic_uint pending_stops;

static pthread_once_t queue_once = PTHREAD_ONCE_INIT;
static em_proxying_queue* dispatch_queue;

static void mln_browser_dispatcher_create_queue(void) {
  dispatch_queue = em_proxying_queue_create();
}

// The queue every wake travels on, created once and never destroyed.
//
// Never destroyed because a wake outlives the dispatcher it names: the last one
// to run is what frees a detached dispatcher, and destroying the queue from
// there would be destroying the thing currently executing. One queue for the
// module costs a single allocation and removes the question entirely.
//
// A dedicated queue rather than Emscripten's system queue, for the reason
// src/browser/sync_callback.c states at length: system-queue work runs at
// arbitrary system-function boundaries, which is signal-handler territory, and
// a whole map call is not safe there.
static em_proxying_queue* mln_browser_dispatcher_queue(void) {
  pthread_once(&queue_once, mln_browser_dispatcher_create_queue);
  return dispatch_queue;
}

static void mln_browser_dispatcher_drain(void* argument);

// Posts one wake to the owner thread. The caller has already counted it in
// `wakes` under the mutex, which is what keeps the dispatcher alive across
// this: a free can only happen once no wake is outstanding.
static bool mln_browser_dispatcher_wake(mln_browser_dispatcher* dispatcher) {
  return emscripten_proxy_async(
           mln_browser_dispatcher_queue(), dispatcher->thread,
           mln_browser_dispatcher_drain, dispatcher
         ) != 0;
}

// The thread's entry point, which returns immediately and on purpose.
//
// The keepalive is what keeps the worker alive without it: Emscripten ends a
// pthread when its entry returns and nothing is holding the runtime, and holds
// it open otherwise. Returning is the point -- everything this thread later
// draws is composited because the task that drew it ended, and a thread that
// never left its entry never ends one.
static void* mln_browser_dispatcher_main(void* argument) {
  (void)argument;
  emscripten_runtime_keepalive_push();
  return NULL;
}

// Performs everything queued, then returns, which is the moment the browser
// composites whatever this drew.
//
// One wake is posted per submission, so a wake that finds the queue already
// emptied by an earlier one does nothing and costs an event-loop turn. That is
// cheaper than the alternative -- leaving a call unqueued because a wake was
// already in flight is how a caller ends up parked on an answer nobody will
// produce.
static void mln_browser_dispatcher_drain(void* argument) {
  mln_browser_dispatcher* dispatcher = argument;
  while (true) {
    pthread_mutex_lock(&dispatcher->mutex);
    mln_browser_call* call = dispatcher->head;
    if (call != NULL) {
      dispatcher->head = call->next;
      if (dispatcher->head == NULL) {
        dispatcher->tail = NULL;
      }
    }
    pthread_mutex_unlock(&dispatcher->mutex);
    if (call == NULL) {
      break;
    }

    // A module-local task always runs: there is no index to reject and no slot
    // count to check, because the caller is this module rather than a host
    // packing a buffer by hand. It reports its own outcome through whatever
    // storage it was given, the same way a table call's status reaches the
    // result slot.
    bool ok = true;
    const char* diagnostic = NULL;
    if (call->task != NULL) {
      call->task(call->task_argument);
    } else {
      ok = mln_browser_invoke_here(
        call->index, call->slots, call->slot_count, call->result
      );
      // Read here, on the thread that wrote it, because this is the last moment
      // it exists: the next call placed on this thread replaces it. A
      // status-returning entry point clears the message when it starts, so an
      // empty one means this call did not fail and nothing but a terminator is
      // copied below.
      diagnostic = mln_thread_last_error_message();
    }
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
    mln_browser_copy_diagnostic(
      dispatcher->completed_diagnostics[slot], MLN_BROWSER_DIAGNOSTIC_CAPACITY,
      diagnostic
    );
    dispatcher->completed_size++;
    pthread_mutex_unlock(&dispatcher->mutex);
  }

  pthread_mutex_lock(&dispatcher->mutex);
  dispatcher->wakes--;
  // True at most once. Nothing is queued after stopping is set, and stopping
  // posts exactly one wake of its own, so the count reaches zero with the
  // dispatcher stopping on one wake and no other.
  const bool finished =
    dispatcher->stopping && dispatcher->head == NULL && dispatcher->wakes == 0;
  const bool detached = dispatcher->detached;
  pthread_mutex_unlock(&dispatcher->mutex);
  if (!finished) {
    return;
  }

  // Dropped from inside the task rather than after it, because Emscripten
  // re-checks whether to end the thread as each task returns; popping here is
  // what makes the very next check end it.
  emscripten_runtime_keepalive_pop();
  if (detached) {
    // Stopped by a host that cannot join, so the last wake is what releases the
    // dispatcher; nothing refers to it after the stop call returned, and no
    // other wake is outstanding to read it.
    pthread_mutex_destroy(&dispatcher->mutex);
    free(dispatcher);
    // After the free rather than before it, because this is what a host waits
    // on: the count reaching zero has to mean the release is done rather than
    // that it has started.
    atomic_fetch_sub(&pending_stops, 1);
  }
}

// Takes ownership of `call` and queues it, or refuses and releases it. Shared
// by both submission paths so that the capacity bound, the publication order,
// and the wake are stated once: a second copy would be a second place for the
// accounting that keeps a completion slot reserved to drift.
static bool mln_browser_dispatcher_enqueue(
  mln_browser_dispatcher* dispatcher, mln_browser_call* call
) {
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
  // Published before the wake, so an owner woken by this always finds this call
  // rather than draining an empty queue beside it.
  if (dispatcher->tail == NULL) {
    dispatcher->head = call;
  } else {
    dispatcher->tail->next = call;
  }
  dispatcher->tail = call;
  dispatcher->wakes++;
  pthread_mutex_unlock(&dispatcher->mutex);
  if (mln_browser_dispatcher_wake(dispatcher)) {
    return true;
  }

  // The wake could not be allocated, so this call may have nothing coming to
  // run it. Taken back rather than left queued: a caller that was told its
  // submission was accepted parks on an answer, and an answer that never
  // arrives strands it for the page's lifetime. An earlier wake may already
  // have claimed the call, in which case its answer *is* coming and the
  // submission stands.
  pthread_mutex_lock(&dispatcher->mutex);
  dispatcher->wakes--;
  mln_browser_call* previous = NULL;
  mln_browser_call* current = dispatcher->head;
  while (current != NULL && current != call) {
    previous = current;
    current = current->next;
  }
  const bool reclaimed = current == call;
  if (reclaimed) {
    if (previous == NULL) {
      dispatcher->head = call->next;
    } else {
      previous->next = call->next;
    }
    if (dispatcher->tail == call) {
      dispatcher->tail = previous;
    }
    dispatcher->outstanding--;
  }
  pthread_mutex_unlock(&dispatcher->mutex);
  if (!reclaimed) {
    return true;
  }
  free(call);
  return false;
}

// Writes the id `[at, end)` of `canvas_ids` into `out` as a CSS identifier,
// returning how many bytes it wrote.
//
// An HTML id may be any non-empty string with no ASCII whitespace in it, and a
// CSS identifier is a far smaller set. So `#` and the id concatenated is a
// correct selector only for the ids that already happen to be identifiers:
// `map:canvas` would travel as `#map:canvas`, where `:canvas` parses as a
// pseudo-class and matches no element at all, and an id beginning with a digit
// or containing a dot or a bracket goes wrong the same way -- selecting nothing
// or, worse, selecting some other element. Every one of those is a valid id,
// and a host that used one would see `pthread_create` refuse the whole transfer
// with its canvas reported as missing.
//
// This is `CSS.escape` -- the CSS Object Model's "serialize an identifier" --
// written here rather than called through `EM_JS`, because `CSS` is a `Window`
// interface and a host may create its dispatcher from a worker, where the name
// is not defined at all. The algorithm is specified over code points, but every
// rule below concerns ASCII and UTF-8 spells every other code point as bytes
// >= 0x80, which the "emit unchanged" branch covers as they are; so a byte at a
// time gives the same answer as a code point at a time.
//
// The one rule not implemented is the first: a U+0000 becomes U+FFFD. An id
// arrives here inside a C string and so cannot contain one.
static size_t mln_browser_dispatcher_escape(
  const char* canvas_ids, size_t at, size_t end, char* out
) {
  static const char digits[] = "0123456789abcdef";
  const size_t length = end - at;
  size_t written = 0;
  for (size_t index = 0; index < length; index++) {
    const unsigned char byte = (unsigned char)canvas_ids[at + index];
    const bool numeric = byte >= '0' && byte <= '9';
    // A control character can never appear literally, and a leading digit -- or
    // a digit after a leading '-' -- would be read as the start of a number
    // rather than of a name. All three travel as a hexadecimal escape, which a
    // space terminates.
    if (
      byte <= 0x1FU || byte == 0x7FU || (index == 0 && numeric) ||
      (index == 1 && numeric && canvas_ids[at] == '-')
    ) {
      out[written++] = '\\';
      if (byte >= 0x10U) {
        out[written++] = digits[byte >> 4U];
      }
      out[written++] = digits[byte & 0x0FU];
      out[written++] = ' ';
      continue;
    }
    // A lone '-' is a valid id and not a valid identifier, because an
    // identifier may not be just a hyphen.
    if (byte == '-' && length == 1) {
      out[written++] = '\\';
      out[written++] = '-';
      continue;
    }
    if (
      byte >= 0x80U || byte == '-' || byte == '_' || numeric ||
      (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z')
    ) {
      out[written++] = (char)byte;
      continue;
    }
    out[written++] = '\\';
    out[written++] = (char)byte;
  }
  return written;
}

// Rewrites a comma-separated list of canvas element ids into the selector list
// Emscripten's transfer attribute expects, or returns null when the list names
// nothing.
//
// Emscripten resolves each entry with `document.querySelector`, so `map` has to
// travel as `#map` and anything an identifier cannot spell literally has to be
// escaped on the way -- see mln_browser_dispatcher_escape(). Making the host
// write the selector instead would put a piece of Emscripten's implementation
// in every host's API; an element id is what a host already has.
//
// The comma stays a separator rather than becoming an escapable character.
// Emscripten splits the string it is given before any of this is visible to it,
// so an id containing a comma cannot reach a transfer however it is spelled
// here, and a host is told so at its own boundary.
//
// Surrounding whitespace is dropped so that a host may write the list the way
// it reads, and an empty entry is skipped rather than passed on, because
// `querySelector("#")` throws and Emscripten reports that as a failed transfer
// for the whole thread.
static char* mln_browser_dispatcher_selector(const char* canvas_ids) {
  if (canvas_ids == NULL) {
    return NULL;
  }
  const size_t length = strlen(canvas_ids);
  if (length == 0) {
    return NULL;
  }
  // An escape is at most four bytes for one -- a backslash, two hexadecimal
  // digits, and the space closing them -- and an entry adds its '#' and the
  // comma before it, of which there are fewer than there are characters. Six
  // times the input plus a terminator covers every list.
  char* selector = calloc(length * 6 + 2, 1);
  if (selector == NULL) {
    return NULL;
  }
  size_t written = 0;
  size_t at = 0;
  while (at < length) {
    while (at < length && (canvas_ids[at] == ' ' || canvas_ids[at] == ',')) {
      at++;
    }
    size_t end = at;
    while (end < length && canvas_ids[end] != ',') {
      end++;
    }
    size_t trimmed = end;
    while (trimmed > at && canvas_ids[trimmed - 1] == ' ') {
      trimmed--;
    }
    if (trimmed > at) {
      if (written > 0) {
        selector[written++] = ',';
      }
      selector[written++] = '#';
      written += mln_browser_dispatcher_escape(
        canvas_ids, at, trimmed, selector + written
      );
    }
    at = end + 1;
  }
  selector[written] = '\0';
  if (written == 0) {
    free(selector);
    return NULL;
  }
  return selector;
}

/**
 * Creates a dispatcher, transferring the named page canvases to its thread.
 *
 * `canvas_ids` is a comma-separated list of `id` attributes of `<canvas>`
 * elements in the host's document, or null or empty for none.
 * mln_browser_webgl_context_create() then names one of them to render onto, and
 * what that context draws reaches the page with no copy, because the canvas the
 * page displays *is* the canvas the owner thread renders into.
 *
 * These are element ids, not selectors. Any id an HTML document accepts works,
 * including one a CSS identifier cannot spell literally, because each is
 * escaped before it reaches the selector Emscripten resolves. The one exception
 * is a comma, which separates this list and so cannot appear inside an entry;
 * surrounding whitespace is trimmed, so an id cannot begin or end with a space
 * either.
 *
 * **This is the only moment a canvas can be transferred.** Emscripten performs
 * the transfer inside `pthread_create`, so every canvas a host will ever render
 * onto through this dispatcher is named here, before the first call. A host
 * that names none is not limited to nothing -- `webgl_context.c` creates a
 * private OffscreenCanvas for a context with no canvas named, which is what a
 * host that reads frames back rather than presenting them wants.
 *
 * Returns null when the thread cannot be created, which includes an id that
 * names no element and an element whose control has already been transferred:
 * Emscripten refuses the whole `pthread_create` in either case rather than
 * starting a thread with part of what was asked for.
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
MLN_API mln_browser_dispatcher* mln_browser_dispatcher_create_with_canvases(
  const char* canvas_ids
) MLN_NOEXCEPT {
  // Before anything else, because a dispatcher whose wakes have nowhere to
  // travel would accept calls and never run one.
  if (mln_browser_dispatcher_queue() == NULL) {
    return NULL;
  }
  mln_browser_dispatcher* dispatcher = calloc(1, sizeof(*dispatcher));
  if (dispatcher == NULL) {
    return NULL;
  }
  if (pthread_mutex_init(&dispatcher->mutex, NULL) != 0) {
    free(dispatcher);
    return NULL;
  }

  // Held until `pthread_create` returns: the attribute keeps the pointer rather
  // than a copy, and Emscripten reads the string as it performs the transfer.
  char* selector = mln_browser_dispatcher_selector(canvas_ids);
  if (selector == NULL && canvas_ids != NULL && canvas_ids[0] != '\0') {
    // A host asked for canvases and none could be built -- an all-separator
    // list, or an allocation that failed. Refused rather than quietly starting
    // a thread with nothing transferred to it, which would fail much later, at
    // a context creation that could not say why its canvas was missing.
    pthread_mutex_destroy(&dispatcher->mutex);
    free(dispatcher);
    return NULL;
  }
  pthread_attr_t attributes;
  pthread_attr_t* attributes_used = NULL;
  if (selector != NULL) {
    if (pthread_attr_init(&attributes) != 0) {
      free(selector);
      pthread_mutex_destroy(&dispatcher->mutex);
      free(dispatcher);
      return NULL;
    }
    attributes_used = &attributes;
    (void)emscripten_pthread_attr_settransferredcanvases(&attributes, selector);
  }
  const int created = pthread_create(
    &dispatcher->thread, attributes_used, mln_browser_dispatcher_main,
    dispatcher
  );
  if (attributes_used != NULL) {
    (void)pthread_attr_destroy(attributes_used);
  }
  free(selector);
  if (created != 0) {
    pthread_mutex_destroy(&dispatcher->mutex);
    free(dispatcher);
    return NULL;
  }
  return dispatcher;
}

/**
 * Creates a dispatcher with no page canvas transferred to its thread.
 *
 * mln_browser_dispatcher_create_with_canvases() with nothing named, which is
 * what a host that reads its frames back rather than presenting them wants.
 * Such a host renders into a private OffscreenCanvas or into a texture, and
 * neither is ever displayed.
 */
MLN_API mln_browser_dispatcher* mln_browser_dispatcher_create(
  void
) MLN_NOEXCEPT {
  return mln_browser_dispatcher_create_with_canvases(NULL);
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
  return mln_browser_dispatcher_enqueue(dispatcher, call);
}

/**
 * Places one module-local task on the dispatcher's thread.
 *
 * `task` is a function in this module rather than a host function pointer, and
 * `argument` is whatever it needs; both are opaque here. Everything else works
 * exactly as mln_browser_dispatcher_submit() does -- the same capacity bound,
 * the same token rules, and the same completion the host collects with
 * mln_browser_dispatcher_take_completion(), which reports true because a task
 * has no index or slot count that could be rejected.
 *
 * This exists because some of what the owner thread owns is not reachable
 * through the generated call table. A WebGL context belongs to the thread that
 * created it, so a context this thread renders through has to be created here,
 * and no C API entry point creates one.
 *
 * **The task's own storage follows the same rule the slots do.** It is written
 * on this thread, so nothing may read or release it until the completion for
 * `token` arrives. Returns false under the same conditions submit does, and
 * nothing runs and no completion follows.
 */
MLN_API bool mln_browser_dispatcher_submit_task(
  mln_browser_dispatcher* dispatcher, mln_browser_dispatcher_task task,
  void* argument, uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || task == NULL) {
    return false;
  }
  mln_browser_call* call = calloc(1, sizeof(*call));
  if (call == NULL) {
    return false;
  }
  call->task = task;
  call->task_argument = argument;
  call->token = token;
  return mln_browser_dispatcher_enqueue(dispatcher, call);
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
 *
 * **The one case that does not wait** is a stop that cannot be posted, which
 * needs a single allocation and so happens only under memory pressure. There is
 * then nothing left to join for, and what becomes of the dispatcher depends on
 * whether an older wake is still outstanding: one that is releases the
 * dispatcher when it finishes, and this returns without waiting for it, while
 * with none outstanding the dispatcher is released here. Either way the thread
 * keeps its keepalive and its worker survives for the document's lifetime,
 * because the wake that failed was the only way left to reach it.
 */
MLN_API void mln_browser_dispatcher_destroy(
  mln_browser_dispatcher* dispatcher
) MLN_NOEXCEPT {
  if (dispatcher == NULL) {
    return;
  }
  // Taken while this call is still the only thing that could release the
  // dispatcher, which is what makes reading it safe: nothing frees one that is
  // not detached, and the hand-over below is where that stops being true.
  const pthread_t thread = dispatcher->thread;
  pthread_mutex_lock(&dispatcher->mutex);
  dispatcher->stopping = true;
  dispatcher->wakes++;
  pthread_mutex_unlock(&dispatcher->mutex);
  // The stop travels as a wake like everything else, because the thread is not
  // waiting on anything this could signal: it is in its event loop, and this is
  // what reaches it there. The wake it runs is the one that drops the keepalive
  // and lets Emscripten end the thread, which is what the join below waits for.
  if (mln_browser_dispatcher_wake(dispatcher)) {
    pthread_join(thread, NULL);
    // Safe here and nowhere earlier. The thread ends only after the drain that
    // found no wake outstanding, so a join that has returned is also a promise
    // that nothing is left to read this.
    pthread_mutex_destroy(&dispatcher->mutex);
    free(dispatcher);
    return;
  }

  // The stop could not be posted, so the count taken above has to come back the
  // way mln_browser_dispatcher_enqueue() and mln_browser_dispatcher_stop() take
  // theirs back. Left standing it would be a wake that never arrives, and no
  // later drain could ever find the count at zero.
  pthread_mutex_lock(&dispatcher->mutex);
  dispatcher->wakes--;
  // Another wake still in flight is what makes freeing here a use-after-free
  // rather than a leak, and the reason the count is consulted at all. Such a
  // wake may have published its completion and not yet reached its own
  // decrement, so the host can have collected every answer and still be one
  // lock away from a dispatcher it is about to free. It finds the count at zero
  // when it finishes, with the dispatcher stopping, and that is the drain that
  // pops the keepalive and releases this.
  //
  // Handing the dispatcher over is what gives up the join: a thread has to be
  // detached to release itself, and a detached thread cannot be joined. So a
  // destroy that reaches here returns without waiting, which is the whole of
  // what this failure costs a caller.
  const bool orphaned = dispatcher->wakes == 0;
  if (!orphaned) {
    // Detached before `detached` is published, and through the handle read
    // above: once that flag is visible the wake may free the dispatcher and the
    // thread may end, and detaching a thread that has already ended falls
    // through to a join -- which is exactly what there is no longer anything to
    // wait for.
    pthread_detach(thread);
    atomic_fetch_add(&pending_stops, 1);
    dispatcher->detached = true;
  }
  pthread_mutex_unlock(&dispatcher->mutex);
  if (!orphaned) {
    return;
  }
  // Nothing will ever run on this dispatcher again and nothing else refers to
  // it, so it is released here -- the same lesser harm
  // mln_browser_dispatcher_stop() settles for when its own wake cannot be
  // posted.
  pthread_mutex_destroy(&dispatcher->mutex);
  free(dispatcher);
}

/**
 * Stops the dispatcher's thread without waiting for it.
 *
 * The thread drains what is queued, releases the dispatcher, and exits. The
 * caller must not touch the dispatcher after this returns.
 *
 * All of that happens after this returns, so a host that needs to know it has
 * finished -- one about to terminate the module's worker pool, say -- reads
 * mln_browser_dispatcher_pending_stops() rather than this call's outcome.
 *
 * When the stop cannot be posted to the thread -- the one allocation this
 * needs, and so only under memory pressure -- the dispatcher is released by
 * whichever wake is still outstanding, or here when none is, and the thread is
 * left holding its keepalive because nothing remains that can reach it. That is
 * a worker that survives for the document's lifetime and is the lesser of the
 * two harms; the alternative is leaking the dispatcher beside it.
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
  atomic_fetch_add(&pending_stops, 1);
  dispatcher->detached = true;
  dispatcher->wakes++;
  pthread_mutex_unlock(&dispatcher->mutex);
  // Safe to read the dispatcher here even though the thread may free it,
  // because the count taken above is what a free waits for: no wake can find
  // the count at zero until this one has run.
  if (mln_browser_dispatcher_wake(dispatcher)) {
    return;
  }

  // The stop could not be posted, so the count taken above has to come back the
  // way mln_browser_dispatcher_enqueue() takes its own back. Left standing it
  // would be a wake that never arrives: no later drain could ever find the
  // count at zero, so the keepalive would never be popped and neither the
  // dispatcher nor the worker would ever be released -- a stop that returned
  // successfully while leaving a thread alive for the document's lifetime.
  pthread_mutex_lock(&dispatcher->mutex);
  dispatcher->wakes--;
  // Another wake still in flight is the better outcome, and needs nothing here:
  // it finds the count at zero when it finishes, with the dispatcher stopping,
  // and that is the drain that pops the keepalive and frees this. A queued call
  // always has such a wake, because enqueue counts one before it publishes, so
  // a count of zero here also says the queue is empty.
  const bool orphaned = dispatcher->wakes == 0;
  pthread_mutex_unlock(&dispatcher->mutex);
  if (!orphaned) {
    return;
  }
  // Nothing will ever run on this dispatcher again, and nothing but this call
  // still refers to it -- the host must not touch it after a stop, and no wake
  // is outstanding to read it. So it is released here.
  //
  // The worker is the part that cannot be recovered: its keepalive is held on
  // its own thread and the only way to reach that thread is the wake that just
  // failed. Releasing the dispatcher anyway is the lesser harm, and the same
  // one mln_browser_dispatcher_destroy() settles for when its wake cannot be
  // posted.
  pthread_mutex_destroy(&dispatcher->mutex);
  free(dispatcher);
  // The stop counted above is finished, however little of it happened. A host
  // waiting on the count would otherwise wait out its whole bound for a
  // dispatcher that is already gone.
  atomic_fetch_sub(&pending_stops, 1);
}

/**
 * Reports how many stopped dispatchers have not finished releasing themselves.
 *
 * mln_browser_dispatcher_stop() returns as soon as it has posted its wake, so a
 * host that stops learns nothing about what became of the thread: the wake that
 * drains the queue, drops the keepalive, and frees the dispatcher runs later,
 * and a page may not join a thread to find out. This is what it reads instead.
 * Zero means every dispatcher this module stopped has been released and its
 * thread's last act has run.
 *
 * A host that is about to terminate the module's worker pool polls this from
 * its own task, so that a worker is not killed part way through a teardown it
 * was told to perform. **Bound the wait.** The count includes a thread still
 * inside a call queued before the stop, and a host that stopped with work
 * outstanding would otherwise wait for it here rather than at the drain where
 * the contract puts it.
 *
 * A destroy counts too, but only in the one case where it cannot wait -- see
 * mln_browser_dispatcher_destroy(). A destroy that joins has already waited for
 * everything this reports.
 */
MLN_API uint32_t mln_browser_dispatcher_pending_stops(void) MLN_NOEXCEPT {
  return atomic_load(&pending_stops);
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
 * `out_diagnostic` receives the message that call left on the owner thread, as
 * null-terminated UTF-8 of at most `diagnostic_capacity` bytes including the
 * terminator, truncated on a UTF-8 boundary. It is empty for a call that did
 * not fail. **This is the only place that message can be had.** It is
 * thread-local to the thread that produced it, and the next call placed there
 * overwrites it, so a host that asked mln_thread_last_error_message() after
 * resuming would read its own empty slot. Pass null, and zero, to drop it.
 *
 * The host drains this from its own task and resolves whatever it parked on
 * that token.
 */
MLN_API bool mln_browser_dispatcher_take_completion(
  mln_browser_dispatcher* dispatcher, uint32_t* out_token, uint32_t* out_ok,
  char* out_diagnostic, uint32_t diagnostic_capacity
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_token == NULL || out_ok == NULL) {
    return false;
  }
  bool taken = false;
  pthread_mutex_lock(&dispatcher->mutex);
  if (dispatcher->completed_size > 0) {
    *out_token = dispatcher->completed_tokens[dispatcher->completed_head];
    *out_ok = dispatcher->completed_ok[dispatcher->completed_head];
    mln_browser_copy_diagnostic(
      out_diagnostic, diagnostic_capacity,
      dispatcher->completed_diagnostics[dispatcher->completed_head]
    );
    dispatcher->completed_head =
      (dispatcher->completed_head + 1) % MLN_BROWSER_COMPLETION_CAPACITY;
    dispatcher->completed_size--;
    dispatcher->outstanding--;
    taken = true;
  }
  pthread_mutex_unlock(&dispatcher->mutex);
  return taken;
}
