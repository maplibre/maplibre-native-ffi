// Frame demands and their results through the render fixture.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "frames.h"

#include "env.h"
#include "status.h"
#include "unity.h"
#include "wait.h"

void mln_test_render_prepare_map(mln_runtime runtime, mln_map map) {
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
}

void mln_test_render_request_forced(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = token;
  demand.coalescing_boundary = token;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
}

typedef struct results_wait {
  const mln_test_render_fixture* fixture;
  size_t minimum;
  mln_render_frame_batch batch;
  bool failed;
} results_wait;

// Holds once every pending demand has settled and one drain yields at least
// `minimum` results. A drain with fewer is released and the wait goes on.
static bool results_ready(void* context) {
  results_wait* wait = context;
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  if (
    mln_render_session_get_snapshot(wait->fixture->session, &snapshot, NULL) !=
      MLN_STATUS_OK ||
    snapshot.pending_demand_count != 0
  ) {
    return false;
  }
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_status status = mln_render_session_drain_frame_results(
    wait->fixture->session, &batch, NULL
  );
  if (status == MLN_STATUS_OK) {
    mln_render_frame_batch_view view = {
      .size = sizeof(mln_render_frame_batch_view)
    };
    if (
      mln_render_frame_batch_get(batch, &view, NULL) == MLN_STATUS_OK &&
      view.result_count >= wait->minimum
    ) {
      wait->batch = batch;
      return true;
    }
    mln_render_frame_batch_release(batch);
  } else if (status != MLN_STATUS_NOT_READY) {
    wait->failed = true;
    return true;
  }
  return false;
}

mln_render_frame_batch mln_test_render_wait_for_results(
  const mln_test_render_fixture* fixture, size_t minimum
) {
  results_wait wait = {
    .fixture = fixture, .minimum = minimum, .batch = MLN_HANDLE_NULL
  };
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, results_ready, &wait, mln_test_deadline_default(), "frame results"
  ));
  TEST_ASSERT_FALSE(wait.failed);
  return wait.batch;
}

mln_render_frame_batch_view mln_test_render_batch_view(
  mln_render_frame_batch batch
) {
  mln_render_frame_batch_view view = {
    .size = sizeof(mln_render_frame_batch_view)
  };
  MLN_TEST_OK(mln_render_frame_batch_get(batch, &view, NULL));
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(
    sizeof(mln_render_frame_result), view.result_size
  );
  return view;
}

const mln_render_frame_result* mln_test_render_view_result(
  const mln_render_frame_batch_view* view, size_t index
) {
  TEST_ASSERT_LESS_THAN_size_t(view->result_count, index);
  return (const mln_render_frame_result*)((const char*)view->results +
                                          (index * view->result_size));
}

mln_render_frame_result mln_test_render_batch_result(
  mln_render_frame_batch batch, size_t index
) {
  const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
  return *mln_test_render_view_result(&view, index);
}

mln_acquired_frame mln_test_render_and_acquire(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture, 1);
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
  const mln_gpu_sync sync = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(frame, &sync, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, *frame);
}

mln_status mln_test_render_read_back(
  const mln_test_render_fixture* fixture, mln_texture_image_info* out_info,
  uint8_t* out_pixels, size_t capacity
) {
  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_texture_read_premultiplied_rgba8(
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
