
#include <stdbool.h>
#include <stddef.h>

#include "map.h"

#include "env.h"
#include "frames.h"
#include "wait.h"

typedef struct still_image_wait {
  const mln_test_render_fixture* fixture;
  mln_test_completion* still;
  bool frame_arrived;
  mln_status failure;
} still_image_wait;

// Drains queued frame results, noting the still image's, and releases any
// frame rendered meanwhile so the texture ring never fills.
static void collect_frames(still_image_wait* wait) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_status drained = mln_test_drain_frame_results(
    wait->fixture->session, &batch, MLN_TEST_DIAGNOSTIC
  );
  if (drained == MLN_STATUS_NOT_READY) {
    return;
  }
  if (drained != MLN_STATUS_OK) {
    wait->failure = drained;
    return;
  }
  const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
  for (size_t index = 0; index < view.result_count; ++index) {
    if (
      mln_test_render_view_result(&view, index)->token ==
      MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE
    ) {
      wait->frame_arrived = true;
    }
  }
  mln_render_frame_batch_release(batch);
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  while (mln_render_session_acquire_frame(
           wait->fixture->session, &frame, MLN_TEST_DIAGNOSTIC
         ) == MLN_STATUS_OK) {
    const mln_status released =
      mln_acquired_frame_release(frame, NULL, MLN_TEST_DIAGNOSTIC);
    if (released != MLN_STATUS_OK) {
      wait->failure = released;
      return;
    }
    frame = MLN_HANDLE_NULL;
  }
}

// Holds once the still image has completed and the frame result of the demand
// that rendered it has arrived. The request submits that demand itself.
static bool still_image_settled(void* context) {
  still_image_wait* wait = context;
  collect_frames(wait);
  return wait->failure != MLN_STATUS_OK ||
         (mln_test_completion_poll(wait->still) &&
          (wait->frame_arrived ||
           mln_test_completion_status(wait->still) != MLN_STATUS_OK));
}

mln_status mln_test_render_pending_still_image(
  const mln_test_render_fixture* fixture, mln_test_completion* still
) {
  still_image_wait wait = {
    .fixture = fixture,
    .still = still,
    .frame_arrived = false,
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
