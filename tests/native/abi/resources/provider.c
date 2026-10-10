// The resource provider: registration and its user data, request handles and
// their cancel callbacks, what a request tells the provider, and how a
// provider's answer reaches the map, including through the ambient cache.

#include "support/map.h"
#include "support/resources.h"
#include "support/test_support.h"

static const char unsupported_scheme_style_url[] =
  "jar:file:/packaged/style.json";
static const char credentialed_unsupported_scheme_style_url[] =
  "jar://user:password@archive/packaged/style.json?access_token=secret#token";
static const char inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static uint32_t pass_through_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)user_data;
  (void)request;
  (void)handle;
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

static void count_runtime_callback_release(void* user_data) {
  atomic_fetch_add((atomic_int*)user_data, 1);
  mln_test_pulse();
}

static void ignore_cancel(void* user_data) { (void)user_data; }

static mln_resource_request_cancel_handler cancel_handler(
  mln_resource_request_cancel_callback callback, void* user_data,
  mln_user_data_release release_user_data
) {
  return (mln_resource_request_cancel_handler){
    .size = sizeof(mln_resource_request_cancel_handler),
    .callback = callback,
    .user_data = user_data,
    .release_user_data = release_user_data,
  };
}

static mln_status submit_provider(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_provider(
    *(const mln_runtime*)context, descriptor, &completion.descriptor, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    (void)mln_test_completion_finish(&completion);
  } else {
    mln_test_completion_reject(&completion);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

static void provider_with_zero_size(void* descriptor) {
  ((mln_resource_provider*)descriptor)->size = 0;
}

static void provider_without_callback(void* descriptor) {
  ((mln_resource_provider*)descriptor)->callback = NULL;
}

static const mln_test_validation_case provider_cases[] = {
  {"a well-formed provider", NULL, MLN_STATUS_OK, NULL},
  {"a zero size", provider_with_zero_size, MLN_STATUS_INVALID_ARGUMENT, "size"},
  {"a null callback", provider_without_callback, MLN_STATUS_INVALID_ARGUMENT,
   "callback"},
};

static void resource_provider_registration_validates_its_descriptor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = pass_through_provider,
  };
  mln_test_run_validation_table(
    provider_cases, sizeof(provider_cases) / sizeof(provider_cases[0]),
    &provider, sizeof(provider), submit_provider, &runtime
  );
  MLN_TEST_INVALID(mln_test_set_resource_provider(runtime, NULL));
  MLN_TEST_INVALID(mln_test_clear_resource_provider(MLN_HANDLE_NULL));
  mln_test_destroy_runtime(runtime);
}

static void resource_provider_registration_releases_owned_state(void) {
  atomic_int release_count;
  atomic_init(&release_count, 0);
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = pass_through_provider,
    .user_data = &release_count,
    .release_user_data = count_runtime_callback_release,
  };

  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_runtime_set_resource_provider(
    MLN_HANDLE_NULL, &provider, &rejected, NULL
  ));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&release_count));

  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&release_count));
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&release_count));
  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  TEST_ASSERT_EQUAL_INT(2, atomic_load(&release_count));
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(3, atomic_load(&release_count));
}

static void custom_provider_request_handles_reject_raw_null_handles(void) {
  mln_resource_request_release(MLN_HANDLE_NULL);
  const mln_resource_response response =
    mln_test_text_response(inline_style_json);
  bool cancelled = false;
  MLN_TEST_INVALID(
    mln_resource_request_cancelled(MLN_HANDLE_NULL, &cancelled, NULL)
  );
  MLN_TEST_INVALID(
    mln_resource_request_complete(MLN_HANDLE_NULL, &response, NULL)
  );
  const mln_resource_request_cancel_handler handler =
    cancel_handler(ignore_cancel, NULL, NULL);
  MLN_TEST_INVALID(mln_resource_request_set_cancel_callback(
    MLN_HANDLE_NULL, &handler, &cancelled, NULL
  ));
  MLN_TEST_INVALID(
    mln_resource_request_wait_until_retired(MLN_HANDLE_NULL, NULL)
  );
}

typedef struct provider_request_probe {
  atomic_bool entered;
} provider_request_probe;

typedef struct cross_thread_provider_submission {
  mln_runtime runtime;
  mln_resource_provider provider;
  mln_status status;
  mln_test_completion completion;
} cross_thread_provider_submission;

static void submit_provider_from_thread(void* user_data) {
  cross_thread_provider_submission* submission = user_data;
  submission->status = mln_runtime_set_resource_provider(
    submission->runtime, &submission->provider,
    &submission->completion.descriptor, NULL
  );
}

static uint32_t recording_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  (void)handle;
  provider_request_probe* probe = user_data;
  mln_test_flag_set(&probe->entered);
  // An unknown decision becomes a handled provider error, which keeps the
  // request off the network.
  return UINT32_MAX;
}

static void resource_provider_command_copies_cross_thread_descriptor(void) {
  provider_request_probe probe = {0};
  mln_runtime runtime = mln_test_create_runtime();
  cross_thread_provider_submission submission = {
    .runtime = runtime,
    .provider =
      {
        .size = sizeof(mln_resource_provider),
        .callback = recording_resource_provider,
        .user_data = &probe,
      },
    .status = MLN_STATUS_NATIVE_ERROR,
    .completion = mln_test_completion_default(0),
  };
  mln_test_thread* thread =
    mln_test_thread_start(submit_provider_from_thread, &submission);
  TEST_ASSERT_NOT_NULL(thread);
  mln_test_thread_join(thread);
  MLN_TEST_OK(submission.status);

  // The accepted command owns the descriptor shape, not this binding storage.
  submission.provider.callback = NULL;
  submission.provider.user_data = NULL;
  MLN_TEST_OK(mln_test_completion_finish(&submission.completion));
  mln_test_completion_destroy(&submission.completion);

  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://copied.json"));
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_TRUE(atomic_load(&probe.entered));
  TEST_ASSERT_NOT_NULL(strstr(message, "unknown decision"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct dropped_request_probe {
  atomic_bool release_inline;
  _Atomic mln_resource_request_handle handle;
} dropped_request_probe;

static uint32_t claim_and_drop_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  dropped_request_probe* probe = user_data;
  atomic_store(&probe->handle, handle);
  mln_test_pulse();
  if (atomic_load(&probe->release_inline)) {
    mln_resource_request_release(handle);
  }
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static bool dropped_request_claimed(void* context) {
  dropped_request_probe* probe = context;
  return atomic_load(&probe->handle) != MLN_HANDLE_NULL;
}

static mln_map start_claimed_request(
  mln_runtime runtime, dropped_request_probe* probe
) {
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = claim_and_drop_resource_provider,
    .user_data = probe,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_style_url(map, "custom://dropped-request-style.json")
  );
  TEST_ASSERT_TRUE(mln_test_await(
    dropped_request_claimed, probe, mln_test_deadline_default(),
    "the provider to claim the request"
  ));
  return map;
}

static void expect_dropped_request_fails(bool release_inline) {
  dropped_request_probe probe = {0};
  atomic_store(&probe.release_inline, release_inline);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_claimed_request(runtime, &probe);
  if (!release_inline) {
    mln_resource_request_release(atomic_load(&probe.handle));
  }
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A provider that claims a request and releases it unanswered fails it, so the
// load reports an error instead of waiting forever.
static void releasing_a_claimed_request_without_a_response_fails_it(void) {
  expect_dropped_request_fails(false);
}

// The same holds when the provider releases the handle inside its callback
// before answering HANDLE.
static void releasing_a_request_inside_its_callback_then_claiming_fails_it(
  void
) {
  expect_dropped_request_fails(true);
}

// A resource request is not a child of its runtime. Releasing a claimed request
// unanswered sends its failure to the map on the runtime's worker, and that
// failure still being queued holds neither the map nor the runtime open: both
// releases are accepted while the worker is parked in front of it.
static void a_released_request_does_not_hold_its_runtime_open(void) {
  dropped_request_probe probe = {0};
  // The case releases both handles itself, so it creates them untracked.
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_runtime_options options = mln_runtime_options_default();
  options.event_wake = mln_test_pulse_wake();
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, MLN_TEST_DIAGNOSTIC));
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = claim_and_drop_resource_provider,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &map));
  MLN_TEST_OK(
    mln_test_map_set_style_url(map, "custom://dropped-request-style.json")
  );
  TEST_ASSERT_TRUE(mln_test_await(
    dropped_request_claimed, &probe, mln_test_deadline_default(),
    "the provider to claim the request"
  ));

  mln_test_gate gate;
  mln_test_gate_init(&gate);
  const mln_completion hold = mln_test_gate_completion(&gate);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &hold, NULL));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(&gate), "the runtime worker never parked"
  );

  // The failure goes to the parked worker's queue.
  mln_resource_request_release(atomic_load(&probe.handle));
  mln_test_completion map_release = mln_test_completion_default(0);
  const mln_status map_status =
    mln_map_release(map, &map_release.descriptor, MLN_TEST_DIAGNOSTIC);
  mln_test_completion runtime_release = mln_test_completion_default(0);
  const mln_status runtime_status = mln_runtime_release(
    runtime, &runtime_release.descriptor, MLN_TEST_DIAGNOSTIC
  );
  mln_test_gate_release(&gate);

  MLN_TEST_OK_MESSAGE(map_status, mln_test_last_error());
  MLN_TEST_OK_MESSAGE(runtime_status, mln_test_last_error());
  MLN_TEST_OK(mln_test_completion_finish(&map_release));
  MLN_TEST_OK(mln_test_completion_finish(&runtime_release));
  mln_test_completion_destroy(&map_release);
  mln_test_completion_destroy(&runtime_release);
}

// A claimed request that the provider keeps holding does not hold the map or
// the runtime open either, and the provider can still release its handle
// once both are gone.
static void a_held_request_does_not_hold_its_runtime_open(void) {
  dropped_request_probe probe = {0};
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_claimed_request(runtime, &probe);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  const mln_resource_request_handle handle = atomic_load(&probe.handle);
  mln_resource_request_release(handle);
  MLN_TEST_OK(mln_resource_request_wait_until_retired(handle, NULL));
}

static void unsupported_style_url_scheme_names_scheme_and_url(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, unsupported_scheme_style_url));
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, unsupported_scheme_style_url));
  TEST_ASSERT_NOT_NULL(strstr(message, "\"jar\""));
  TEST_ASSERT_NOT_NULL(strstr(message, "mln_runtime_set_resource_provider"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void unsupported_style_url_diagnostic_redacts_credentials(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_style_url(map, credentialed_unsupported_scheme_style_url)
  );
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "jar://archive/packaged/style.json"));
  TEST_ASSERT_NULL(strstr(message, "user"));
  TEST_ASSERT_NULL(strstr(message, "password"));
  TEST_ASSERT_NULL(strstr(message, "access_token"));
  TEST_ASSERT_NULL(strstr(message, "secret"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void unsupported_style_url_names_declining_provider(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = pass_through_provider,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, unsupported_scheme_style_url));
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "registered resource provider"));
  TEST_ASSERT_NOT_NULL(strstr(message, "declined"));
  TEST_ASSERT_NULL(strstr(message, "register a resource provider"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct inline_release_provider_state {
  atomic_bool callback_finished;
  atomic_int completion_status;
} inline_release_provider_state;

static uint32_t inline_release_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  inline_release_provider_state* state = user_data;
  const mln_resource_response response =
    mln_test_text_response(inline_style_json);
  (void)request;
  atomic_store(
    &state->completion_status,
    mln_resource_request_complete(handle, &response, NULL)
  );
  mln_resource_request_release(handle);
  mln_test_flag_set(&state->callback_finished);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static void resource_provider_defers_inline_release_until_callback_returns(
  void
) {
  inline_release_provider_state state;
  atomic_init(&state.callback_finished, false);
  atomic_init(&state.completion_status, MLN_STATUS_NATIVE_ERROR);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = inline_release_resource_provider,
    .user_data = &state,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://inline-style.json"));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  TEST_ASSERT_TRUE(atomic_load(&state.callback_finished));
  MLN_TEST_OK(atomic_load(&state.completion_status));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct cancel_probe {
  atomic_bool provider_entered;
  atomic_int cancel_count;
  atomic_int release_count;
  atomic_bool released_before_cancel;
  atomic_bool release_inside_callback;
  atomic_bool skip_register;
  atomic_int register_status;
  atomic_bool register_reported_cancelled;
  atomic_bool rejected_invalid_handlers;
  _Atomic mln_resource_request_handle handle;
} cancel_probe;

static void count_cancel(void* user_data) {
  cancel_probe* probe = user_data;
  if (atomic_load(&probe->release_count) != 0) {
    mln_test_flag_set(&probe->released_before_cancel);
  }
  atomic_fetch_add(&probe->cancel_count, 1);
  if (atomic_load(&probe->release_inside_callback)) {
    mln_resource_request_release(atomic_load(&probe->handle));
  }
  mln_test_pulse();
}

static void count_cancel_release(void* user_data) {
  cancel_probe* probe = user_data;
  atomic_fetch_add(&probe->release_count, 1);
  mln_test_pulse();
}

// Each invalid handler fails without taking the request's one registration
// or releasing its user data.
static bool rejects_invalid_cancel_handlers(
  mln_resource_request_handle handle, cancel_probe* probe
) {
  mln_resource_request_cancel_handler undersized =
    cancel_handler(count_cancel, probe, count_cancel_release);
  undersized.size -= 1;
  const mln_resource_request_cancel_handler without_callback =
    cancel_handler(NULL, probe, count_cancel_release);
  const mln_resource_request_cancel_handler* const handlers[] = {
    NULL, &undersized, &without_callback
  };
  for (size_t index = 0; index < sizeof(handlers) / sizeof(*handlers);
       ++index) {
    bool cancelled = false;
    if (
      mln_resource_request_set_cancel_callback(
        handle, handlers[index], &cancelled, NULL
      ) != MLN_STATUS_INVALID_ARGUMENT
    ) {
      return false;
    }
  }
  return true;
}

static uint32_t cancel_probe_resource_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)request;
  cancel_probe* probe = user_data;
  atomic_store(&probe->handle, handle);
  if (!atomic_load(&probe->skip_register)) {
    atomic_store(
      &probe->rejected_invalid_handlers,
      rejects_invalid_cancel_handlers(handle, probe)
    );
    bool cancelled = true;
    const mln_resource_request_cancel_handler handler =
      cancel_handler(count_cancel, probe, count_cancel_release);
    atomic_store(
      &probe->register_status, mln_resource_request_set_cancel_callback(
                                 handle, &handler, &cancelled, NULL
                               )
    );
    atomic_store(&probe->register_reported_cancelled, cancelled);
  }
  mln_test_flag_set(&probe->provider_entered);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static mln_map start_cancel_probe_request(
  mln_runtime runtime, cancel_probe* probe
) {
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = cancel_probe_resource_provider,
    .user_data = probe,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://cancel-style.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe->provider_entered));
  if (!atomic_load(&probe->skip_register)) {
    TEST_ASSERT_TRUE(atomic_load(&probe->rejected_invalid_handlers));
    MLN_TEST_OK(atomic_load(&probe->register_status));
    TEST_ASSERT_FALSE(atomic_load(&probe->register_reported_cancelled));
  }
  return map;
}

// Destroying the map discards its pending style request. MapLibre then cancels
// the handled request, which runs the registered callback once. The request
// keeps that single registration, and rejects a late completion.
static void cancel_callback_runs_when_map_discards_request(void) {
  cancel_probe probe = {0};
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));

  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&probe.cancel_count, 1));
  const mln_resource_request_handle handle = atomic_load(&probe.handle);
  // The context retires as the callback returns, before the request does.
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&probe.release_count, 1));
  TEST_ASSERT_FALSE(atomic_load(&probe.released_before_cancel));

  bool cancelled = false;
  MLN_TEST_OK(mln_resource_request_cancelled(handle, &cancelled, NULL));
  TEST_ASSERT_TRUE(cancelled);
  const mln_resource_response response =
    mln_test_text_response(inline_style_json);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_resource_request_complete(handle, &response, NULL)
  );

  cancelled = false;
  const mln_resource_request_cancel_handler unreleased =
    cancel_handler(count_cancel, &probe, NULL);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_resource_request_set_cancel_callback(
                                handle, &unreleased, &cancelled, NULL
                              )
  );
  TEST_ASSERT_FALSE(cancelled);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));

  mln_resource_request_release(handle);
  MLN_TEST_INVALID_STATE(mln_resource_request_set_cancel_callback(
    handle, &unreleased, &cancelled, NULL
  ));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

// A registration that arrives after cancellation stores nothing and reports
// the cancellation through out_cancelled instead of invoking the callback.
typedef struct cancelled_poll {
  mln_resource_request_handle handle;
  bool cancelled;
  mln_status status;
} cancelled_poll;

// Cancellation lands on a MapLibre thread with no callback registered to
// report it, so the wait re-checks the request's cancelled state.
static bool request_reports_cancelled(void* context) {
  cancelled_poll* poll = context;
  poll->status =
    mln_resource_request_cancelled(poll->handle, &poll->cancelled, NULL);
  return poll->status != MLN_STATUS_OK || poll->cancelled;
}

static void late_cancel_callback_registration_reports_cancelled(void) {
  cancel_probe probe = {0};
  mln_test_flag_set(&probe.skip_register);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);
  mln_test_destroy_map(map);
  const mln_resource_request_handle handle = atomic_load(&probe.handle);

  cancelled_poll poll = {.handle = handle, .status = MLN_STATUS_OK};
  TEST_ASSERT_TRUE(mln_test_await(
    request_reports_cancelled, &poll, mln_test_deadline_default(),
    "the request to report cancelled"
  ));
  MLN_TEST_OK(poll.status);
  TEST_ASSERT_TRUE(poll.cancelled);

  bool cancelled = false;
  const mln_resource_request_cancel_handler handler =
    cancel_handler(count_cancel, &probe, count_cancel_release);
  MLN_TEST_OK(
    mln_resource_request_set_cancel_callback(handle, &handler, &cancelled, NULL)
  );
  TEST_ASSERT_TRUE(cancelled);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));

  // A registration the C API did not keep leaves user_data with the caller.
  mln_resource_request_release(handle);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.cancel_count));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

// The callback runs unlocked, so releasing the cancelled handle from inside it
// retires the request without deadlocking.
static void cancel_callback_may_release_the_request(void) {
  cancel_probe probe = {0};
  mln_test_flag_set(&probe.release_inside_callback);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = start_cancel_probe_request(runtime, &probe);

  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&probe.cancel_count, 1));
  const mln_resource_request_handle handle = atomic_load(&probe.handle);
  MLN_TEST_OK(mln_resource_request_wait_until_retired(handle, NULL));
  bool cancelled = false;
  const mln_resource_request_cancel_handler unreleased =
    cancel_handler(count_cancel, &probe, NULL);
  MLN_TEST_INVALID_STATE(mln_resource_request_set_cancel_callback(
    handle, &unreleased, &cancelled, NULL
  ));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.cancel_count));
  mln_test_destroy_runtime(runtime);
}

// A style named through the tile server's URI scheme alias reaches the
// provider twice over: as the alias the style names, which is also the
// request's cache identity, and as the URL the built-in network stack would
// fetch.
static void a_request_names_its_alias_and_its_resolved_url(void) {
  static const char alias_url[] = "maplibre://maps/streets";
  static const mln_test_provided_resource resources[] = {
    {.url = alias_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)inline_style_json,
       .byte_count = sizeof(inline_style_json) - 1,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, alias_url));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));

  const mln_test_provider_request* request =
    mln_test_provider_request_at(provider, alias_url, 0);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, request->kind);
  TEST_ASSERT_EQUAL_STRING(alias_url, request->requested_url);
  TEST_ASSERT_EQUAL_STRING_LEN("https://", request->resolved_url, 8);
  TEST_ASSERT_NULL(strstr(request->resolved_url, "maplibre://"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// A PMTiles source asks the provider for its archive with a byte range, the
// header, while the style request carries none. The archive is unanswered, so
// nothing past the header is asked for.
static void a_pmtiles_request_carries_its_byte_range(void) {
  static const char style_url[] = "custom://pmtiles/style.json";
  static const char archive_url[] = "custom://pmtiles/archive.pmtiles";
  static const char style_json[] =
    "{\"version\":8,\"sources\":{\"archive\":{\"type\":\"vector\","
    "\"url\":\"pmtiles://custom://pmtiles/archive.pmtiles\"}},"
    "\"layers\":[]}";
  static const mln_test_provided_resource resources[] = {
    {.url = style_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)style_json,
       .byte_count = sizeof(style_json) - 1,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, style_url));
  TEST_ASSERT_TRUE(
    mln_test_provider_wait_for_requests(provider, archive_url, 1)
  );

  const mln_test_provider_request* style =
    mln_test_provider_request_at(provider, style_url, 0);
  TEST_ASSERT_NOT_NULL(style);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, style->kind);
  TEST_ASSERT_BITS_LOW(MLN_RESOURCE_REQUEST_RANGE, style->fields);

  const mln_test_provider_request* archive =
    mln_test_provider_request_at(provider, archive_url, 0);
  TEST_ASSERT_NOT_NULL(archive);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_SOURCE, archive->kind);
  TEST_ASSERT_BITS_HIGH(MLN_RESOURCE_REQUEST_RANGE, archive->fields);
  TEST_ASSERT_EQUAL_UINT64(0, archive->range.start);
  TEST_ASSERT_GREATER_THAN_UINT64(archive->range.start, archive->range.end);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// One vector source with one tile at zoom 0, which a 64 by 64 static map
// requests once.
static const char tiled_style_json[] =
  "{\"version\":8,\"sources\":{\"tiles\":{\"type\":\"vector\",\"tiles\":"
  "[\"custom://tiles/{z}/{x}/{y}.pbf\"],\"minzoom\":0,\"maxzoom\":0}},"
  "\"layers\":[{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"tiles\","
  "\"source-layer\":\"any\"}]}";
static const char tile_url[] = "custom://tiles/0/0/0.pbf";

typedef struct tile_answer_case {
  const char* label;
  mln_resource_response response;
  // Whether the still image completes. A tile error fails it; a tile that
  // does not exist, or has no content, renders as an empty tile.
  bool renders;
} tile_answer_case;

static const tile_answer_case tile_answer_cases[] = {
  {"an empty tile", {.status = MLN_RESOURCE_RESPONSE_STATUS_OK}, true},
  {"no content", {.status = MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT}, true},
  {"not found",
   {.status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
    .error_message = "tile not found"},
   true},
  {"a server error",
   {.status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_SERVER,
    .error_message = "tile server error"},
   false},
  {"a connection error",
   {.status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_CONNECTION,
    .error_message = "tile connection error"},
   false},
  {"a rate limit",
   {.status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_RATE_LIMIT,
    .error_message = "tile rate limit"},
   false},
  {"another error",
   {.status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = MLN_RESOURCE_ERROR_REASON_OTHER,
    .error_message = "tile other error"},
   false},
};

// A provider's answer for a tile decides whether the map renders: a missing or
// empty tile renders as nothing, and any other error fails the still image.
static void a_tile_answer_decides_whether_the_map_renders(void) {
  for (size_t index = 0;
       index < sizeof(tile_answer_cases) / sizeof(tile_answer_cases[0]);
       index += 1) {
    const tile_answer_case* row = &tile_answer_cases[index];
    const mln_test_provided_resource resources[] = {
      {.url = tile_url, .response = row->response},
    };
    mln_test_provider* provider = mln_test_provider_create(resources, 1);
    mln_runtime runtime = mln_test_create_runtime();
    mln_test_provider_install(runtime, provider);
    mln_map_options options = mln_map_options_default();
    options.initial_extent =
      (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
    options.map_mode = MLN_MAP_MODE_STATIC;
    mln_map map = mln_test_create_map_with_options(runtime, &options);
    mln_test_load_style_and_wait(
      runtime, map, MLN_BUFFER_LITERAL(tiled_style_json)
    );
    mln_test_render_fixture fixture = {0};
    TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

    const mln_status still = mln_test_render_still_image(&fixture, map);
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_provider_requests(provider, tile_url) >= 1, row->label
    );
    if (row->renders) {
      MLN_TEST_OK_MESSAGE(still, row->label);
    } else {
      TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(MLN_STATUS_OK, still, row->label);
      TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(
        MLN_STATUS_NOT_READY, still, row->label
      );
    }
    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
    mln_test_provider_destroy(provider);
  }
}

// A provider error for the style becomes the map's loading failure, carrying
// the provider's message, or a generic one when it gave none. A response whose
// fields carry an unknown bit is malformed, and fails the same way.
static void a_style_error_reaches_the_loading_failure(void) {
  static const mln_test_provided_resource resources[] = {
    {.url = "custom://described.json",
     .response =
       {
         .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
         .error_reason = MLN_RESOURCE_ERROR_REASON_SERVER,
         .error_message = "the style server is down",
       }},
    {.url = "custom://undescribed.json",
     .response =
       {
         .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
         .error_reason = MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
       }},
    {.url = "custom://unknown-field.json",
     .response = {
       .fields = UINT32_C(1) << 31,
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)tiled_style_json,
       .byte_count = sizeof(tiled_style_json) - 1,
     }},
  };
  static const char* const expected[] = {
    "loading style failed: the style server is down",
    "loading style failed: resource provider failed",
    "loading style failed: mln_resource_response.fields contains unknown bits",
  };
  const size_t count = sizeof(resources) / sizeof(resources[0]);
  mln_test_provider* provider = mln_test_provider_create(resources, count);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  for (size_t index = 0; index < count; index += 1) {
    mln_map map = mln_test_create_map(runtime);
    MLN_TEST_OK(mln_test_map_set_style_url(map, resources[index].url));
    char message[512];
    TEST_ASSERT_TRUE(
      mln_test_await_loading_failure(runtime, map, message, sizeof(message))
    );
    TEST_ASSERT_EQUAL_STRING(expected[index], message);
    mln_test_destroy_map(map);
  }
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// Loads `url` on a new map and waits for its style, which puts the provider's
// answer in the runtime's ambient cache. The default cache is in memory and
// lives only while something holds it, so a case keeps this map until its
// next load has read the cache.
static mln_map load_style(mln_runtime runtime, const char* url) {
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  return map;
}

static const char cached_style_url[] = "custom://cached-style.json";
// Whole seconds, since the cache stores timestamps at that precision.
static const int64_t style_modified_unix_ms = 1700000000000;
static const int64_t style_expired_unix_ms = 1700000060000;

static mln_resource_response cacheable_style(bool must_revalidate) {
  return (mln_resource_response){
    .size = sizeof(mln_resource_response),
    .fields = MLN_RESOURCE_RESPONSE_MODIFIED | MLN_RESOURCE_RESPONSE_EXPIRES,
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .bytes = (const uint8_t*)inline_style_json,
    .byte_count = sizeof(inline_style_json) - 1,
    .must_revalidate = must_revalidate,
    .modified_unix_ms = style_modified_unix_ms,
    .expires_unix_ms = style_expired_unix_ms,
    .etag = "\"v1\"",
  };
}

static void expect_revalidation_of_cached_style(
  const mln_test_provider_request* request, bool carries_prior_data
) {
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_TRUE(request->has_prior_etag);
  TEST_ASSERT_EQUAL_STRING("\"v1\"", request->prior_etag);
  TEST_ASSERT_BITS_HIGH(
    MLN_RESOURCE_REQUEST_PRIOR_MODIFIED | MLN_RESOURCE_REQUEST_PRIOR_EXPIRES,
    request->fields
  );
  TEST_ASSERT_EQUAL_INT64(
    style_modified_unix_ms, request->prior_modified_unix_ms
  );
  TEST_ASSERT_EQUAL_INT64(
    style_expired_unix_ms, request->prior_expires_unix_ms
  );
  TEST_ASSERT_EQUAL_size_t(
    carries_prior_data ? sizeof(inline_style_json) - 1 : 0,
    request->prior_data_size
  );
}

// A provider answer's metadata goes into the ambient cache, and the next
// request for the resource carries it back. An expired entry that must be
// revalidated is withheld from the map until the provider answers, so the
// request also carries its bytes, and NOT_MODIFIED delivers them.
static void a_not_modified_answer_delivers_the_cached_style(void) {
  const mln_test_provided_resource resources[] = {
    {.url = cached_style_url,
     .response = cacheable_style(true),
     .has_later_response = true,
     .later_response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map first = load_style(runtime, cached_style_url);
  const mln_test_provider_request* initial =
    mln_test_provider_request_at(provider, cached_style_url, 0);
  TEST_ASSERT_NOT_NULL(initial);
  TEST_ASSERT_FALSE(initial->has_prior_etag);
  TEST_ASSERT_BITS_LOW(MLN_RESOURCE_REQUEST_PRIOR_MODIFIED, initial->fields);
  TEST_ASSERT_EQUAL_size_t(0, initial->prior_data_size);

  mln_map second = load_style(runtime, cached_style_url);
  expect_revalidation_of_cached_style(
    mln_test_provider_request_at(provider, cached_style_url, 1), true
  );
  mln_test_destroy_map(second);
  mln_test_destroy_map(first);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// An expired entry that need not be revalidated reaches the map straight from
// the cache: the style loads while the provider still holds the revalidation,
// which carries the entry's metadata but not its bytes.
static void a_usable_cached_style_loads_before_the_provider_answers(void) {
  const mln_test_provided_resource resources[] = {
    {.url = cached_style_url,
     .response = cacheable_style(false),
     .later_held = true},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map first = load_style(runtime, cached_style_url);

  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, cached_style_url));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  TEST_ASSERT_TRUE(
    mln_test_provider_wait_for_requests(provider, cached_style_url, 2)
  );
  const mln_test_provider_request* revalidation =
    mln_test_provider_request_at(provider, cached_style_url, 1);
  expect_revalidation_of_cached_style(revalidation, false);
  const mln_resource_response not_modified = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
  };
  MLN_TEST_OK(
    mln_resource_request_complete(revalidation->handle, &not_modified, NULL)
  );
  mln_resource_request_release(revalidation->handle);
  mln_test_destroy_map(map);
  mln_test_destroy_map(first);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

MLN_TEST_GROUP {
  RUN_TEST(resource_provider_registration_validates_its_descriptor);
  RUN_TEST(resource_provider_registration_releases_owned_state);
  RUN_TEST(custom_provider_request_handles_reject_raw_null_handles);
  RUN_TEST(resource_provider_command_copies_cross_thread_descriptor);
  RUN_TEST(releasing_a_claimed_request_without_a_response_fails_it);
  RUN_TEST(releasing_a_request_inside_its_callback_then_claiming_fails_it);
  RUN_TEST(a_released_request_does_not_hold_its_runtime_open);
  RUN_TEST(a_held_request_does_not_hold_its_runtime_open);
  RUN_TEST(unsupported_style_url_scheme_names_scheme_and_url);
  RUN_TEST(unsupported_style_url_diagnostic_redacts_credentials);
  RUN_TEST(unsupported_style_url_names_declining_provider);
  RUN_TEST(resource_provider_defers_inline_release_until_callback_returns);
  RUN_TEST(cancel_callback_runs_when_map_discards_request);
  RUN_TEST(late_cancel_callback_registration_reports_cancelled);
  RUN_TEST(cancel_callback_may_release_the_request);
  RUN_TEST(a_request_names_its_alias_and_its_resolved_url);
  RUN_TEST(a_pmtiles_request_carries_its_byte_range);
  RUN_TEST(a_tile_answer_decides_whether_the_map_renders);
  RUN_TEST(a_style_error_reaches_the_loading_failure);
  RUN_TEST(a_not_modified_answer_delivers_the_cached_style);
  RUN_TEST(a_usable_cached_style_loads_before_the_provider_answers);
}
