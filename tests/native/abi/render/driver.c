// A render session's driver and lifecycle: what attach reports, which thread
// may service a caller driver, one session per map, maintenance commands in
// frame order, detach, abandon, and disposal.

#include "support/frames.h"
#include "support/test_support.h"

static mln_render_session_snapshot read_snapshot(mln_render_session session) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(session, &snapshot, NULL));
  return snapshot;
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

// Attach reports the driver the preset selects. The fixture services a caller
// driver from the case's thread, which fixes that thread as the session's
// graphics thread; a core worker drives itself. While attached, the session
// holds its map, which takes no second session and refuses its release, and
// refuses destroy, which ends only a detached or abandoned session.
static void an_attached_session_holds_its_map_and_refuses_destroy(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
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
  const bool caller =
    fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  if (caller) {
    TEST_ASSERT_TRUE(fixture.observed_attaching);
    TEST_ASSERT_TRUE(fixture.observed_driver_ready);
  }
  mln_render_session_snapshot snapshot = read_snapshot(fixture.session);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);
  TEST_ASSERT_EQUAL_UINT32(capabilities.driver, snapshot.driver);

  foreign_driver_probe probe = {.session = fixture.session};
  atomic_init(&probe.done, false);
  mln_test_thread* thread =
    mln_test_thread_start(service_from_foreign_thread, &probe);
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &probe.done));
  mln_test_thread_join(thread);
  MLN_TEST_STATUS(
    caller ? MLN_STATUS_WRONG_THREAD : MLN_STATUS_INVALID_STATE, probe.status
  );

  mln_test_render_fixture second = {0};
  TEST_ASSERT_FALSE(mln_test_render_fixture_create(map, &second));
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_last_error(), "map already has an attached render session")
  );
  mln_completion discard = mln_test_discard_completion();
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_map_release(map, &discard, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_STRING(
    "map still has an attached render session", mln_test_last_error()
  );
  MLN_TEST_OK(mln_test_map_request_repaint(map));

  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_destroy(fixture.session, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(strstr(
    mln_test_last_error(),
    "must be detached or abandoned before it is destroyed"
  ));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_ATTACHED, read_snapshot(fixture.session).state
  );
  mln_test_render_request_forced(&fixture, 1);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture, 1);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED,
    mln_test_render_batch_result(batch, 0).disposition
  );
  mln_render_frame_batch_release(batch);

  // Once the session is gone, the map takes another.
  mln_test_render_fixture_destroy(&fixture);
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &second));
  mln_test_render_fixture_destroy(&second);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Detach runs on the driver and frees the map's session slot before the
// handle is destroyed. A detached session accepts no further work and stays
// detached for destroy, and a destroyed or null handle names no session.
static void a_detached_session_frees_its_map_and_refuses_work(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_DETACHED, read_snapshot(fixture.session).state
  );
  mln_test_render_fixture other = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &other));
  mln_test_render_fixture_destroy(&other);

  const mln_render_session detached = fixture.session;
  const mln_completion discard = mln_test_discard_completion();
  const mln_frame_demand demand = mln_frame_demand_default();
  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 32,
    .height = 32,
    .scale_factor = 1.0,
  };
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_request_frame(detached, &demand, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_barrier(detached, &discard, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_resize(detached, &extent, &discard, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_reduce_memory_use(detached, &discard, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_clear_data(detached, &discard, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_dump_debug_logs(detached, &discard, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_acquire_frame(detached, &frame, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_read_texture(detached, &discard, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_detach(detached, &discard, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_abandon(detached, &abandoned, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_DETACHED, read_snapshot(detached).state
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_INVALID_STATE(
    mln_render_session_get_snapshot(detached, &snapshot, NULL)
  );
  MLN_TEST_INVALID_STATE(
    mln_render_session_reduce_memory_use(detached, &discard, NULL)
  );
  MLN_TEST_INVALID_STATE(mln_render_session_destroy(detached, NULL));
  MLN_TEST_INVALID(
    mln_render_session_reduce_memory_use(MLN_HANDLE_NULL, &discard, NULL)
  );
  MLN_TEST_INVALID(
    mln_render_session_clear_data(MLN_HANDLE_NULL, &discard, NULL)
  );
  MLN_TEST_INVALID(
    mln_render_session_dump_debug_logs(MLN_HANDLE_NULL, &discard, NULL)
  );
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
    MLN_TEST_OK_MESSAGE(
      command->submit(fixture.session, &completion, NULL), command->label
    );
    mln_test_render_request_forced(&fixture, 10 * row + 2);
    probe_wait wait = {.fixture = &fixture, .probe = &probe};
    MLN_TEST_OK_MESSAGE(
      mln_test_render_step_until(
        &fixture, maintenance_settled, &wait, mln_test_deadline_default(),
        "a maintenance command between two frames"
      ),
      command->label
    );
    TEST_ASSERT_FALSE_MESSAGE(probe.drain_failed, command->label);
    MLN_TEST_OK_MESSAGE(probe.status, command->label);
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
  MLN_TEST_OK(mln_render_session_reduce_memory_use(
    fixture.session, &pending.descriptor, NULL
  ));
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(fixture.session, &abandoned, NULL));
  TEST_ASSERT_TRUE(
    abandoned.disposition == MLN_RENDER_ABANDON_DISPOSITION_CLEAN ||
    abandoned.disposition == MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED
  );
  // Abandon settles every accepted command before it returns.
  TEST_ASSERT_TRUE(mln_test_completion_poll(&pending));
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    MLN_TEST_STATUS(
      MLN_STATUS_TARGET_LOST, mln_test_completion_status(&pending)
    );
  }
  mln_test_completion_destroy(&pending);

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_ABANDONED, read_snapshot(fixture.session).state
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
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
    MLN_TEST_OK_MESSAGE(
      command->submit(fixture.session, &completion, NULL), command->label
    );
    MLN_TEST_OK_MESSAGE(
      mln_test_render_step_until(
        &fixture, reentrant_abandon_settled, &probe,
        mln_test_deadline_default(), "a command whose completion abandons"
      ),
      command->label
    );
    MLN_TEST_OK_MESSAGE(probe.completion_status, command->label);
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

// What abandon returned from inside an attach completion, which can run
// before the attach call returns the session's handle.
typedef struct attach_abandon_probe {
  atomic_bool published;
  mln_render_session session;
  atomic_bool completed;
  bool saw_session;
  mln_status completion_status;
  mln_status abandon_status;
} attach_abandon_probe;

static void abandon_once_attach_returns(
  void* user_data, const mln_completion_result* result
) {
  attach_abandon_probe* probe = user_data;
  probe->completion_status = result->status;
  // The driver delivers the attach, so waiting here holds only the driver
  // until the case publishes the handle. A delivery on the attaching thread,
  // inside the attach call, would wait out the deadline.
  probe->saw_session = mln_test_wait_for_flag(&probe->published);
  if (probe->saw_session) {
    mln_render_abandon_result abandoned = {
      .size = sizeof(mln_render_abandon_result)
    };
    probe->abandon_status =
      mln_render_session_abandon(probe->session, &abandoned, NULL);
  }
  mln_test_flag_set(&probe->completed);
}

static bool attach_abandon_settled(void* context) {
  const attach_abandon_probe* probe = context;
  return atomic_load(&probe->completed);
}

// The driver delivers an attach completion inside its driver call, like every
// other command's, so abandon from it is busy and the session stays attached.
static void abandon_from_an_attach_completion_is_busy(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  attach_abandon_probe probe = {.abandon_status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.published, false);
  atomic_init(&probe.completed, false);
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.requested_texture_ring_depth = 2;
  options.frame_wake = mln_test_pulse_wake();
  options.driver_work_wake = mln_test_pulse_wake();
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = abandon_once_attach_returns,
    .user_data = &probe,
    .release_user_data = maintenance_released,
  };
  mln_test_render_fixture fixture = {0};
  MLN_TEST_OK(
    mln_test_render_fixture_start_attach(map, &options, &completion, &fixture)
  );
  probe.session = fixture.session;
  mln_test_flag_set(&probe.published);

  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, attach_abandon_settled, &probe, mln_test_deadline_default(),
    "an attach that abandons"
  ));
  TEST_ASSERT_TRUE_MESSAGE(
    probe.saw_session, "the attach completed inside the attach call"
  );
  MLN_TEST_OK(probe.completion_status);
  MLN_TEST_STATUS(MLN_STATUS_BUSY, probe.abandon_status);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_ATTACHED, read_snapshot(fixture.session).state
  );

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
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_render_session_dispose(fixture.session, NULL));

  TEST_ASSERT_TRUE(mln_test_completion_wait(&attach, -1));
  const mln_status attach_status = mln_test_completion_status(&attach);
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    MLN_TEST_STATUS(MLN_STATUS_TARGET_LOST, attach_status);
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

// Disposing an attached session that has rendered gives queued work a terminal
// result and retires the session, freeing the map's session slot. The wake's
// release marks the point after which the session makes no graphics call, so
// the host may destroy its device then.
static void a_disposed_attached_session_releases_its_wakes_before_the_device(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
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
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
  mln_test_render_request_forced(&fixture, 1);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&fixture, 1));

  mln_test_completion queued = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_reduce_memory_use(
    fixture.session, &queued.descriptor, NULL
  ));
  MLN_TEST_OK(mln_render_session_dispose(fixture.session, NULL));
  // A core worker may run the command before it sees the disposal.
  const mln_status queued_status = mln_test_completion_settle(&queued);
  TEST_ASSERT_TRUE(
    queued_status == MLN_STATUS_OK || queued_status == MLN_STATUS_TARGET_LOST
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&released));
  mln_test_render_fixture_destroy(&fixture);

  mln_test_render_fixture other = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &other));
  mln_test_render_fixture_destroy(&other);
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
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &map));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_runtime_dispose(runtime, NULL));
  MLN_TEST_OK(mln_map_dispose(map, NULL));
  TEST_ASSERT_FALSE(atomic_load(&retired));
  MLN_TEST_OK(mln_render_session_dispose(fixture.session, NULL));
  mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_INVALID_STATE(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&retired));
  mln_test_render_fixture_destroy(&fixture);
}

static void lock_nothing(void* context) { (void)context; }

// A queue lock names both of its callbacks or neither, a disabled one retains
// no user data, and only a Vulkan target takes one, because only the Vulkan
// driver submits to a queue that the host names.
static void attach_checks_the_queue_lock(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.queue_lock.release_user_data = lock_nothing;
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_render_fixture_start_attach(
                                   map, &options, &attach.descriptor, &fixture
                                 )
  );
  options.queue_lock.release_user_data = NULL;
  options.queue_lock.lock = lock_nothing;
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_render_fixture_start_attach(
                                   map, &options, &attach.descriptor, &fixture
                                 )
  );
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
  options.queue_lock.unlock = lock_nothing;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED, mln_test_render_fixture_start_attach(
                              map, &options, &attach.descriptor, &fixture
                            )
  );
#endif
  mln_test_completion_reject(&attach);
  mln_test_completion_destroy(&attach);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A host whose wakes call back into the session instead of scheduling. It has
// file scope because a wake fired on another thread can outlast the case.
typedef struct reentrant_host {
  mln_render_session session;
  uint64_t token;
  atomic_bool armed;
  atomic_bool rendered;
  atomic_bool service_requested;
  atomic_uint inline_services;
  atomic_uint failures;
} reentrant_host;

static reentrant_host reentrant;
static MLN_TEST_THREAD_LOCAL bool on_graphics_thread;

static void drain_inside_frame_wake(void* context) {
  reentrant_host* host = context;
  if (atomic_load(&host->armed)) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    mln_status status = MLN_STATUS_OK;
    while ((status = mln_render_session_drain_frame_results(
              host->session, &batch, NULL
            )) == MLN_STATUS_OK) {
      size_t count = 0;
      if (mln_render_frame_batch_count(batch, &count, NULL) != MLN_STATUS_OK)
        atomic_fetch_add(&host->failures, 1U);
      for (size_t index = 0; index < count; index += 1) {
        mln_render_frame_result result = {
          .size = sizeof(mln_render_frame_result)
        };
        if (
          mln_render_frame_batch_get(batch, index, &result, NULL) ==
            MLN_STATUS_OK &&
          result.token == host->token &&
          result.disposition == MLN_RENDER_RESULT_RENDERED
        )
          atomic_store(&host->rendered, true);
      }
      mln_render_frame_batch_release(batch);
      batch = MLN_HANDLE_NULL;
    }
    if (status != MLN_STATUS_NOT_READY) atomic_fetch_add(&host->failures, 1U);
  }
  mln_test_pulse();
}

static mln_status service_on_graphics_thread(reentrant_host* host) {
  size_t serviced = 0;
  const mln_status status = mln_render_session_service_driver_work(
    host->session, SIZE_MAX, &serviced, NULL
  );
  if (status != MLN_STATUS_OK && status != MLN_STATUS_BUSY)
    atomic_fetch_add(&host->failures, 1U);
  return status;
}

// Services inline on the graphics thread, the one thread that a caller driver
// takes service from. A wake on that thread while a service runs further up its
// stack finds the driver busy, and the running service takes the work. A wake
// on another thread leaves the service to the case's wait.
static void service_inside_driver_wake(void* context) {
  reentrant_host* host = context;
  if (atomic_load(&host->armed)) {
    if (!on_graphics_thread)
      atomic_store(&host->service_requested, true);
    else if (service_on_graphics_thread(host) == MLN_STATUS_OK)
      atomic_fetch_add(&host->inline_services, 1U);
  }
  mln_test_pulse();
}

static bool reentrant_frame_rendered(void* context) {
  reentrant_host* host = context;
  if (atomic_exchange(&host->service_requested, false))
    (void)service_on_graphics_thread(host);
  return atomic_load(&host->rendered) || atomic_load(&host->failures) != 0;
}

// Native code invokes wakes outside the session's locks, so a host may drain
// results inside its frame wake and service a caller driver inside its
// driver-work wake. A caller driver's demand then renders and reports from
// inside request_frame on the graphics thread; a core worker reports from its
// own thread.
static void wakes_may_call_back_into_the_session(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  reentrant = (reentrant_host){.token = 7};
  on_graphics_thread = true;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.requested_texture_ring_depth = 2;
  options.frame_wake = (mln_wake){
    .size = sizeof(mln_wake),
    .callback = drain_inside_frame_wake,
    .user_data = &reentrant,
  };
  options.driver_work_wake = (mln_wake){
    .size = sizeof(mln_wake),
    .callback = service_inside_driver_wake,
    .user_data = &reentrant,
  };
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
  reentrant.session = fixture.session;
  atomic_store(&reentrant.armed, true);

  mln_test_render_request_forced(&fixture, reentrant.token);
  TEST_ASSERT_TRUE(mln_test_await(
    reentrant_frame_rendered, &reentrant, mln_test_deadline_default(),
    "a frame result drained inside the frame wake"
  ));
  atomic_store(&reentrant.armed, false);
  on_graphics_thread = false;
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&reentrant.failures));
  if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    TEST_ASSERT_NOT_EQUAL_UINT(0, atomic_load(&reentrant.inline_services));
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(an_attached_session_holds_its_map_and_refuses_destroy);
  RUN_TEST(a_detached_session_frees_its_map_and_refuses_work);
  RUN_TEST(maintenance_commands_run_in_order_with_frames);
  RUN_TEST(abandon_completes_pending_work_and_invalidates_accessors);
  RUN_TEST(abandon_from_a_driver_completion_is_busy);
  RUN_TEST(abandon_from_an_attach_completion_is_busy);
  RUN_TEST(a_session_disposed_while_attaching_frees_the_map);
  RUN_TEST(a_disposed_attached_session_releases_its_wakes_before_the_device);
  RUN_TEST(parent_first_disposal_retires_a_native_render_attachment);
  RUN_TEST(wakes_may_call_back_into_the_session);
  RUN_TEST(attach_checks_the_queue_lock);
}
