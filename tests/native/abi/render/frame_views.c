// Borrowed views of an acquired frame, and disposal of a frame, which a
// binding's finalizer uses in place of a synchronized release.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "maplibre_native_c/callback_adapter.h"
#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static mln_acquired_frame render_and_acquire(
  const mln_test_render_fixture* fixture
) {
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(fixture)
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(
    fixture->session, &frame, MLN_TEST_DIAGNOSTIC
  ));
  return frame;
}

static void attach(
  mln_runtime* runtime, mln_map* map, mln_test_render_fixture* fixture
) {
  *runtime = mln_test_create_runtime();
  *map = mln_test_create_map(*runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(*map, mln_test_empty_style_json));
  MLN_TEST_OK(mln_test_runtime_barrier(*runtime));
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(*map, fixture));
}

static void detach(
  mln_runtime runtime, mln_map map, mln_test_render_fixture* fixture
) {
  mln_test_render_fixture_destroy(fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Each scope holds the frame: an explicit release reports busy, and leaves
// the caller the handle, until every scope has ended.
static void borrowed_views_hold_a_frame_until_every_view_ends(void) {
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_acquired_frame frame = render_and_acquire(&fixture);

  MLN_TEST_INVALID(
    mln_adapter_acquired_frame_view_begin(frame, NULL, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "out_scope must not be null"),
    mln_test_last_error()
  );
  void* scopes[2] = {NULL, NULL};
  for (size_t index = 0; index < 2; index += 1) {
    MLN_TEST_OK(
      mln_adapter_acquired_frame_view_begin(frame, &scopes[index], NULL)
    );
    TEST_ASSERT_NOT_NULL(scopes[index]);
  }
  mln_gpu_sync sync = mln_gpu_sync_default();
  for (size_t index = 0; index < 2; index += 1) {
    const mln_acquired_frame held = frame;
    MLN_TEST_STATUS(
      MLN_STATUS_BUSY, mln_acquired_frame_release(&frame, &sync, NULL)
    );
    TEST_ASSERT_EQUAL_UINT64(held, frame);
    mln_adapter_acquired_frame_view_end(scopes[index]);
  }
  mln_adapter_acquired_frame_view_end(NULL);

  const mln_acquired_frame released = frame;
  MLN_TEST_OK(mln_acquired_frame_release(&frame, &sync, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  void* stale = NULL;
  MLN_TEST_INVALID(
    mln_adapter_acquired_frame_view_begin(released, &stale, NULL)
  );
  TEST_ASSERT_NULL(stale);

  detach(runtime, map, &fixture);
}

static bool session_abandoned(void* context) {
  const mln_test_render_fixture* fixture = context;
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  return mln_render_session_get_snapshot(fixture->session, &snapshot, NULL) ==
           MLN_STATUS_OK &&
         snapshot.state == MLN_RENDER_SESSION_STATE_ABANDONED;
}

// Disposing a frame consumes it and gives up the session's target without
// consumer GPU synchronization. Every later view of the session's frames
// fails, and the session is abandoned once the views already open end. The
// other frames and the session stay owned and are still released.
static void disposing_a_frame_abandons_its_session_after_open_views(void) {
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture);
  mln_acquired_frame kept = render_and_acquire(&fixture);
  const mln_acquired_frame disposed = render_and_acquire(&fixture);
  void* scope = NULL;
  MLN_TEST_OK(mln_adapter_acquired_frame_view_begin(kept, &scope, NULL));

  MLN_TEST_OK(mln_acquired_frame_dispose(disposed, MLN_TEST_DIAGNOSTIC));
  MLN_TEST_INVALID(mln_acquired_frame_dispose(disposed, NULL));
  void* rejected = NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_TARGET_LOST,
    mln_adapter_acquired_frame_view_begin(kept, &rejected, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "no longer owns"), mln_test_last_error()
  );
  TEST_ASSERT_NULL(rejected);
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_STATUS(
    MLN_STATUS_TARGET_LOST, mln_acquired_frame_get_result(kept, &result, NULL)
  );

  mln_adapter_acquired_frame_view_end(scope);
  TEST_ASSERT_TRUE(mln_test_await(
    session_abandoned, &fixture, mln_test_deadline_default(),
    "the disposed frame's session to be abandoned"
  ));
  mln_gpu_sync sync = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(&kept, &sync, NULL));

  detach(runtime, map, &fixture);
}

MLN_TEST_GROUP {
  RUN_TEST(borrowed_views_hold_a_frame_until_every_view_ends);
  RUN_TEST(disposing_a_frame_abandons_its_session_after_open_views);
}
