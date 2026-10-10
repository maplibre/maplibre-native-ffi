// Still images through the render fixture.

#include <stdbool.h>
#include <stddef.h>

#include "map.h"

#include "env.h"
#include "wait.h"

typedef struct still_image_wait {
  const mln_test_render_fixture* fixture;
  mln_test_completion* still;
  bool demand_pending;
  mln_status failure;
} still_image_wait;

// Collects the pending demand's result, if it has arrived, and releases any
// frame it rendered so the texture ring never fills.
static bool collect_demand(still_image_wait* wait) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_status drained = mln_render_session_drain_frame_results(
    wait->fixture->session, &batch, MLN_TEST_DIAGNOSTIC
  );
  if (drained == MLN_STATUS_NOT_READY) {
    return false;
  }
  if (drained != MLN_STATUS_OK) {
    wait->failure = drained;
    return false;
  }
  mln_render_frame_batch_release(batch);
  wait->demand_pending = false;
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  while (mln_render_session_acquire_frame(
           wait->fixture->session, &frame, MLN_TEST_DIAGNOSTIC
         ) == MLN_STATUS_OK) {
    const mln_status released =
      mln_acquired_frame_release(&frame, NULL, MLN_TEST_DIAGNOSTIC);
    if (released != MLN_STATUS_OK) {
      wait->failure = released;
      return false;
    }
    frame = MLN_HANDLE_NULL;
  }
  return true;
}

// Holds once the still image has completed and the last demand has settled.
// Until then it keeps exactly one demand in flight, since a static map renders
// its still image only inside a frame. The demands are forced, so each renders
// the latest update whether or not it is new.
static bool still_image_settled(void* context) {
  still_image_wait* wait = context;
  if (wait->demand_pending && !collect_demand(wait)) {
    return wait->failure != MLN_STATUS_OK;
  }
  if (mln_test_completion_poll(wait->still)) {
    return true;
  }
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  const mln_status requested = mln_render_session_request_frame(
    wait->fixture->session, &demand, MLN_TEST_DIAGNOSTIC
  );
  if (requested != MLN_STATUS_OK) {
    wait->failure = requested;
    return true;
  }
  wait->demand_pending = true;
  return false;
}

mln_status mln_test_render_pending_still_image(
  const mln_test_render_fixture* fixture, mln_test_completion* still
) {
  still_image_wait wait = {
    .fixture = fixture,
    .still = still,
    .demand_pending = false,
    .failure = MLN_STATUS_OK,
  };
  const mln_status status = mln_test_render_step_until(
    fixture, still_image_settled, &wait, mln_test_deadline_default(),
    "a still image"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return wait.failure != MLN_STATUS_OK ? wait.failure
                                       : mln_test_completion_status(still);
}

mln_status mln_test_render_still_image(
  const mln_test_render_fixture* fixture, mln_map map
) {
  mln_test_completion still = mln_test_completion_default(0);
  const mln_status submitted =
    mln_map_request_still_image(map, &still.descriptor, MLN_TEST_DIAGNOSTIC);
  if (submitted != MLN_STATUS_OK) {
    still.descriptor.release_user_data(still.descriptor.user_data);
    mln_test_completion_destroy(&still);
    return submitted;
  }
  const mln_status status =
    mln_test_render_pending_still_image(fixture, &still);
  mln_test_completion_destroy(&still);
  return status;
}
