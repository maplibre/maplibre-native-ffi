// The backend-neutral half of the render fixture: attaching, driving, and
// tearing down an owned texture target, and tracking sessions for reclaim.

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

// Frees the record once it is empty, so a thread that exits holds nothing.
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
  size_t serviced = 0;
  return mln_render_session_service_driver_work(
    fixture->session, SIZE_MAX, &serviced, MLN_TEST_DIAGNOSTIC
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
  if (!mln_test_await(step_ready, &step, deadline, what)) {
    return MLN_STATUS_NOT_READY;
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

bool mln_test_render_fixture_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_create_with(
    map, fixture, mln_test_backend_attach
  );
}

bool mln_test_render_fixture_create_with(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach
) {
  if (map == MLN_HANDLE_NULL || fixture == NULL) {
    return false;
  }
  mln_test_render_reserve_session();
  *fixture = (mln_test_render_fixture){0};
  fixture->driver = mln_test_backend_driver();
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.requested_texture_ring_depth = 2;
  options.driver = fixture->driver;
  options.frame_wake = (mln_wake){
    .size = sizeof(mln_wake),
    .callback = mln_test_render_count_wake,
    .user_data = &fixture->frame_wakes
  };
  options.driver_work_wake = (mln_wake){
    .size = sizeof(mln_wake),
    .callback = mln_test_render_count_wake,
    .user_data = &fixture->driver_wakes
  };
  mln_test_completion completion = mln_test_completion_default(0);
  mln_status status = MLN_STATUS_INVALID_STATE;
  if (!attach(
        map, &options, &fixture->backend_state, &fixture->session,
        &completion.descriptor, &status
      )) {
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
      (void)mln_render_session_destroy(fixture->session, MLN_TEST_DIAGNOSTIC);
    }
    mln_test_backend_destroy(fixture->backend_state);
    *fixture = (mln_test_render_fixture){0};
    return false;
  }
  mln_test_completion_destroy(&completion);
  mln_test_render_track_session(fixture);
  return true;
}

mln_status mln_test_render_fixture_start_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
) {
  mln_test_render_reserve_session();
  *fixture = (mln_test_render_fixture){0};
  fixture->driver = mln_test_backend_driver();
  mln_render_session_attach_options attach = *options;
  attach.driver = fixture->driver;
  mln_status status = MLN_STATUS_INVALID_STATE;
  if (!mln_test_backend_attach(
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

void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture) {
  mln_test_release_drained_batch();
  if (fixture == NULL) {
    return;
  }
  if (fixture->session != MLN_HANDLE_NULL) {
    mln_test_completion detach = mln_test_completion_default(0);
    const mln_status detach_status = mln_render_session_detach(
      fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
    );
    if (detach_status == MLN_STATUS_OK) {
      MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &detach));
    } else {
      detach.descriptor.release_user_data(detach.descriptor.user_data);
      TEST_ASSERT_TRUE(
        detach_status == MLN_STATUS_INVALID_STATE ||
        detach_status == MLN_STATUS_INVALID_ARGUMENT
      );
    }
    mln_test_completion_destroy(&detach);
    if (detach_status != MLN_STATUS_INVALID_ARGUMENT) {
      MLN_TEST_OK(
        mln_render_session_destroy(fixture->session, MLN_TEST_DIAGNOSTIC)
      );
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
      (void)mln_render_session_destroy(entry.session, MLN_TEST_DIAGNOSTIC);
    }
    mln_test_backend_destroy(entry.backend_state);
    reclaimed = true;
  }
  release_empty_session_slots();
  return reclaimed;
}
