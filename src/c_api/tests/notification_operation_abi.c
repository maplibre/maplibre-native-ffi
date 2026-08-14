// Raw C ABI coverage for common operations, owned ready batches, drain leases,
// and receiver-scoped level-triggered notification sources.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "abi_tests.h"
#include "maplibre_native_c/callback_adapter.h"
#include "test_support.h"
#include "unity.h"

typedef struct callback_probe {
  atomic_uint calls;
  atomic_bool entered;
  atomic_bool release;
} callback_probe;

static void count_notification(void* user_data) {
  callback_probe* probe = user_data;
  atomic_fetch_add(&probe->calls, 1);
}

static void block_notification(void* user_data) {
  callback_probe* probe = user_data;
  atomic_fetch_add(&probe->calls, 1);
  atomic_store(&probe->entered, true);
  while (!atomic_load(&probe->release)) {
  }
}

static void initialize_callback_probe(callback_probe* probe) {
  atomic_init(&probe->calls, 0);
  atomic_init(&probe->entered, false);
  atomic_init(&probe->release, false);
}

static bool ready_view_contains(
  const mln_ready_batch_view* view, uint32_t kind, uint64_t id
) {
  for (size_t index = 0; index < view->endpoint_count; index += 1) {
    const mln_ready_endpoint* endpoint =
      (const mln_ready_endpoint*)((const char*)view->endpoints +
                                  index * view->endpoint_size);
    if (endpoint->kind == kind && endpoint->id == id) {
      return true;
    }
  }
  return false;
}

static mln_ready_batch drain_ready(
  mln_notification_source source, mln_ready_batch_view* out_view
) {
  mln_ready_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_notification_source_drain_ready(source, &batch)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, batch);
  *out_view = (mln_ready_batch_view){.size = sizeof(mln_ready_batch_view)};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_ready_batch_get(batch, out_view));
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_ready_endpoint), out_view->endpoint_size);
  return batch;
}

static void completion_notifies_before_and_after_callback_registration(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));

  mln_operation first = MLN_HANDLE_NULL;
  mln_test_operation_control* first_control = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_operation_create(source, false, &first, &first_control)
  );
  mln_test_operation_complete(first_control, MLN_STATUS_OK, "first");

  bool completed = false;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_operation_poll(first, &completed));
  TEST_ASSERT_TRUE(completed);

  callback_probe probe;
  initialize_callback_probe(&probe);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_notification_source_set_callback(source, count_notification, &probe)
  );
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&probe.calls));

  mln_ready_batch_view view;
  mln_ready_batch batch = drain_ready(source, &view);
  TEST_ASSERT_EQUAL_size_t(1, view.endpoint_count);
  TEST_ASSERT_TRUE(
    ready_view_contains(&view, MLN_NOTIFICATION_ENDPOINT_OPERATION, first)
  );
  mln_ready_batch_release(batch);

  mln_operation second = MLN_HANDLE_NULL;
  mln_test_operation_control* second_control = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_operation_create(source, false, &second, &second_control)
  );
  mln_test_operation_complete(second_control, MLN_STATUS_OK, "second");
  TEST_ASSERT_EQUAL_UINT(2, atomic_load(&probe.calls));

  batch = drain_ready(source, &view);
  TEST_ASSERT_EQUAL_size_t(1, view.endpoint_count);
  TEST_ASSERT_TRUE(
    ready_view_contains(&view, MLN_NOTIFICATION_ENDPOINT_OPERATION, second)
  );
  mln_ready_batch_release(batch);

  mln_operation_release(second);
  mln_operation_release(first);
  mln_test_operation_control_destroy(second_control);
  mln_test_operation_control_destroy(first_control);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
}

typedef struct operation_race_probe {
  mln_operation operation;
  mln_test_operation_control* control;
  atomic_bool start;
  mln_status cancel_status;
} operation_race_probe;

static void cancel_operation_after_start(void* argument) {
  operation_race_probe* probe = argument;
  while (!atomic_load(&probe->start)) {
  }
  probe->cancel_status = mln_operation_cancel(probe->operation);
}

static void complete_operation_after_start(void* argument) {
  operation_race_probe* probe = argument;
  while (!atomic_load(&probe->start)) {
  }
  mln_test_operation_complete(
    probe->control, MLN_STATUS_NATIVE_ERROR, "concurrent completion"
  );
}

static void polling_waiting_and_cancellation_are_race_safe(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_operation operation = MLN_HANDLE_NULL;
  mln_test_operation_control* control = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_operation_create(source, true, &operation, &control)
  );

  bool completed = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_poll(operation, &completed)
  );
  TEST_ASSERT_FALSE(completed);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_wait(operation, 0, &completed)
  );
  TEST_ASSERT_FALSE(completed);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_wait(operation, 2, &completed)
  );
  TEST_ASSERT_FALSE(completed);

  operation_race_probe probe = {
    .operation = operation,
    .control = control,
    .cancel_status = MLN_STATUS_INVALID_STATE,
  };
  atomic_init(&probe.start, false);
  mln_test_thread* cancel_thread =
    mln_test_thread_start(cancel_operation_after_start, &probe);
  mln_test_thread* complete_thread =
    mln_test_thread_start(complete_operation_after_start, &probe);
  atomic_store(&probe.start, true);
  mln_test_thread_join(cancel_thread);
  mln_test_thread_join(complete_thread);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_wait(operation, -1, &completed)
  );
  TEST_ASSERT_TRUE(completed);
  TEST_ASSERT_TRUE(
    probe.cancel_status == MLN_STATUS_OK ||
    probe.cancel_status == MLN_STATUS_INVALID_STATE
  );
  mln_status terminal_status = MLN_STATUS_OK;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_get_status(operation, &terminal_status)
  );
  TEST_ASSERT_TRUE(
    terminal_status == MLN_STATUS_CANCELLED ||
    terminal_status == MLN_STATUS_NATIVE_ERROR
  );
  TEST_ASSERT_EQUAL_UINT(
    probe.cancel_status == MLN_STATUS_OK ? 1 : 0,
    mln_test_operation_cancel_count(control)
  );

  mln_operation_release(operation);
  mln_test_operation_control_destroy(control);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
}

typedef struct observer_race_probe {
  mln_operation operation;
  mln_test_operation_control* control;
  atomic_bool start;
} observer_race_probe;

static void release_observer_after_start(void* argument) {
  observer_race_probe* probe = argument;
  while (!atomic_load(&probe->start)) {
  }
  mln_operation_release(probe->operation);
}

static void complete_unobserved_after_start(void* argument) {
  observer_race_probe* probe = argument;
  while (!atomic_load(&probe->start)) {
  }
  mln_test_operation_complete(probe->control, MLN_STATUS_OK, "late result");
}

static void an_uncancellable_pending_operation_can_lose_its_observer(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_operation operation = MLN_HANDLE_NULL;
  mln_test_operation_control* control = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_operation_create(source, false, &operation, &control)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED, mln_operation_cancel(operation)
  );

  observer_race_probe probe = {.operation = operation, .control = control};
  atomic_init(&probe.start, false);
  mln_test_thread* release_thread =
    mln_test_thread_start(release_observer_after_start, &probe);
  mln_test_thread* completion_thread =
    mln_test_thread_start(complete_unobserved_after_start, &probe);
  atomic_store(&probe.start, true);
  mln_test_thread_join(release_thread);
  mln_test_thread_join(completion_thread);

  bool completed = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_operation_poll(operation, &completed)
  );
  mln_ready_batch_view view;
  const mln_ready_batch batch = drain_ready(source, &view);
  TEST_ASSERT_EQUAL_size_t(0, view.endpoint_count);
  mln_ready_batch_release(batch);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
  mln_test_operation_control_destroy(control);
}

typedef struct release_cancel_probe {
  mln_operation operation;
  mln_status status;
} release_cancel_probe;

static void cancel_released_operation(void* argument) {
  release_cancel_probe* probe = argument;
  probe->status = mln_operation_cancel(probe->operation);
}

static void observer_release_detaches_an_endpoint_copied_by_cancellation(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_operation operation = MLN_HANDLE_NULL;
  mln_test_operation_control* control = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_operation_create(source, true, &operation, &control)
  );
  atomic_bool cancel_entered;
  atomic_bool release_cancel;
  atomic_init(&cancel_entered, false);
  atomic_init(&release_cancel, false);
  mln_test_operation_block_cancel(control, &cancel_entered, &release_cancel);

  release_cancel_probe probe = {
    .operation = operation,
    .status = MLN_STATUS_INVALID_STATE,
  };
  mln_test_thread* cancel_thread =
    mln_test_thread_start(cancel_released_operation, &probe);
  while (!atomic_load(&cancel_entered)) {
  }
  mln_operation_release(operation);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
  atomic_store(&release_cancel, true);
  mln_test_thread_join(cancel_thread);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.status);
  mln_test_operation_control_destroy(control);
}

static void owned_ready_batches_preserve_receiver_boundaries(void) {
  mln_notification_source runtime_source = MLN_HANDLE_NULL;
  mln_notification_source driver_source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_notification_source_create(&runtime_source)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_notification_source_create(&driver_source)
  );

  const uint64_t runtime_id = UINT64_C(0x1001);
  const uint64_t operation_id = UINT64_C(0x1002);
  const uint64_t driver_id = UINT64_C(0x2001);
  mln_test_endpoint_control* runtime_endpoint = NULL;
  mln_test_endpoint_control* operation_endpoint = NULL;
  mln_test_endpoint_control* driver_endpoint = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_endpoint_create(
      runtime_source, runtime_id, MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS,
      true, &runtime_endpoint
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_endpoint_create(
      runtime_source, operation_id, MLN_NOTIFICATION_ENDPOINT_OPERATION, false,
      &operation_endpoint
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_endpoint_create(
      driver_source, driver_id, MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK, true,
      &driver_endpoint
    )
  );

  mln_test_endpoint_mark_ready(runtime_endpoint);
  mln_test_endpoint_mark_ready(operation_endpoint);
  mln_test_endpoint_mark_ready(driver_endpoint);

  mln_ready_batch_view first_view;
  mln_ready_batch first = drain_ready(runtime_source, &first_view);
  TEST_ASSERT_EQUAL_size_t(2, first_view.endpoint_count);
  TEST_ASSERT_TRUE(ready_view_contains(
    &first_view, MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, runtime_id
  ));
  TEST_ASSERT_TRUE(ready_view_contains(
    &first_view, MLN_NOTIFICATION_ENDPOINT_OPERATION, operation_id
  ));
  TEST_ASSERT_FALSE(ready_view_contains(
    &first_view, MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK, driver_id
  ));
  const mln_ready_endpoint first_copy = first_view.endpoints[0];

  mln_ready_batch_view second_view;
  mln_ready_batch second = drain_ready(runtime_source, &second_view);
  TEST_ASSERT_EQUAL_size_t(1, second_view.endpoint_count);
  TEST_ASSERT_TRUE(ready_view_contains(
    &second_view, MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, runtime_id
  ));
  TEST_ASSERT_EQUAL_size_t(2, first_view.endpoint_count);
  TEST_ASSERT_EQUAL_UINT32(first_copy.kind, first_view.endpoints[0].kind);
  TEST_ASSERT_EQUAL_UINT64(first_copy.id, first_view.endpoints[0].id);

  mln_ready_batch_view driver_view;
  mln_ready_batch driver = drain_ready(driver_source, &driver_view);
  TEST_ASSERT_EQUAL_size_t(1, driver_view.endpoint_count);
  TEST_ASSERT_TRUE(ready_view_contains(
    &driver_view, MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK, driver_id
  ));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_notification_source_close(runtime_source)
  );
  mln_test_endpoint_clear_ready(runtime_endpoint);
  mln_test_endpoint_clear_ready(driver_endpoint);
  mln_test_endpoint_control_destroy(operation_endpoint);
  mln_test_endpoint_control_destroy(runtime_endpoint);
  mln_test_endpoint_control_destroy(driver_endpoint);

  mln_ready_batch_release(driver);
  mln_ready_batch_release(second);
  mln_ready_batch_release(first);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_notification_source_close(runtime_source)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_notification_source_close(driver_source)
  );
}

static void a_new_ready_endpoint_notifies_while_another_remains_ready(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_test_endpoint_control* events = NULL;
  mln_test_endpoint_control* operation = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_endpoint_create(
                     source, UINT64_C(0x2101),
                     MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, true, &events
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_endpoint_create(
                     source, UINT64_C(0x2102),
                     MLN_NOTIFICATION_ENDPOINT_OPERATION, false, &operation
                   )
  );
  callback_probe probe;
  initialize_callback_probe(&probe);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_notification_source_set_callback(source, count_notification, &probe)
  );

  mln_test_endpoint_mark_ready(events);
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&probe.calls));
  mln_ready_batch_view view;
  mln_ready_batch first = drain_ready(source, &view);
  TEST_ASSERT_TRUE(ready_view_contains(
    &view, MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, UINT64_C(0x2101)
  ));
  mln_ready_batch_release(first);

  mln_test_endpoint_mark_ready(operation);
  TEST_ASSERT_EQUAL_UINT(2, atomic_load(&probe.calls));
  mln_ready_batch second = drain_ready(source, &view);
  TEST_ASSERT_TRUE(ready_view_contains(
    &view, MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, UINT64_C(0x2101)
  ));
  TEST_ASSERT_TRUE(ready_view_contains(
    &view, MLN_NOTIFICATION_ENDPOINT_OPERATION, UINT64_C(0x2102)
  ));
  mln_ready_batch_release(second);

  mln_test_endpoint_clear_ready(events);
  mln_test_endpoint_control_destroy(operation);
  mln_test_endpoint_control_destroy(events);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
}

typedef struct endpoint_thread_probe {
  mln_test_endpoint_control* endpoint;
} endpoint_thread_probe;

static void mark_endpoint_on_thread(void* argument) {
  endpoint_thread_probe* probe = argument;
  mln_test_endpoint_mark_ready(probe->endpoint);
}

static void an_empty_drain_cannot_lose_the_next_notification(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_test_endpoint_control* endpoint = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_endpoint_create(
                     source, UINT64_C(0x3001),
                     MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, true, &endpoint
                   )
  );
  callback_probe probe;
  initialize_callback_probe(&probe);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_notification_source_set_callback(source, count_notification, &probe)
  );

  mln_ready_batch_view empty_view;
  mln_ready_batch empty = drain_ready(source, &empty_view);
  TEST_ASSERT_EQUAL_size_t(0, empty_view.endpoint_count);
  mln_ready_batch_release(empty);

  endpoint_thread_probe thread_probe = {.endpoint = endpoint};
  mln_test_thread* thread =
    mln_test_thread_start(mark_endpoint_on_thread, &thread_probe);
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&probe.calls));

  mln_ready_batch_view ready_view;
  mln_ready_batch ready = drain_ready(source, &ready_view);
  TEST_ASSERT_EQUAL_size_t(1, ready_view.endpoint_count);
  mln_ready_batch_release(ready);
  mln_test_endpoint_clear_ready(endpoint);

  mln_ready_batch drained = drain_ready(source, &empty_view);
  TEST_ASSERT_EQUAL_size_t(0, empty_view.endpoint_count);
  mln_ready_batch_release(drained);
  thread = mln_test_thread_start(mark_endpoint_on_thread, &thread_probe);
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_UINT(2, atomic_load(&probe.calls));

  mln_test_endpoint_control_destroy(endpoint);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
}

typedef struct callback_update_probe {
  mln_notification_source source;
  callback_probe* replacement;
  atomic_bool finished;
  mln_status status;
} callback_update_probe;

static void replace_callback_on_thread(void* argument) {
  callback_update_probe* probe = argument;
  probe->status = mln_notification_source_set_callback(
    probe->source, count_notification, probe->replacement
  );
  atomic_store(&probe->finished, true);
}

static void callback_replacement_waits_for_an_inflight_entry(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_test_endpoint_control* endpoint = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_endpoint_create(
                     source, UINT64_C(0x4001),
                     MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, true, &endpoint
                   )
  );
  callback_probe blocking;
  callback_probe replacement;
  initialize_callback_probe(&blocking);
  initialize_callback_probe(&replacement);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_notification_source_set_callback(source, block_notification, &blocking)
  );

  endpoint_thread_probe endpoint_probe = {.endpoint = endpoint};
  mln_test_thread* publisher =
    mln_test_thread_start(mark_endpoint_on_thread, &endpoint_probe);
  while (!atomic_load(&blocking.entered)) {
  }

  callback_update_probe update = {
    .source = source,
    .replacement = &replacement,
    .status = MLN_STATUS_INVALID_STATE,
  };
  atomic_init(&update.finished, false);
  mln_test_thread* replacer =
    mln_test_thread_start(replace_callback_on_thread, &update);
  mln_test_sleep_milliseconds(5);
  TEST_ASSERT_FALSE(atomic_load(&update.finished));
  atomic_store(&blocking.release, true);
  mln_test_thread_join(publisher);
  mln_test_thread_join(replacer);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, update.status);
  TEST_ASSERT_TRUE(atomic_load(&update.finished));
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&replacement.calls));
  mln_test_endpoint_control_destroy(endpoint);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
}

typedef struct drain_lease_probe {
  mln_notification_source source;
  atomic_bool entered;
  atomic_bool release;
} drain_lease_probe;

static void hold_ready_drain_on_thread(void* argument) {
  drain_lease_probe* probe = argument;
  mln_test_hold_notification_ready_drain(
    probe->source, &probe->entered, &probe->release
  );
}

static void a_second_concurrent_ready_drain_is_rejected(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  drain_lease_probe probe = {.source = source};
  atomic_init(&probe.entered, false);
  atomic_init(&probe.release, false);
  mln_test_thread* holder =
    mln_test_thread_start(hold_ready_drain_on_thread, &probe);
  while (!atomic_load(&probe.entered)) {
  }

  mln_ready_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_notification_source_drain_ready(source, &batch)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, batch);
  atomic_store(&probe.release, true);
  mln_test_thread_join(holder);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_notification_source_drain_ready(source, &batch)
  );
  mln_ready_batch_release(batch);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
}

static void adapter_records_and_runtime_events_share_one_source(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = ":memory:";
  options.notification_source = source;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_runtime_create(&options, &runtime)
  );
  mln_adapter_resource_request_queue queue = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_resource_request_queue_create(source, &queue)
  );

  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_request_repaint(map));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  const mln_adapter_queued_resource_provider_route route = {
    .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
    .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
    .url = "**",
  };
  mln_adapter_queued_resource_provider provider = {
    .routes = &route,
    .route_count = 1,
    .queue = queue,
  };
  const mln_resource_request request = {
    .size = sizeof(mln_resource_request),
    .requested_url = "maplibre://style",
    .resolved_url = "https://example.test/style.json",
    .kind = MLN_RESOURCE_KIND_STYLE,
    .loading_method = MLN_RESOURCE_LOADING_METHOD_ALL,
    .priority = MLN_RESOURCE_PRIORITY_REGULAR,
    .usage = MLN_RESOURCE_USAGE_ONLINE,
    .storage_policy = MLN_RESOURCE_STORAGE_POLICY_PERMANENT,
  };
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_HANDLE,
    mln_adapter_queued_resource_provider_callback(&provider, &request, 1)
  );

  mln_ready_batch_view view;
  mln_ready_batch ready = drain_ready(source, &view);
  TEST_ASSERT_TRUE(ready_view_contains(
    &view, MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, runtime
  ));
  TEST_ASSERT_TRUE(ready_view_contains(
    &view, MLN_NOTIFICATION_ENDPOINT_ADAPTER_RESOURCE_REQUESTS, queue
  ));
  mln_ready_batch_release(ready);

  mln_event_batch events = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &events)
  );
  mln_event_batch_release(events);
  mln_adapter_queued_resource_request* record = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_resource_request_queue_acquire(queue, &record)
  );
  TEST_ASSERT_NOT_NULL(record);
  mln_adapter_resource_provider_request_destroy(record);

  ready = drain_ready(source, &view);
  TEST_ASSERT_EQUAL_size_t(0, view.endpoint_count);
  mln_ready_batch_release(ready);

  mln_test_destroy_map(map);
  mln_adapter_resource_request_queue_close(queue);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_close(runtime));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_close(source));
}

void run_notification_operation_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(completion_notifies_before_and_after_callback_registration);
  RUN_TEST(polling_waiting_and_cancellation_are_race_safe);
  RUN_TEST(an_uncancellable_pending_operation_can_lose_its_observer);
  RUN_TEST(observer_release_detaches_an_endpoint_copied_by_cancellation);
  RUN_TEST(owned_ready_batches_preserve_receiver_boundaries);
  RUN_TEST(a_new_ready_endpoint_notifies_while_another_remains_ready);
  RUN_TEST(an_empty_drain_cannot_lose_the_next_notification);
  RUN_TEST(callback_replacement_waits_for_an_inflight_entry);
  RUN_TEST(a_second_concurrent_ready_drain_is_rejected);
  RUN_TEST(adapter_records_and_runtime_events_share_one_source);
}
