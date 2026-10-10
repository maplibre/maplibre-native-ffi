// The camera end-handler probe.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "camera.h"

#include "wait.h"

static void record_end(void* user_data, const mln_camera_transition_end* end) {
  mln_test_transition_end* probe = user_data;
  if (probe->hook != NULL) {
    probe->hook(probe, end);
  }
  atomic_store(&probe->outcome, end->outcome);
  atomic_store(&probe->generation, end->generation);
  atomic_store(&probe->end_size, end->size);
  atomic_fetch_add(&probe->calls, 1);
  mln_test_pulse();
}

static void record_release(void* user_data) {
  mln_test_transition_end* probe = user_data;
  if (atomic_load(&probe->calls) == 0) {
    atomic_store(&probe->released_first, true);
  }
  atomic_fetch_add(&probe->releases, 1);
  mln_test_pulse();
}

void mln_test_transition_end_init(mln_test_transition_end* probe) {
  atomic_init(&probe->calls, 0);
  atomic_init(&probe->releases, 0);
  atomic_init(&probe->released_first, false);
  atomic_init(&probe->outcome, UINT32_MAX);
  atomic_init(&probe->generation, UINT64_MAX);
  atomic_init(&probe->end_size, 0);
  probe->hook = NULL;
  probe->context = NULL;
}

mln_camera_transition_handler mln_test_transition_end_handler(
  mln_test_transition_end* probe
) {
  return (mln_camera_transition_handler){
    .callback = record_end,
    .user_data = probe,
    .release_user_data = record_release,
  };
}

bool mln_test_wait_transition_end(mln_test_transition_end* probe) {
  return mln_test_wait_for_count(&probe->releases, 1);
}
