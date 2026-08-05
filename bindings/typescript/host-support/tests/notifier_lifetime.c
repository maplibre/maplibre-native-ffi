/**
 * Proves that a host may release what its notifier reads once it has stopped.
 *
 * The support layer reads an owner's notifier under the queue lock and calls it
 * outside, because the notifier reaches the host's runtime and holding the lock
 * across it would let a host deadlock against a MapLibre thread. That leaves a
 * window in which a host that stops notifications frees the state the notifier
 * reads while a producer is about to read it, and the producer is on MapLibre's
 * thread, so nothing else orders the two.
 *
 * A window that narrow never loses by chance, so this widens it on purpose. The
 * shim calls mln_abi_test_before_notify() between reading the notifier and
 * calling it when built with MLN_ABI_TEST_HOOKS, and the notifier installed
 * here sleeps inside itself, so each phase below puts a stop squarely inside
 * one of the two moments a stop has to survive.
 *
 * Records are pushed through the custom geometry fetch listener, whose payload
 * is a plain allocation this file releases, so nothing here needs a runtime, a
 * map, or a render backend.
 *
 * Build and run:
 *
 *     cc -std=c23 -g -pthread -fsanitize=address -DMLN_ABI_TEST_HOOKS \
 *       -Ibindings/typescript/host-support/include \
 *       -Ibindings/typescript/host-support/generated \
 *       -I<install>/include \
 *       bindings/typescript/host-support/src/mln_abi.c \
 *       bindings/typescript/host-support/tests/notifier_lifetime.c \
 *       -L<install>/lib -lmaplibre-native-c -o notifier-lifetime
 *
 * A sanitizer is what makes this decisive, because it reports the read of freed
 * memory rather than waiting for the bytes to have been reused. Without one the
 * check on the state's marker still catches a release that landed too early,
 * once the allocator has handed the memory on.
 */

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "maplibre_native_c.h"
#include "mln_abi.h"

/** Cycles per phase, and how long a widened window is held open. */
#define CYCLES 60
#define PRODUCERS 4
#define WINDOW_MICROSECONDS 1000L
#define DRAIN_BATCH 64U

/** What a live notifier state reads as, and what a released one does not. */
#define STATE_MARKER 0xc0ffee01u

typedef struct notifier_state {
  uint32_t marker;
  atomic_uint calls;
} notifier_state;

/** How a phase widens the window, and where. */
typedef enum phase_window {
  /* Between the shim reading the notifier and calling it. */
  WINDOW_BEFORE_CALL,
  /* Inside the notifier itself. */
  WINDOW_INSIDE_CALL,
} phase_window;

/** How a phase ends notifications. */
typedef enum phase_teardown {
  /* Clearing the notifier, which is what a cleared callback takes. */
  TEARDOWN_STOP,
  /* Destroying the owner outright, which is what a closed context takes. */
  TEARDOWN_DESTROY,
} phase_teardown;

static atomic_int quit;
static atomic_ullong pushes;
static atomic_ullong notifications;
static atomic_int failures;
/* The identity producers push under. It changes as owners are retired, and a
 * push under a retired identity is released by the shim rather than delivered,
 * which is the outcome that path already promises. */
static atomic_uintptr_t published_identity;
static long before_call_microseconds;
static long inside_call_microseconds;
static void (*fetch_listener)(void*, mln_canonical_tile_id);

static void nap(long microseconds) {
  if (microseconds <= 0) {
    return;
  }
  struct timespec span;
  span.tv_sec = microseconds / 1000000L;
  span.tv_nsec = (microseconds % 1000000L) * 1000L;
  nanosleep(&span, NULL);
}

/* The shim calls this between reading an owner's notifier and calling it. */
void mln_abi_test_before_notify(void) { nap(before_call_microseconds); }

static void notify(void* user_data) {
  notifier_state* state = (notifier_state*)user_data;
  nap(inside_call_microseconds);
  if (state->marker != STATE_MARKER) {
    fprintf(
      stderr, "FAIL: the notifier's state was released under it (0x%08x)\n",
      state->marker
    );
    atomic_fetch_add(&failures, 1);
    return;
  }
  atomic_fetch_add(&state->calls, 1U);
  atomic_fetch_add(&notifications, 1ULL);
}

/* Stands in for a MapLibre thread: pushes records and never synchronizes with
 * the thread that installs and removes notifiers. */
static void* producer(void* unused) {
  (void)unused;
  mln_canonical_tile_id tile;
  memset(&tile, 0, sizeof tile);
  tile.z = 1;
  while (atomic_load(&quit) == 0) {
    fetch_listener((void*)atomic_load(&published_identity), tile);
    atomic_fetch_add(&pushes, 1ULL);
    nap(50);
  }
  return NULL;
}

static void drain(uint64_t owner) {
  mln_abi_record records[DRAIN_BATCH];
  for (;;) {
    const uint32_t count = mln_abi_queue_drain(owner, records, DRAIN_BATCH);
    for (uint32_t index = 0; index < count; ++index) {
      mln_abi_record_destroy(
        records[index].kind, (void*)(uintptr_t)records[index].record
      );
    }
    if (count < DRAIN_BATCH) {
      return;
    }
  }
}

/** Takes an owner and an identity for it, or reports which step refused. */
static int open_owner(uint64_t* owner) {
  *owner = mln_abi_owner_create();
  if (*owner == 0U) {
    fprintf(stderr, "FAIL: no owner\n");
    return 0;
  }
  const uint64_t identity = mln_abi_owner_register(*owner);
  if (identity == 0U) {
    fprintf(stderr, "FAIL: no registration identity\n");
    return 0;
  }
  atomic_store(&published_identity, (uintptr_t)identity);
  return 1;
}

static void run_phase(
  const char* name, phase_window window, phase_teardown teardown
) {
  before_call_microseconds =
    window == WINDOW_BEFORE_CALL ? WINDOW_MICROSECONDS : 0L;
  inside_call_microseconds =
    window == WINDOW_INSIDE_CALL ? WINDOW_MICROSECONDS : 0L;

  uint64_t owner = 0U;
  if (!open_owner(&owner)) {
    atomic_fetch_add(&failures, 1);
    return;
  }

  atomic_store(&quit, 0);
  pthread_t threads[PRODUCERS];
  for (int index = 0; index < PRODUCERS; ++index) {
    pthread_create(&threads[index], NULL, producer, NULL);
  }

  const unsigned long long before = atomic_load(&notifications);
  for (int cycle = 0; cycle < CYCLES; ++cycle) {
    notifier_state* state = malloc(sizeof(*state));
    if (state == NULL) {
      break;
    }
    state->marker = STATE_MARKER;
    atomic_store(&state->calls, 0U);
    mln_abi_queue_set_notifier(owner, notify, state);
    /* Long enough for a producer to have read this notifier and started into
     * whichever moment this phase widened. */
    nap(WINDOW_MICROSECONDS + 200L);

    if (teardown == TEARDOWN_STOP) {
      mln_abi_queue_set_notifier(owner, NULL, NULL);
    } else {
      mln_abi_owner_destroy(owner);
    }
    /* The line the fix exists for. Nothing may still be reading this. */
    state->marker = 0U;
    free(state);

    if (teardown == TEARDOWN_STOP) {
      drain(owner);
    } else if (!open_owner(&owner)) {
      atomic_fetch_add(&failures, 1);
      break;
    }
  }

  atomic_store(&quit, 1);
  for (int index = 0; index < PRODUCERS; ++index) {
    pthread_join(threads[index], NULL);
  }
  drain(owner);
  mln_abi_owner_destroy(owner);

  const unsigned long long delivered = atomic_load(&notifications) - before;
  printf("%s: %llu notifications survived teardown\n", name, delivered);
  if (delivered == 0ULL) {
    /* A phase that never reached the notifier proved nothing, so it is a
     * failure rather than a pass by absence. */
    fprintf(stderr, "FAIL: %s never reached the notifier\n", name);
    atomic_fetch_add(&failures, 1);
  }
}

int main(void) {
  fetch_listener = (void (*)(void*, mln_canonical_tile_id))
    mln_abi_custom_geometry_fetch_listener_address();

  run_phase("stop during the read", WINDOW_BEFORE_CALL, TEARDOWN_STOP);
  run_phase("stop during the call", WINDOW_INSIDE_CALL, TEARDOWN_STOP);
  run_phase("destroy during the read", WINDOW_BEFORE_CALL, TEARDOWN_DESTROY);
  run_phase("destroy during the call", WINDOW_INSIDE_CALL, TEARDOWN_DESTROY);

  const int total = atomic_load(&failures);
  printf(
    "%llu pushes, %llu notifications\n",
    (unsigned long long)atomic_load(&pushes),
    (unsigned long long)atomic_load(&notifications)
  );
  printf(total == 0 ? "ALL CHECKS PASSED\n" : "%d CHECKS FAILED\n", total);
  return total;
}
