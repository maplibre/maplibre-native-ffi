// Dispatched delivery: registrations and completions that name a queue
// alongside their callback run in mln_delivery_queue_dispatch() on the
// calling thread, in push order, with their release after their last call. A
// dispatched call may call any C function: dispose its owner, release the
// queue, or dispatch again, which reports BUSY. Releasing a queue abandons
// what is pending without running it.

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

// Set on the thread that runs a case, which is the only one that dispatches.
static MLN_TEST_THREAD_LOCAL bool on_case_thread;

enum { max_calls = 16 };

// The calls a case's queue ran, by kind, in the order they ran. Callbacks
// record instead of asserting, since a failed assertion would unwind through
// the dispatch that runs them.
typedef struct call_log {
  atomic_int off_thread;
  size_t count;
  char kinds[max_calls + 1];
  uint64_t generations[max_calls];
} call_log;

static void record(call_log* log, char kind, uint64_t generation) {
  if (!on_case_thread) {
    atomic_fetch_add(&log->off_thread, 1);
    return;
  }
  if (log->count < max_calls) {
    log->kinds[log->count] = kind;
    log->generations[log->count] = generation;
  }
  log->count += 1;
}

static void start_case(call_log* log) {
  memset(log, 0, sizeof(*log));
  on_case_thread = true;
}

static void release_registration(void* user_data) { record(user_data, 'r', 0); }

static void release_completion(void* user_data) { record(user_data, 'c', 0); }

static mln_completion dispatched_completion(
  mln_delivery_queue queue, mln_completion_callback callback, void* user_data,
  mln_user_data_release release
) {
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = callback,
    .user_data = user_data,
    .release_user_data = release,
    .queue = queue,
  };
}

typedef struct kind_wait {
  mln_delivery_queue queue;
  const call_log* log;
  char kind;
  mln_status failure;
} kind_wait;

static bool dispatched_kind(void* context) {
  kind_wait* wait = context;
  const mln_status status = mln_delivery_queue_dispatch(wait->queue, NULL);
  if (status != MLN_STATUS_OK && status != MLN_STATUS_NOT_READY) {
    wait->failure = status;
    return true;
  }
  return memchr(wait->log->kinds, wait->kind, wait->log->count) != NULL;
}

// Dispatches queue as its wake pulses until a call of kind has run.
static void dispatch_until(
  mln_delivery_queue queue, const call_log* log, char kind
) {
  kind_wait wait = {
    .queue = queue, .log = log, .kind = kind, .failure = MLN_STATUS_OK
  };
  TEST_ASSERT_TRUE(mln_test_await(
    dispatched_kind, &wait, mln_test_deadline_default(), "a dispatched call"
  ));
  MLN_TEST_OK(wait.failure);
}

// An empty queue reports NOT_READY, and a released or foreign handle is
// refused. A queue's own wake cannot name a queue.
static void dispatch_reports_its_status(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY, mln_delivery_queue_dispatch(queue.handle, NULL)
  );
  MLN_TEST_INVALID(mln_delivery_queue_dispatch(MLN_HANDLE_NULL, NULL));
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_INVALID(mln_delivery_queue_dispatch(runtime, NULL));
  mln_test_destroy_runtime(runtime);

  mln_delivery_queue other = MLN_HANDLE_NULL;
  const mln_delivery_queue_options options = {
    .size = sizeof(options),
    .wake = {.callback = mln_test_delivery_count_wake, .queue = queue.handle},
  };
  MLN_TEST_INVALID(mln_delivery_queue_create(&options, &other, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, other);

  mln_test_delivery_queue_release(&queue);
  MLN_TEST_INVALID_STATE(mln_delivery_queue_dispatch(queue.handle, NULL));
}

static void record_events(void* user_data, const mln_event_batch_view* events) {
  for (size_t index = 0; index < events->event_count; index += 1) {
    const mln_runtime_event* event = MLN_STRIDED_AT(
      mln_runtime_event, events->events, events->event_size, index
    );
    record(user_data, 'E', event->generation);
  }
}

static void record_generation(
  void* user_data, const mln_completion_result* result
) {
  record(
    user_data, result->status == MLN_STATUS_OK ? 'C' : 'F', result->generation
  );
}

// A map command's events run before its completion, whose release follows
// it, all on the dispatching thread. Clearing the handler queues its release
// after its last call.
static void events_and_their_completion_run_in_push_order(void) {
  static call_log log;
  start_case(&log);
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED)
  );
  const mln_runtime_event_handler handler = {
    .size = sizeof(handler),
    .callback = record_events,
    .user_data = &log,
    .release_user_data = release_registration,
    .queue = queue.handle,
  };
  MLN_TEST_AWAIT_OK(mln_runtime_set_event_handler(
    runtime, &handler, &completion.descriptor, NULL
  ));

  const mln_completion styled = dispatched_completion(
    queue.handle, record_generation, &log, release_completion
  );
  MLN_TEST_OK(
    mln_map_set_style_json(map, mln_test_background_style_json, &styled, NULL)
  );
  dispatch_until(queue.handle, &log, 'c');
  TEST_ASSERT_EQUAL_STRING("ECc", log.kinds);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, log.generations[0]);
  TEST_ASSERT_EQUAL_UINT64(log.generations[0], log.generations[1]);

  MLN_TEST_AWAIT_OK(
    mln_runtime_clear_event_handler(runtime, &completion.descriptor, NULL)
  );
  dispatch_until(queue.handle, &log, 'r');
  TEST_ASSERT_EQUAL_STRING("ECcr", log.kinds);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&log.off_thread));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

typedef struct answering_provider {
  call_log log;
  mln_status answer;
} answering_provider;

// Answers each request inside the call through the handle that it owns.
static void answer_request(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  answering_provider* provider = user_data;
  record(&provider->log, 'P', 0);
  const bool lent_url =
    request->requested_url.size != 0 &&
    mln_test_lent_text_is_terminated(request->requested_url);
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .bytes = inline_style_json,
    .byte_count = sizeof(inline_style_json) - 1,
  };
  provider->answer = lent_url
                       ? mln_resource_request_complete(handle, &response, NULL)
                       : MLN_STATUS_INVALID_ARGUMENT;
}

static void release_provider(void* user_data) {
  answering_provider* provider = user_data;
  record(&provider->log, 'r', 0);
}

// A dispatched provider call owns its request handle, and the style loads with
// the answer it gives there. The provider's release runs in dispatch once a
// clear retires it.
static void a_dispatched_request_passes_its_handle_to_the_call(void) {
  static answering_provider provider;
  start_case(&provider.log);
  provider.answer = MLN_STATUS_NATIVE_ERROR;
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider registration = {
    .size = sizeof(registration),
    .callback = answer_request,
    .user_data = &provider,
    .release_user_data = release_provider,
    .queue = queue.handle,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &registration));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_style_url(map, "custom://dispatched-provider.json")
  );

  dispatch_until(queue.handle, &provider.log, 'P');
  MLN_TEST_OK(provider.answer);
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  dispatch_until(queue.handle, &provider.log, 'r');
  TEST_ASSERT_EQUAL_STRING("Pr", provider.log.kinds);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&provider.log.off_thread));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

// Releasing the queue abandons the pending request, which fails because no
// call took its handle, and the release of a registration that still names
// the queue never runs.
static void releasing_the_queue_abandons_pending_calls_and_releases(void) {
  static answering_provider provider;
  start_case(&provider.log);
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider registration = {
    .size = sizeof(registration),
    .callback = answer_request,
    .user_data = &provider,
    .release_user_data = release_provider,
    .queue = queue.handle,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &registration));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://abandoned.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, 1));

  mln_test_delivery_queue_release(&queue);
  char message[256] = {0};
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));
  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  TEST_ASSERT_EQUAL_size_t(0, provider.log.count);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&provider.log.off_thread));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// What a reentrant call saw, for the case to check after dispatch returns.
typedef struct reentry {
  call_log log;
  mln_delivery_queue queue;
  mln_map map;
  mln_status nested;
  mln_status concurrent;
  mln_status submitted;
  mln_status fenced;
} reentry;

static void record_later(void* user_data, const mln_completion_result* result) {
  reentry* state = user_data;
  record(&state->log, result->status == MLN_STATUS_OK ? 'L' : 'F', 0);
}

static void dispatch_from_another_thread(void* argument) {
  reentry* state = argument;
  state->concurrent = mln_delivery_queue_dispatch(state->queue, NULL);
}

// Dispatches again from inside the call and from another thread while it
// runs, then queues a later completion and waits until it is queued.
static void reenter(void* user_data, const mln_completion_result* result) {
  reentry* state = user_data;
  record(&state->log, result->status == MLN_STATUS_OK ? 'A' : 'F', 0);
  state->nested = mln_delivery_queue_dispatch(state->queue, NULL);
  mln_test_thread* thread =
    mln_test_thread_start(dispatch_from_another_thread, state);
  mln_test_thread_join(thread);
  const mln_completion later =
    dispatched_completion(state->queue, record_later, state, NULL);
  state->submitted = mln_map_set_debug_options(state->map, 0, &later, NULL);
  // A later completion on the map follows the queued one.
  state->fenced = mln_test_map_fence(state->map);
}

// A dispatch while another runs, further up the stack or on another thread,
// reports BUSY, and the running dispatch then also runs what was queued by
// the time BUSY returned.
static void a_dispatch_inside_a_call_is_busy(void) {
  static reentry state;
  start_case(&state.log);
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  state.queue = queue.handle;
  state.map = mln_test_create_map(runtime);
  const mln_completion first =
    dispatched_completion(queue.handle, reenter, &state, NULL);
  MLN_TEST_OK(mln_map_set_debug_options(state.map, 0, &first, NULL));
  MLN_TEST_OK(mln_test_map_fence(state.map));

  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  MLN_TEST_STATUS(MLN_STATUS_BUSY, state.nested);
  MLN_TEST_STATUS(MLN_STATUS_BUSY, state.concurrent);
  MLN_TEST_OK(state.submitted);
  MLN_TEST_OK(state.fenced);
  TEST_ASSERT_EQUAL_STRING("AL", state.log.kinds);
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY, mln_delivery_queue_dispatch(queue.handle, NULL)
  );
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&state.log.off_thread));

  mln_test_destroy_map(state.map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

static void release_the_queue(
  void* user_data, const mln_completion_result* result
) {
  reentry* state = user_data;
  record(&state->log, result->status == MLN_STATUS_OK ? 'A' : 'F', 0);
  mln_delivery_queue_release(state->queue);
}

static void release_first(void* user_data) {
  reentry* state = user_data;
  record(&state->log, 'a', 0);
}

static void release_second(void* user_data) {
  reentry* state = user_data;
  record(&state->log, 'b', 0);
}

static void record_second(
  void* user_data, const mln_completion_result* result
) {
  reentry* state = user_data;
  record(&state->log, result->status == MLN_STATUS_OK ? 'B' : 'F', 0);
}

// A call that releases its queue finishes, release included, and the calls
// queued after it never run.
static void a_call_that_releases_the_queue_abandons_the_rest(void) {
  static reentry state;
  start_case(&state.log);
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  state.queue = queue.handle;
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_completion first = dispatched_completion(
    queue.handle, release_the_queue, &state, release_first
  );
  const mln_completion second =
    dispatched_completion(queue.handle, record_second, &state, release_second);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &first, NULL));
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &second, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));

  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  TEST_ASSERT_EQUAL_STRING("Aa", state.log.kinds);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wake_releases, 1));
  MLN_TEST_INVALID_STATE(mln_delivery_queue_dispatch(queue.handle, NULL));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&state.log.off_thread));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct disposing_owner {
  call_log log;
  mln_runtime runtime;
  mln_map map;
  mln_status map_disposed;
  mln_status runtime_disposed;
} disposing_owner;

static void dispose_owners(
  void* user_data, const mln_completion_result* result
) {
  disposing_owner* owner = user_data;
  record(&owner->log, result->status == MLN_STATUS_OK ? 'C' : 'F', 0);
  owner->map_disposed = mln_map_dispose(owner->map, NULL);
  owner->runtime_disposed = mln_runtime_dispose(owner->runtime, NULL);
}

static void pass_request(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)request;
  record(user_data, 'P', 0);
  (void)mln_resource_request_pass_through(handle, NULL);
}

// A call may dispose the map and runtime that queued it. The runtime's
// registrations then retire, and their releases run in a later dispatch.
static void a_call_may_dispose_its_owners(void) {
  static disposing_owner owner;
  start_case(&owner.log);
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  owner.runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &owner.runtime, NULL));
  const mln_resource_provider provider = {
    .size = sizeof(provider),
    .callback = pass_request,
    .user_data = &owner.log,
    .release_user_data = release_registration,
    .queue = queue.handle,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(owner.runtime, &provider));
  owner.map = MLN_HANDLE_NULL;
  const mln_map_options map_options = mln_test_map_options();
  MLN_TEST_OK(
    mln_test_map_create_status(owner.runtime, &map_options, &owner.map)
  );
  const mln_completion completion =
    dispatched_completion(queue.handle, dispose_owners, &owner, NULL);
  MLN_TEST_OK(mln_map_set_debug_options(owner.map, 0, &completion, NULL));

  dispatch_until(queue.handle, &owner.log, 'r');
  MLN_TEST_OK(owner.map_disposed);
  MLN_TEST_OK(owner.runtime_disposed);
  TEST_ASSERT_EQUAL_CHAR('C', owner.log.kinds[0]);
  TEST_ASSERT_EQUAL_CHAR('r', owner.log.kinds[owner.log.count - 1]);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&owner.log.off_thread));
  mln_test_delivery_queue_release(&queue);
}

static void record_end(void* user_data, const mln_camera_transition_end* end) {
  record(user_data, 'E', end->generation);
}

// A cancelled ease runs its end handler and then its release in dispatch.
static void a_camera_end_handler_runs_its_end_then_its_release(void) {
  static call_log log;
  start_case(&log);
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 6;
  update.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_TRANSITION_ID;
  update.animation.duration_ms = 60000;
  update.animation.transition_id = 7;
  update.animation.end_handler = (mln_camera_transition_handler){
    .callback = record_end,
    .user_data = &log,
    .release_user_data = release_registration,
    .queue = queue.handle,
  };
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transition(map, 7, &completion.descriptor, NULL)
  );
  dispatch_until(queue.handle, &log, 'r');
  TEST_ASSERT_EQUAL_STRING("Er", log.kinds);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, log.generations[0]);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&log.off_thread));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

MLN_TEST_GROUP {
  RUN_TEST(dispatch_reports_its_status);
  RUN_TEST(events_and_their_completion_run_in_push_order);
  RUN_TEST(a_dispatched_request_passes_its_handle_to_the_call);
  RUN_TEST(releasing_the_queue_abandons_pending_calls_and_releases);
  RUN_TEST(a_dispatch_inside_a_call_is_busy);
  RUN_TEST(a_call_that_releases_the_queue_abandons_the_rest);
  RUN_TEST(a_call_may_dispose_its_owners);
  RUN_TEST(a_camera_end_handler_runs_its_end_then_its_release);
}
