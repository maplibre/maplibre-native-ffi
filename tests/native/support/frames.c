
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "frames.h"

#include "env.h"
#include "status.h"
#include "unity.h"
#include "wait.h"

// The style turns its transition off. A default transition keeps the map
// publishing an update after every frame for its 300 ms, and nothing paces a
// fixture's frames, so a wait for idle would render thousands of them. A
// browser WebGPU device then spends seconds after the case on their returns.
static const char prepared_style_json[] =
  "{\"version\":8,\"transition\":{\"duration\":0,\"delay\":0},"
  "\"sources\":{},\"layers\":[]}";

void mln_test_render_prepare_map(mln_map map) {
  MLN_TEST_OK(
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(prepared_style_json))
  );
  MLN_TEST_OK(mln_test_map_fence(map));
}

void mln_test_render_request_forced(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  demand.flags = MLN_FRAME_DEMAND_FORCE;
  demand.token = token;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
}

typedef struct results_wait {
  const mln_test_render_fixture* fixture;
  mln_render_frame_batch batch;
  bool failed;
} results_wait;

// Holds once a drain yields results.
static bool results_ready(void* context) {
  results_wait* wait = context;
  const mln_status status =
    mln_test_drain_frame_results(wait->fixture->session, &wait->batch, NULL);
  wait->failed = status != MLN_STATUS_OK && status != MLN_STATUS_NOT_READY;
  return status != MLN_STATUS_NOT_READY;
}

typedef struct result_list_wait {
  const mln_test_render_fixture* fixture;
  size_t count;
  size_t received;
  mln_render_frame_result* out;
  bool failed;
} result_list_wait;

// Holds once `count` results have arrived, copying each drain's in order.
static bool result_list_ready(void* context) {
  result_list_wait* wait = context;
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_status status =
    mln_test_drain_frame_results(wait->fixture->session, &batch, NULL);
  if (status == MLN_STATUS_NOT_READY) return false;
  if (status != MLN_STATUS_OK) {
    wait->failed = true;
    return true;
  }
  const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
  for (size_t index = 0; index < view.result_count; index += 1) {
    if (wait->received < wait->count) {
      wait->out[wait->received] = *mln_test_render_view_result(&view, index);
    }
    wait->received += 1;
  }
  mln_render_frame_batch_release(batch);
  return wait->received >= wait->count;
}

mln_render_frame_batch mln_test_render_wait_for_results(
  const mln_test_render_fixture* fixture
) {
  results_wait wait = {.fixture = fixture, .batch = MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, results_ready, &wait, mln_test_deadline_default(), "frame results"
  ));
  TEST_ASSERT_FALSE(wait.failed);
  return wait.batch;
}

void mln_test_render_wait_for_result_list(
  const mln_test_render_fixture* fixture, size_t count,
  mln_render_frame_result* out
) {
  result_list_wait wait = {.fixture = fixture, .count = count, .out = out};
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, result_list_ready, &wait, mln_test_deadline_default(),
    "frame results"
  ));
  TEST_ASSERT_FALSE(wait.failed);
  TEST_ASSERT_EQUAL_size_t(count, wait.received);
}

// The views of recent drains, by batch. A case holds few batches at once.
static struct {
  mln_render_frame_batch batch;
  mln_render_frame_batch_view view;
} drained_views[64];
static size_t next_drained_view = 0;

mln_status mln_test_drain_frame_results(
  mln_render_session session, mln_render_frame_batch* out_batch,
  mln_diagnostic* out_diagnostic
) {
  mln_render_frame_batch_view view = {
    .size = sizeof(mln_render_frame_batch_view)
  };
  const mln_status status = mln_render_session_drain_frame_results(
    session, out_batch, &view, out_diagnostic
  );
  if (status == MLN_STATUS_OK) {
    const size_t slot =
      next_drained_view++ % (sizeof(drained_views) / sizeof(*drained_views));
    drained_views[slot].batch = *out_batch;
    drained_views[slot].view = view;
  }
  return status;
}

mln_render_frame_batch_view mln_test_render_batch_view(
  mln_render_frame_batch batch
) {
  for (size_t index = 0; index < sizeof(drained_views) / sizeof(*drained_views);
       index += 1) {
    if (drained_views[index].batch == batch && batch != MLN_HANDLE_NULL) {
      const mln_render_frame_batch_view view = drained_views[index].view;
      TEST_ASSERT_GREATER_OR_EQUAL_UINT32(
        sizeof(mln_render_frame_result), view.result_size
      );
      return view;
    }
  }
  TEST_FAIL_MESSAGE("the batch did not come from mln_test_drain_frame_results");
  return (mln_render_frame_batch_view){0};
}

const mln_render_frame_result* mln_test_render_view_result(
  const mln_render_frame_batch_view* view, size_t index
) {
  TEST_ASSERT_LESS_THAN_size_t(view->result_count, index);
  return MLN_STRIDED_AT(
    mln_render_frame_result, view->results, view->result_size, index
  );
}

mln_render_frame_result mln_test_render_batch_result(
  mln_render_frame_batch batch, size_t index
) {
  const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
  return *mln_test_render_view_result(&view, index);
}

// Whether a rendered frame shows the map settled: fully loaded, wanting no
// further frame, and drawing the newest update the map has published.
static bool frame_shows_settled_map(
  mln_map map, const mln_render_frame_result* frame
) {
  if (frame->mode != MLN_RENDER_MODE_FULL || frame->needs_repaint) {
    return false;
  }
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  return !snapshot.transition_in_progress &&
         frame->map_update_generation >=
           snapshot.latest_render_update_generation;
}

void mln_test_render_until_idle(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
) {
  MLN_TEST_OK(mln_test_map_fence(map));
  const mln_test_deadline deadline = mln_test_deadline_default();
  // The first demand renders an update the case published. When there is none,
  // a forced frame shows whether the update the session last drew settled the
  // map. After an unsettled frame, the next update brings what the map lacked.
  mln_frame_demand_flags flags = 0;
  for (;;) {
    mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
    demand.flags = flags;
    MLN_TEST_OK(
      mln_render_session_request_frame(fixture->session, &demand, NULL)
    );
    const mln_render_frame_batch batch =
      mln_test_render_wait_for_results(fixture);
    const mln_render_frame_result frame =
      mln_test_render_batch_result(batch, 0);
    mln_render_frame_batch_release(batch);
    (void)mln_test_drain_all(runtime);
    if (frame.disposition == MLN_RENDER_RESULT_RENDERED) {
      if (frame_shows_settled_map(map, &frame)) {
        return;
      }
      flags = MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
    } else if (frame.disposition == MLN_RENDER_RESULT_NO_UPDATE) {
      flags = MLN_FRAME_DEMAND_FORCE;
    }
    TEST_ASSERT_FALSE_MESSAGE(
      mln_test_deadline_passed(deadline), "the map never settled"
    );
  }
}

mln_acquired_frame mln_test_render_and_acquire(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture->session, &frame, NULL));
  TEST_ASSERT_NOT_EQUAL(MLN_HANDLE_NULL, frame);
  return frame;
}

void mln_test_render_release_frame(mln_acquired_frame* frame) {
  const mln_gpu_sync sync = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_OK(mln_acquired_frame_release(*frame, &sync, NULL));
  *frame = MLN_HANDLE_NULL;
}

mln_status mln_test_render_read_back(
  const mln_test_render_fixture* fixture, mln_texture_image_info* out_info,
  uint8_t* out_pixels, size_t capacity
) {
  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_render_session_read_texture(
    fixture->session, &readback.descriptor, NULL
  ));
  const mln_status status =
    mln_test_render_fixture_finish_operation(fixture, &readback);
  if (status == MLN_STATUS_OK) {
    mln_texture_readback_result result = {0};
    TEST_ASSERT_TRUE(
      mln_test_completion_copy_value(&readback, &result, sizeof(result))
    );
    TEST_ASSERT_EQUAL_size_t(result.info.byte_length, result.data.size);
    *out_info = result.info;
    if (out_pixels != NULL) {
      const size_t size =
        result.data.size < capacity ? result.data.size : capacity;
      memcpy(out_pixels, result.data.data, size);
    }
  }
  mln_test_completion_destroy(&readback);
  return status;
}
