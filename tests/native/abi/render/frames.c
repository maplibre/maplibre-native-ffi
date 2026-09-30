// Rendered frames: readback as an ordered operation, acquired frames after
// abandon, and the repaint flag while the map animates.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "support/frames.h"

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void texture_readback_is_an_ordered_owned_operation_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(map, mln_test_red_background_style_json)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 260);

  mln_test_completion readback = mln_test_completion_readback();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_texture_read_premultiplied_rgba8(
                     fixture.session, &readback.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &readback)
  );
  mln_texture_readback_result result = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &result, sizeof(result))
  );
  const mln_buffer_view bytes = result.data;
  const mln_texture_image_info info = result.info;
  TEST_ASSERT_EQUAL_UINT32(64, info.width);
  TEST_ASSERT_EQUAL_UINT32(64, info.height);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(4, (uint32_t)bytes.size);
  TEST_ASSERT_EQUAL_UINT8(255, ((const uint8_t*)bytes.data)[0]);
  TEST_ASSERT_EQUAL_UINT8(0, ((const uint8_t*)bytes.data)[1]);
  TEST_ASSERT_EQUAL_UINT8(0, ((const uint8_t*)bytes.data)[2]);
  TEST_ASSERT_EQUAL_UINT8(255, ((const uint8_t*)bytes.data)[3]);
  mln_test_completion_destroy(&readback);

  const mln_gpu_sync sync = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release(&frame, &sync, NULL)
  );
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Abandon right after the host acquires a frame succeeds even though a core
// worker may still be inside the call that rendered it. The frame then
// reports target loss, and releasing it is CPU-only.
static void acquired_frame_release_after_abandon_is_cpu_only(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 250);

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_abandon(fixture.session, &abandoned, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, abandoned.disposition
  );
  TEST_ASSERT_GREATER_THAN_UINT32(0, abandoned.quarantined_resource_count);
  mln_render_frame_result invalid = {.size = sizeof(mln_render_frame_result)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_TARGET_LOST, mln_acquired_frame_get_result(frame, &invalid, NULL)
  );
  const mln_gpu_sync sync = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release(&frame, &sync, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Renders one forced frame and returns its result, releasing any frame the
// ring holds so it never fills.
static mln_render_frame_result render_one(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  while (mln_render_session_acquire_frame(fixture->session, &frame, NULL) ==
         MLN_STATUS_OK) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_acquired_frame_release(&frame, NULL, NULL)
    );
    frame = MLN_HANDLE_NULL;
  }
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  return result;
}

static void ease_zoom(mln_map map, double zoom) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  // Far longer than any run, so the transition is still going at every frame
  // below until the jump ends it.
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  update.animation.duration_ms = 3600000;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

static void jump_zoom(mln_map map, double zoom) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// A style whose transitions, placement fades included, take no time.
static const char instant_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[],"
  "\"transition\":{\"duration\":0,\"delay\":0}}";

// Renders one forced frame and waits until the map has handled it. The map
// queues the frame-finished event while it handles the frame on the runtime
// worker, so a map command submitted once the event arrives runs after that
// handling, and after any update the handling published. A runtime barrier
// would not do: it waits only for earlier submissions, not for the worker.
static mln_render_frame_result render_and_settle(
  const mln_test_render_fixture* fixture, mln_runtime runtime, mln_map map,
  uint64_t token
) {
  const mln_render_frame_result result = render_one(fixture, token);
  mln_runtime_event event = {0};
  TEST_ASSERT_TRUE(mln_test_await_event(
    runtime, MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED, map, &event, NULL, 0
  ));
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_event_mask(
      map, MLN_RUNTIME_EVENT_MASK_ALL, &completion.descriptor, NULL
    )
  );
  return result;
}

// While a camera transition runs, the map publishes a new update after every
// frame, which is how a render-if-needed host learns to render again. The
// camera alone leaves needs_repaint unset: it reports the renderer's own
// transitions, which this style makes instant. A jump ends the camera
// transition, and the map stops publishing.
static void a_camera_transition_publishes_an_update_after_every_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(instant_style_json))
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  ease_zoom(map, 8.0);
  mln_render_frame_result previous =
    render_and_settle(&fixture, runtime, map, 1);
  for (uint64_t token = 2; token <= 4; token += 1) {
    const mln_render_frame_result result =
      render_and_settle(&fixture, runtime, map, token);
    TEST_ASSERT_GREATER_THAN_UINT64(
      previous.map_update_generation, result.map_update_generation
    );
    TEST_ASSERT_FALSE(result.needs_repaint);
    previous = result;
  }

  jump_zoom(map, 2.0);
  const mln_render_frame_result settled =
    render_and_settle(&fixture, runtime, map, 5);
  TEST_ASSERT_FALSE(settled.needs_repaint);
  TEST_ASSERT_EQUAL_UINT64(
    settled.map_update_generation, render_one(&fixture, 6).map_update_generation
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(texture_readback_is_an_ordered_owned_operation_result);
  RUN_TEST(acquired_frame_release_after_abandon_is_cpu_only);
  RUN_TEST(a_camera_transition_publishes_an_update_after_every_frame);
}
