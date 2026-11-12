#include <stdint.h>
#include <stdlib.h>

#include "render.h"

#include "env.h"
#include "render_backend.h"
#include "status.h"
#include "unity.h"
#include "wait.h"

// Tracked by value: the caller's fixture usually lives on a test stack frame an
// aborting assertion unwinds before teardown runs. Tracking is thread local so
// one thread's teardown leaves another thread's sessions alone.
typedef struct tracked_session {
  mln_render_session session;
  void* backend_state;
} tracked_session;

static MLN_TEST_THREAD_LOCAL tracked_session* tracked_sessions;
static MLN_TEST_THREAD_LOCAL size_t tracked_session_count;
static MLN_TEST_THREAD_LOCAL size_t tracked_session_capacity;

// Grows before the caller attaches: failing after would strand the session.
void mln_test_render_reserve_session(void) {
  if (tracked_session_count < tracked_session_capacity) {
    return;
  }
  const size_t capacity =
    tracked_session_capacity == 0 ? 8 : tracked_session_capacity * 2;
  tracked_session* grown =
    realloc(tracked_sessions, capacity * sizeof(*tracked_sessions));
  TEST_ASSERT_NOT_NULL_MESSAGE(grown, "tracking a render session failed");
  tracked_sessions = grown;
  tracked_session_capacity = capacity;
}

static void release_empty_session_slots(void) {
  if (tracked_session_count == 0) {
    free(tracked_sessions);
    tracked_sessions = NULL;
    tracked_session_capacity = 0;
  }
}

void mln_test_render_track_session(const mln_test_render_fixture* fixture) {
  tracked_sessions[tracked_session_count] = (tracked_session){
    .session = fixture->session,
    .backend_state = fixture->backend_state,
  };
  tracked_session_count += 1;
}

static void untrack_session(mln_render_session session) {
  for (size_t index = 0; index < tracked_session_count; index += 1) {
    if (tracked_sessions[index].session == session) {
      tracked_sessions[index] = tracked_sessions[tracked_session_count - 1];
      tracked_session_count -= 1;
      release_empty_session_slots();
      return;
    }
  }
}

void mln_test_render_count_wake(void* user_data) {
  atomic_fetch_add((atomic_uint*)user_data, 1U);
  mln_test_pulse();
}

mln_status mln_test_render_fixture_service(
  const mln_test_render_fixture* fixture
) {
  if (fixture->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    return MLN_STATUS_OK;
  }
  return mln_render_session_service_driver_work(
    fixture->session, MLN_TEST_DIAGNOSTIC
  );
}

typedef struct step_state {
  const mln_test_render_fixture* fixture;
  bool (*ready)(void* context);
  void* context;
  mln_status service_status;
} step_state;

static bool step_ready(void* context) {
  step_state* step = context;
  step->service_status = mln_test_render_fixture_service(step->fixture);
  return step->service_status != MLN_STATUS_OK || step->ready(step->context);
}

mln_status mln_test_render_step_until(
  const mln_test_render_fixture* fixture, bool (*ready)(void* context),
  void* context, mln_test_deadline deadline, const char* what
) {
  step_state step = {
    .fixture = fixture,
    .ready = ready,
    .context = context,
    .service_status = MLN_STATUS_OK,
  };
  if (!mln_test_await_on_event_loop(step_ready, &step, deadline, what)) {
    return MLN_TEST_RENDER_TIMED_OUT;
  }
  return step.service_status;
}

static bool completion_delivered(void* context) {
  return mln_test_completion_poll(context);
}

mln_status mln_test_render_fixture_finish_operation(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
) {
  if (
    fixture == NULL || fixture->session == MLN_HANDLE_NULL || completion == NULL
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const mln_status status = mln_test_render_step_until(
    fixture, completion_delivered, completion, mln_test_deadline_default(),
    "a render session operation"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  return mln_test_completion_status(completion);
}

mln_logical_extent mln_test_session_extent(
  mln_render_session session, mln_logical_size size
) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(session, &snapshot, NULL));
  return (mln_logical_extent){
    .width = size.width,
    .height = size.height,
    .scale_factor = snapshot.extent.scale_factor,
  };
}

mln_status mln_test_render_session_failure(
  mln_render_session session, char* message, size_t capacity
) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(session, &snapshot, NULL));
  size_t length = 0;
  MLN_TEST_OK(mln_render_session_get_failure(
    session, message, capacity, &length, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_TRUE_MESSAGE(
    capacity == 0 || length < capacity, "failure message did not fit"
  );
  return snapshot.failure_status;
}

typedef struct disposal_state {
  mln_render_session session;
  bool service;
  mln_test_completion* completion;
} disposal_state;

// A disposed caller-driven session detaches at its next service, and its
// handle goes stale once it retires, which ends the servicing.
static bool disposal_settled(void* context) {
  disposal_state* state = context;
  if (
    state->service &&
    mln_render_session_service_driver_work(state->session, NULL) !=
      MLN_STATUS_OK
  ) {
    state->service = false;
  }
  return mln_test_completion_poll(state->completion);
}

static mln_status finish_disposal(
  mln_render_session session, bool caller_driven,
  mln_test_completion* completion
) {
  disposal_state state = {
    .session = session,
    .service = caller_driven,
    .completion = completion,
  };
  if (!mln_test_await_on_event_loop(
        disposal_settled, &state, mln_test_deadline_default(),
        "a render session disposal"
      )) {
    return MLN_TEST_RENDER_TIMED_OUT;
  }
  return mln_test_completion_settle(completion);
}

mln_status mln_test_render_fixture_finish_disposal(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
) {
  return finish_disposal(
    fixture->session,
    fixture->driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, completion
  );
}

mln_status mln_test_render_session_release(mln_render_session session) {
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  const bool caller_driven =
    mln_render_session_get_capabilities(session, &capabilities, NULL) ==
      MLN_STATUS_OK &&
    capabilities.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_render_session_release(
    session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) {
    completion.descriptor.release_user_data(completion.descriptor.user_data);
    mln_test_completion_destroy(&completion);
    return status;
  }
  return finish_disposal(session, caller_driven, &completion);
}

// Set only while create_fixture() attaches a ring of another depth.
static MLN_TEST_THREAD_LOCAL size_t attach_ring_depth = 0;

size_t mln_test_render_ring_depth(void) {
  return attach_ring_depth != 0 ? attach_ring_depth : 2;
}

// An offscreen session creates its own device, so the fixture holds no state.
static bool attach_offscreen(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_OFFSCREEN,
  };
  target.data.offscreen.logical_size =
    (mln_logical_size){.width = 64, .height = 64};
  *out_state = NULL;
  *out_status = mln_map_attach(
    map, &target, MLN_HANDLE_NULL, options, out_session, completion,
    MLN_TEST_DIAGNOSTIC
  );
  return true;
}

// Attaches with `attach` on `driver`. A borrowed attach makes a ring of
// `ring_depth` textures, where zero takes two.
static bool create_fixture(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach, uint32_t driver, size_t ring_depth
) {
  if (map == MLN_HANDLE_NULL || fixture == NULL) {
    return false;
  }
  mln_test_render_reserve_session();
  *fixture = (mln_test_render_fixture){0};
  fixture->driver = driver;
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = fixture->driver;
  options.frame_wake = (mln_wake){
    .callback = mln_test_render_count_wake, .user_data = &fixture->frame_wakes
  };
  options.driver_work_wake = (mln_wake){
    .callback = mln_test_render_count_wake, .user_data = &fixture->driver_wakes
  };
  mln_test_completion completion = mln_test_completion_default(0);
  mln_status status = MLN_STATUS_INVALID_STATE;
  attach_ring_depth = ring_depth;
  const bool submitted = attach(
    map, &options, &fixture->backend_state, &fixture->session,
    &completion.descriptor, &status
  );
  attach_ring_depth = 0;
  if (!submitted) {
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
    *fixture = (mln_test_render_fixture){0};
    return false;
  }
  if (status == MLN_STATUS_OK && fixture->session != MLN_HANDLE_NULL) {
    mln_render_session_snapshot attaching = {
      .size = sizeof(mln_render_session_snapshot)
    };
    fixture->observed_attaching =
      mln_render_session_get_snapshot(
        fixture->session, &attaching, MLN_TEST_DIAGNOSTIC
      ) == MLN_STATUS_OK &&
      attaching.state == MLN_RENDER_SESSION_STATE_ATTACHING;
    if (fixture->driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
      fixture->observed_driver_ready = atomic_load(&fixture->driver_wakes) != 0;
    }
  }
  const mln_status finish_status =
    status == MLN_STATUS_OK && fixture->session != MLN_HANDLE_NULL
      ? mln_test_render_fixture_finish_operation(fixture, &completion)
      : MLN_STATUS_INVALID_STATE;
  if (
    status != MLN_STATUS_OK || fixture->session == MLN_HANDLE_NULL ||
    finish_status != MLN_STATUS_OK
  ) {
    if (status != MLN_STATUS_OK) {
      completion.descriptor.release_user_data(completion.descriptor.user_data);
    }
    mln_test_completion_destroy(&completion);
    if (fixture->session != MLN_HANDLE_NULL) {
      mln_render_abandon_result abandoned = {
        .size = sizeof(mln_render_abandon_result)
      };
      (void)mln_render_session_abandon(
        fixture->session, &abandoned, MLN_TEST_DIAGNOSTIC
      );
      (void)mln_test_render_session_release(fixture->session);
    }
    mln_test_backend_destroy(fixture->backend_state);
    *fixture = (mln_test_render_fixture){0};
    return false;
  }
  mln_test_completion_destroy(&completion);
  mln_test_render_track_session(fixture);
  return true;
}

bool mln_test_render_fixture_create_with(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach
) {
  return create_fixture(map, fixture, attach, mln_test_backend_driver(), 0);
}

bool mln_test_render_fixture_create_on(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach, uint32_t driver
) {
  return create_fixture(map, fixture, attach, driver, 0);
}

bool mln_test_render_fixture_create_offscreen(
  mln_map map, mln_test_render_fixture* fixture
) {
  return create_fixture(
    map, fixture, attach_offscreen, MLN_RENDER_DRIVER_CORE_WORKER, 0
  );
}

bool mln_test_render_fixture_create_borrowed(
  mln_map map, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_create_with(
    map, fixture, mln_test_backend_attach_borrowed
  );
}

bool mln_test_render_fixture_create_borrowed_ring(
  mln_map map, mln_test_render_fixture* fixture, size_t depth, uint32_t driver
) {
  if (depth == 0 || depth > MLN_TEXTURE_RING_MAX_DEPTH) {
    return false;
  }
  return create_fixture(
    map, fixture, mln_test_backend_attach_borrowed,
    driver == MLN_TEST_PRESET_DRIVER ? mln_test_backend_driver() : driver, depth
  );
}

mln_status mln_test_render_fixture_start_attach_with(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach_target, uint32_t driver
) {
  mln_test_render_reserve_session();
  *fixture = (mln_test_render_fixture){0};
  fixture->driver = driver;
  mln_render_session_attach_options attach = *options;
  attach.driver = fixture->driver;
  mln_status status = MLN_STATUS_INVALID_STATE;
  if (!attach_target(
        map, &attach, &fixture->backend_state, &fixture->session, completion,
        &status
      )) {
    *fixture = (mln_test_render_fixture){0};
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (status != MLN_STATUS_OK) {
    mln_test_backend_destroy(fixture->backend_state);
    *fixture = (mln_test_render_fixture){0};
    return status;
  }
  mln_test_render_track_session(fixture);
  return MLN_STATUS_OK;
}

mln_status mln_test_render_fixture_start_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_start_attach_with(
    map, options, completion, fixture, mln_test_backend_attach_borrowed,
    mln_test_backend_driver()
  );
}

mln_status mln_test_render_fixture_start_offscreen_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_start_attach_with(
    map, options, completion, fixture, attach_offscreen,
    MLN_RENDER_DRIVER_CORE_WORKER
  );
}

void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture) {
  if (fixture == NULL) {
    return;
  }
  if (fixture->session != MLN_HANDLE_NULL) {
    // A session the case already disposed or destroyed is stale and needs no
    // teardown.
    mln_render_session_snapshot snapshot = {
      .size = sizeof(mln_render_session_snapshot)
    };
    const mln_status liveness = mln_render_session_get_snapshot(
      fixture->session, &snapshot, MLN_TEST_DIAGNOSTIC
    );
    if (liveness == MLN_STATUS_OK) {
      mln_test_completion detach = mln_test_completion_default(0);
      const mln_status detach_status = mln_render_session_detach(
        fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
      );
      if (detach_status == MLN_STATUS_OK) {
        MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &detach));
      } else {
        detach.descriptor.release_user_data(detach.descriptor.user_data);
        MLN_TEST_STATUS(MLN_STATUS_INVALID_STATE, detach_status);
      }
      mln_test_completion_destroy(&detach);
      MLN_TEST_OK(mln_test_render_session_release(fixture->session));
    } else {
      MLN_TEST_STATUS(MLN_STATUS_INVALID_STATE, liveness);
    }
  }
  untrack_session(fixture->session);
  mln_test_backend_destroy(fixture->backend_state);
  *fixture = (mln_test_render_fixture){0};
}

bool mln_test_render_reclaim_thread_sessions(void) {
  bool reclaimed = false;
  while (tracked_session_count > 0) {
    tracked_session_count -= 1;
    const tracked_session entry = tracked_sessions[tracked_session_count];
    if (entry.session != MLN_HANDLE_NULL) {
      mln_render_abandon_result abandoned = {
        .size = sizeof(mln_render_abandon_result)
      };
      (void)mln_render_session_abandon(
        entry.session, &abandoned, MLN_TEST_DIAGNOSTIC
      );
      (void)mln_test_render_session_release(entry.session);
    }
    mln_test_backend_destroy(entry.backend_state);
    reclaimed = true;
  }
  release_empty_session_slots();
  return reclaimed;
}
