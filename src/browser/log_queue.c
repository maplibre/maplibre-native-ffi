// The browser log queue.
//
// MapLibre dispatches a log record from whichever thread produced it, and
// `callback_adapter.h` already copies that record into native-owned storage so
// a host that cannot answer synchronously still gets the payload. What it
// cannot do for a browser host is deliver it: its listener is a C function
// pointer, and a JavaScript function installed with Emscripten's `addFunction`
// is bound to the agent that installed it, so a call from a MapLibre worker
// pthread reaches nothing (emscripten-core#21273).
//
// So the listener is compiled in here instead, and it queues. The host drains
// the queue from its own thread, which is the same shape the C API already uses
// for runtime events: work is collected where it happens and read where the
// host lives.
//
// The queue holds record pointers, not copies. The adapter owns each record
// until the host destroys it with `mln_adapter_log_record_destroy`, so nothing
// here duplicates a payload that is already native-owned.
//
// **This registers once and never unregisters.** That is a deliberate
// simplification, and it is what makes the rest of this file boring.
// `mln_adapter_log_record_listener` takes no user data, while the adapter
// treats the state's address as the registration's identity -- so a compiled-in
// listener cannot tell which registration dispatched a record. Every scheme
// that tries to reconstruct that (generation counters, per-epoch states,
// retirement flags) has a race, because retirement is asynchronous while
// registration calls are not: a record can be produced under one registration
// and queued under the next.
//
// A single lifetime registration removes the question. The host swaps which of
// its own callbacks receives records, which it can do exactly, and native never
// sees a registration change. The cost is that after the host stops listening
// the adapter keeps copying records into this queue; the host's drain parks
// rather than releasing them one by one, so they accumulate in the bounded ring
// below until the next callback marks past them. Either way it stops mattering
// as soon as the page goes away.
//
// **This owns the process-global log callback.** Anything registering through
// `mln_log_set_callback` or `mln_adapter_log_set_callback` afterwards retires
// this one, and nothing here notices: `log_installed` stays true, a later
// install reports success without restoring anything, and browser log delivery
// is gone for good. A browser host uses this and nothing else.
//
// Giving the adapter's listener a user-data parameter would let this unregister
// safely, and would serve every host that cannot answer a callback
// synchronously. That belongs in `callback_adapter.h` rather than here.

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/callback_adapter.h"

// A bounded ring, because an unbounded one lets a host that stops draining turn
// a logging burst into the page's memory ceiling. Overflow drops the oldest
// record and counts the drop, so the host can report that it lost records
// rather than believing it saw them all.
#define MLN_BROWSER_LOG_CAPACITY 1024

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static void* log_records[MLN_BROWSER_LOG_CAPACITY];
static uint32_t log_head = 0;
static uint32_t log_size = 0;
static uint64_t log_dropped = 0;
static bool log_installed = false;
// Total records ever enqueued. A host takes a mark before it starts listening
// and then ignores anything older, which is the only exact way to say "produced
// before this callback existed": the counter moves under the same lock that
// enqueues, so there is no window between reading it and a record arriving.
static uint64_t log_enqueued = 0;
static uint64_t log_marks[MLN_BROWSER_LOG_CAPACITY];

// Called from any MapLibre thread. It takes ownership of `record` and must
// return promptly, so it does nothing but queue.
static void mln_browser_log_listener(void* record) {
  if (record == NULL) {
    // The adapter sends one null record when a registration retires. This
    // registration never retires, so nothing here waits on it.
    return;
  }
  void* evicted = NULL;
  pthread_mutex_lock(&log_mutex);
  if (log_size == MLN_BROWSER_LOG_CAPACITY) {
    evicted = log_records[log_head];
    log_head = (log_head + 1) % MLN_BROWSER_LOG_CAPACITY;
    log_size--;
    log_dropped++;
  }
  const uint32_t slot = (log_head + log_size) % MLN_BROWSER_LOG_CAPACITY;
  log_records[slot] = record;
  log_marks[slot] = log_enqueued++;
  log_size++;
  pthread_mutex_unlock(&log_mutex);
  // Freed outside the lock: destroying a record is the adapter's work, not this
  // queue's, and it should not hold up a thread trying to log.
  if (evicted != NULL) {
    mln_adapter_log_record_destroy(evicted);
  }
}

// Registered once for the module's lifetime, so its address stays the identity
// the adapter recorded.
static mln_adapter_log_callback_state log_state = {
  .listener = mln_browser_log_listener,
  .consume = 0,
};

/**
 * Installs the queueing log callback, once.
 *
 * `consume` is what MapLibre is told about every dispatched record, because the
 * host cannot answer in time. Calling this again is a no-op that reports
 * success: the registration is for the module's lifetime, and a host stops
 * receiving records by dropping its own callback rather than by unregistering.
 *
 * **Call this from one thread.** A browser host is a single JavaScript agent,
 * and this is not a concurrent installer: a second caller that arrives while
 * the first is still registering is told the callback is installed before it
 * actually is, and would observe success even if that registration then failed.
 * The lock below protects the queue, not this sequence.
 */
MLN_API mln_status mln_browser_log_install(uint32_t consume) MLN_NOEXCEPT {
  // Claimed in one critical section rather than checked in one and published in
  // another, so two callers cannot both decide to register: the second would
  // register the same address again, overwriting the adapter's in-flight
  // bookkeeping and the consume value the first one fixed.
  //
  // The registration itself happens outside the lock, because the listener
  // takes this same lock and the adapter may dispatch while installing.
  pthread_mutex_lock(&log_mutex);
  const bool claimed = !log_installed;
  if (claimed) {
    log_installed = true;
    log_state.consume = consume;
  }
  pthread_mutex_unlock(&log_mutex);
  if (!claimed) {
    return MLN_STATUS_OK;
  }
  const mln_status status = mln_adapter_log_set_callback(&log_state);
  if (status != MLN_STATUS_OK) {
    pthread_mutex_lock(&log_mutex);
    log_installed = false;
    pthread_mutex_unlock(&log_mutex);
  }
  return status;
}

/**
 * Reports how many records have been enqueued so far.
 *
 * A host reads this before it starts listening and passes it back to
 * `mln_browser_log_take_since`, which is what keeps a record produced while
 * nobody was listening from reaching whoever listens next.
 */
MLN_API uint64_t mln_browser_log_mark(void) MLN_NOEXCEPT {
  pthread_mutex_lock(&log_mutex);
  const uint64_t mark = log_enqueued;
  pthread_mutex_unlock(&log_mutex);
  return mark;
}

/**
 * Takes the oldest record enqueued at or after `mark`, or null when there is
 * none.
 *
 * Anything older is released here rather than returned, because it belongs to a
 * period the caller was not listening for. The caller owns what it gets back
 * and releases it with `mln_adapter_log_record_destroy`.
 */
MLN_API void* mln_browser_log_take_since(uint64_t mark) MLN_NOEXCEPT {
  while (true) {
    void* record = NULL;
    bool older = false;
    pthread_mutex_lock(&log_mutex);
    if (log_size > 0) {
      record = log_records[log_head];
      older = log_marks[log_head] < mark;
      log_head = (log_head + 1) % MLN_BROWSER_LOG_CAPACITY;
      log_size--;
    }
    pthread_mutex_unlock(&log_mutex);
    if (record == NULL) {
      return NULL;
    }
    if (!older) {
      return record;
    }
    mln_adapter_log_record_destroy(record);
  }
}

/**
 * Reports how many records the ring dropped, and resets the count.
 *
 * A drop means the ring was full when a record arrived. That happens when the
 * host has stopped draining, and equally when producers outrun a drain that is
 * running -- a logging burst against a fixed ring and a task-paced reader. A
 * host reads this beside its drain so a gap in its log is something it can
 * report rather than something it cannot see, but it should not read a non-zero
 * count as proof that it stopped listening.
 */
MLN_API uint64_t mln_browser_log_take_dropped(void) MLN_NOEXCEPT {
  pthread_mutex_lock(&log_mutex);
  const uint64_t dropped = log_dropped;
  log_dropped = 0;
  pthread_mutex_unlock(&log_mutex);
  return dropped;
}
