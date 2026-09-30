// A render session's driver and lifecycle: what attach reports, which thread
// may service a caller driver, one session per map, maintenance commands in
// frame order, detach, abandon, and disposal.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "support/frames.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static mln_render_session_snapshot read_snapshot(mln_render_session session) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(session, &snapshot, NULL)
  );
  return snapshot;
}

static void attach_reports_the_selected_native_driver(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(fixture.driver, capabilities.driver);
  TEST_ASSERT_EQUAL_UINT32(2, capabilities.texture_ring_depth);
  TEST_ASSERT_BITS_HIGH(
    MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
      MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC,
    capabilities.flags
  );
  // A caller driver attaches only when the host services it, so the session
  // is still attaching after submission and has published driver work.
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    TEST_ASSERT_TRUE(fixture.observed_attaching);
    TEST_ASSERT_TRUE(fixture.observed_driver_ready);
  }
  const mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);
  TEST_ASSERT_EQUAL_UINT32(capabilities.driver, snapshot.driver);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct foreign_driver_probe {
  mln_render_session session;
  atomic_bool done;
  mln_status status;
} foreign_driver_probe;

static void service_from_foreign_thread(void* argument) {
  foreign_driver_probe* probe = argument;
  size_t serviced = 0;
  probe->status =
    mln_render_session_service_driver_work(probe->session, 1, &serviced, NULL);
  mln_test_flag_set(&probe->done);
}

// The fixture services a caller driver from the case's thread, which fixes
// that thread as the session's graphics thread. A core worker drives itself.
static void driver_service_fixes_and_enforces_graphics_thread_identity(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  foreign_driver_probe probe = {.session = fixture.session};
  atomic_init(&probe.done, false);
  mln_test_thread* thread =
    mln_test_thread_start(service_from_foreign_thread, &probe);
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &probe.done));
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_INT(
    fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      ? MLN_STATUS_WRONG_THREAD
      : MLN_STATUS_INVALID_STATE,
    probe.status
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A map takes a second session only once the first has let it go.
static void a_map_holds_one_session_at_a_time(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture first = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &first));

  mln_test_render_fixture second = {0};
  TEST_ASSERT_FALSE(mln_test_render_fixture_create(map, &second));
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_last_error(), "map already has an attached render session")
  );

  mln_test_render_fixture_destroy(&first);
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &second));
  mln_test_render_fixture_destroy(&second);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef mln_status (*maintenance_submit)(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
);

typedef struct maintenance_case {
  const char* label;
  maintenance_submit submit;
} maintenance_case;

static const maintenance_case maintenance_cases[] = {
  {"reduce memory use", mln_render_session_reduce_memory_use},
  {"clear data", mln_render_session_clear_data},
  {"dump debug logs", mln_render_session_dump_debug_logs},
};

// What the frame-result queue held when a maintenance command completed. The
// completion runs on the driver, between the work items around it.
typedef struct maintenance_probe {
  mln_render_session session;
  atomic_bool completed;
  mln_status status;
  bool drain_failed;
  size_t token_count;
  uint64_t tokens[4];
} maintenance_probe;

static void drain_tokens(maintenance_probe* probe) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_status drained =
    mln_render_session_drain_frame_results(probe->session, &batch, NULL);
  if (drained == MLN_STATUS_NOT_READY) {
    return;
  }
  size_t count = 0;
  if (
    drained != MLN_STATUS_OK ||
    mln_render_frame_batch_count(batch, &count, NULL) != MLN_STATUS_OK
  ) {
    probe->drain_failed = true;
    mln_render_frame_batch_release(batch);
    return;
  }
  for (size_t index = 0; index < count; index += 1) {
    mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
    if (
      probe->token_count == sizeof(probe->tokens) / sizeof(probe->tokens[0]) ||
      mln_render_frame_batch_get(batch, index, &result, NULL) != MLN_STATUS_OK
    ) {
      probe->drain_failed = true;
      break;
    }
    probe->tokens[probe->token_count] = result.token;
    probe->token_count += 1;
  }
  mln_render_frame_batch_release(batch);
}

static void maintenance_completed(
  void* user_data, const mln_completion_result* result
) {
  maintenance_probe* probe = user_data;
  probe->status = result->status;
  drain_tokens(probe);
  mln_test_flag_set(&probe->completed);
}

static void maintenance_released(void* user_data) { (void)user_data; }

typedef struct probe_wait {
  const mln_test_render_fixture* fixture;
  maintenance_probe* probe;
} probe_wait;

// Holds once the command completed and the later demand's result arrived.
static bool maintenance_settled(void* context) {
  probe_wait* wait = context;
  if (!atomic_load(&wait->probe->completed)) {
    return false;
  }
  drain_tokens(wait->probe);
  return wait->probe->drain_failed || wait->probe->token_count >= 2;
}

// Each maintenance command runs on the driver in acceptance order with frame
// demands: it completes after the demand accepted before it has a result and
// before the demand accepted after it has one.
static void maintenance_commands_run_in_order_with_frames(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  for (size_t row = 0;
       row < sizeof(maintenance_cases) / sizeof(maintenance_cases[0]);
       row += 1) {
    const maintenance_case* command = &maintenance_cases[row];
    maintenance_probe probe = {.session = fixture.session};
    atomic_init(&probe.completed, false);
    const mln_completion completion = {
      .size = sizeof(mln_completion),
      .callback = maintenance_completed,
      .user_data = &probe,
      .release_user_data = maintenance_released,
    };
    mln_test_render_request_forced(&fixture, 10 * row + 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, command->submit(fixture.session, &completion, NULL),
      command->label
    );
    mln_test_render_request_forced(&fixture, 10 * row + 2);
    probe_wait wait = {.fixture = &fixture, .probe = &probe};
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK,
      mln_test_render_step_until(
        &fixture, maintenance_settled, &wait, mln_test_deadline_default(),
        "a maintenance command between two frames"
      ),
      command->label
    );
    TEST_ASSERT_FALSE_MESSAGE(probe.drain_failed, command->label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(MLN_STATUS_OK, probe.status, command->label);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(2, probe.token_count, command->label);
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(
      10 * row + 1, probe.tokens[0], command->label
    );
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(
      10 * row + 2, probe.tokens[1], command->label
    );
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void normal_detach_runs_on_the_driver_and_retires_map_attachment(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_test_completion detach = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_detach(fixture.session, &detach.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &detach)
  );
  mln_test_completion_destroy(&detach);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_DETACHED, read_snapshot(fixture.session).state
  );

  // The map's session slot is free again before the handle is destroyed.
  mln_test_render_fixture other = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &other));
  mln_test_render_fixture_destroy(&other);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct detached_call {
  const char* label;
  mln_status (*call)(mln_render_session session);
} detached_call;

static mln_status detached_request_frame(mln_render_session session) {
  const mln_frame_demand demand = mln_frame_demand_default();
  return mln_render_session_request_frame(session, &demand, NULL);
}

static mln_status detached_barrier(mln_render_session session) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_render_session_barrier(session, &completion, NULL);
}

static mln_status detached_resize(mln_render_session session) {
  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 32,
    .height = 32,
    .scale_factor = 1.0,
  };
  const mln_completion completion = mln_test_discard_completion();
  return mln_render_session_resize(session, &extent, &completion, NULL);
}

static mln_status detached_reduce_memory_use(mln_render_session session) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_render_session_reduce_memory_use(session, &completion, NULL);
}

static mln_status detached_clear_data(mln_render_session session) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_render_session_clear_data(session, &completion, NULL);
}

static mln_status detached_dump_debug_logs(mln_render_session session) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_render_session_dump_debug_logs(session, &completion, NULL);
}

static mln_status detached_acquire_frame(mln_render_session session) {
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  return mln_render_session_acquire_frame(session, &frame, NULL);
}

static mln_status detached_readback(mln_render_session session) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_texture_read_premultiplied_rgba8(session, &completion, NULL);
}

static mln_status detached_detach(mln_render_session session) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_render_session_detach(session, &completion, NULL);
}

static mln_status detached_abandon(mln_render_session session) {
  mln_render_abandon_result result = {
    .size = sizeof(mln_render_abandon_result)
  };
  return mln_render_session_abandon(session, &result, NULL);
}

static const detached_call detached_calls[] = {
  {"request frame", detached_request_frame},
  {"barrier", detached_barrier},
  {"resize", detached_resize},
  {"reduce memory use", detached_reduce_memory_use},
  {"clear data", detached_clear_data},
  {"dump debug logs", detached_dump_debug_logs},
  {"acquire frame", detached_acquire_frame},
  {"texture readback", detached_readback},
  {"detach", detached_detach},
  {"abandon", detached_abandon},
};

// Once detached, a session accepts no further work and reports its state,
// and it stays detached for destroy.
static void a_detached_session_rejects_every_call_that_needs_its_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_completion detach = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_detach(fixture.session, &detach.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &detach)
  );
  mln_test_completion_destroy(&detach);

  for (size_t row = 0; row < sizeof(detached_calls) / sizeof(detached_calls[0]);
       row += 1) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_STATE, detached_calls[row].call(fixture.session),
      detached_calls[row].label
    );
  }
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_DETACHED, read_snapshot(fixture.session).state
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void stale_and_null_sessions_reject_maintenance_commands(void) {
  mln_completion operation = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_reduce_memory_use(MLN_HANDLE_NULL, &operation, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_clear_data(MLN_HANDLE_NULL, &operation, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_dump_debug_logs(MLN_HANDLE_NULL, &operation, NULL)
  );

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const mln_render_session stale = fixture.session;
  mln_test_render_fixture_destroy(&fixture);
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_get_snapshot(stale, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_reduce_memory_use(stale, &operation, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_destroy(stale, NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A session abandoned right after attach has nothing in flight to wait for,
// and a caller driver's queued command completes as target lost.
static void abandon_completes_pending_work_and_invalidates_accessors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  // A caller driver runs nothing until the case services it, so the command
  // is still queued at abandon. A core worker may already have run it, and
  // with no frame rendered yet it has no renderer to act on.
  mln_test_completion pending = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_reduce_memory_use(
                     fixture.session, &pending.descriptor, NULL
                   )
  );
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_abandon(fixture.session, &abandoned, NULL)
  );
  TEST_ASSERT_TRUE(
    abandoned.disposition == MLN_RENDER_ABANDON_DISPOSITION_CLEAN ||
    abandoned.disposition == MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED
  );
  // Abandon settles every accepted command before it returns.
  TEST_ASSERT_TRUE(mln_test_completion_poll(&pending));
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_TARGET_LOST, mln_test_completion_status(&pending)
    );
  }
  mln_test_completion_destroy(&pending);

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_ABANDONED, read_snapshot(fixture.session).state
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_acquire_frame(fixture.session, &frame, NULL)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// What abandon returned from inside the completion of a command that the
// driver runs.
typedef struct reentrant_abandon_probe {
  mln_render_session session;
  atomic_bool completed;
  mln_status completion_status;
  mln_status abandon_status;
} reentrant_abandon_probe;

static void abandon_from_completion(
  void* user_data, const mln_completion_result* result
) {
  reentrant_abandon_probe* probe = user_data;
  probe->completion_status = result->status;
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  probe->abandon_status =
    mln_render_session_abandon(probe->session, &abandoned, NULL);
  mln_test_flag_set(&probe->completed);
}

static bool reentrant_abandon_settled(void* context) {
  const reentrant_abandon_probe* probe = context;
  return atomic_load(&probe->completed);
}

static const maintenance_case driver_run_commands[] = {
  {"barrier", mln_render_session_barrier},
  {"reduce memory use", mln_render_session_reduce_memory_use},
};

// A completion that the driver delivers runs inside the session's driver
// call: on the core worker, or on the thread servicing a caller driver.
// Abandon from there cannot wait for the call to end, so it is busy and the
// session stays attached.
static void abandon_from_a_driver_completion_is_busy(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  // A rendered frame gives the maintenance command a renderer to act on.
  mln_test_render_request_forced(&fixture, 1);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&fixture, 1));

  for (size_t row = 0;
       row < sizeof(driver_run_commands) / sizeof(driver_run_commands[0]);
       row += 1) {
    const maintenance_case* command = &driver_run_commands[row];
    reentrant_abandon_probe probe = {.session = fixture.session};
    atomic_init(&probe.completed, false);
    const mln_completion completion = {
      .size = sizeof(mln_completion),
      .callback = abandon_from_completion,
      .user_data = &probe,
      .release_user_data = maintenance_released,
    };
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, command->submit(fixture.session, &completion, NULL),
      command->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK,
      mln_test_render_step_until(
        &fixture, reentrant_abandon_settled, &probe,
        mln_test_deadline_default(), "a command whose completion abandons"
      ),
      command->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, probe.completion_status, command->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_BUSY, probe.abandon_status, command->label
    );
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      MLN_RENDER_SESSION_STATE_ATTACHED, read_snapshot(fixture.session).state,
      command->label
    );
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void count_nothing(void* context) { (void)context; }

static void flag_release(void* context) { mln_test_flag_set(context); }

// Disposing a session before its attach completes still gives the attach one
// terminal result, retires the session, and frees the map's session slot. A
// caller driver has not attached yet, because the case never serviced it; a
// core worker may have.
static void a_session_disposed_while_attaching_frees_the_map(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  atomic_bool released;
  atomic_init(&released, false);
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.requested_texture_ring_depth = 2;
  options.frame_wake = (mln_wake){
    .size = sizeof(mln_wake),
    .callback = count_nothing,
    .user_data = &released,
    .release_user_data = flag_release,
  };
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_start_attach(
                     map, &options, &attach.descriptor, &fixture
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_dispose(fixture.session, NULL)
  );

  TEST_ASSERT_TRUE(mln_test_completion_wait(&attach, -1));
  const mln_status attach_status = mln_test_completion_status(&attach);
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_TARGET_LOST, attach_status);
  } else {
    TEST_ASSERT_TRUE(
      attach_status == MLN_STATUS_OK || attach_status == MLN_STATUS_TARGET_LOST
    );
  }
  mln_test_completion_destroy(&attach);
  // The wake's release runs once the session is retired.
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&released));

  mln_test_render_fixture other = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &other));
  mln_test_render_fixture_destroy(&other);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void retirement_wake(void* context) { (void)context; }
static void retirement_release(void* context) {
  atomic_store((atomic_bool*)context, true);
}

static void parent_first_disposal_retires_a_native_render_attachment(void) {
  atomic_bool retired = false;
  mln_runtime_options options = mln_runtime_options_default();
  options.event_wake.callback = retirement_wake;
  options.event_wake.user_data = &retired;
  options.event_wake.release_user_data = retirement_release;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create(&options, &runtime, NULL)
  );
  mln_map map = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_create_status(runtime, NULL, &map)
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_dispose(runtime, NULL));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_dispose(map, NULL));
  TEST_ASSERT_FALSE(atomic_load(&retired));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_dispose(fixture.session, NULL)
  );
  mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&retired));
  mln_test_render_fixture_destroy(&fixture);
}

MLN_TEST_GROUP {
  RUN_TEST(attach_reports_the_selected_native_driver);
  RUN_TEST(driver_service_fixes_and_enforces_graphics_thread_identity);
  RUN_TEST(a_map_holds_one_session_at_a_time);
  RUN_TEST(maintenance_commands_run_in_order_with_frames);
  RUN_TEST(normal_detach_runs_on_the_driver_and_retires_map_attachment);
  RUN_TEST(a_detached_session_rejects_every_call_that_needs_its_target);
  RUN_TEST(stale_and_null_sessions_reject_maintenance_commands);
  RUN_TEST(abandon_completes_pending_work_and_invalidates_accessors);
  RUN_TEST(abandon_from_a_driver_completion_is_busy);
  RUN_TEST(a_session_disposed_while_attaching_frees_the_map);
  RUN_TEST(parent_first_disposal_retires_a_native_render_attachment);
}
