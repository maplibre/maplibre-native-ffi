// Deferred callbacks, which answer MapLibre at once and hand a copy of each
// call to a listener.

#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/test_support.h"

static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// A handle the loader never issued. A call that the adapter rejects never
// reads it.
static const mln_resource_request_handle unissued_handle = 1;

static mln_resource_request style_request(void) {
  return (mln_resource_request){
    .size = sizeof(mln_resource_request),
    .requested_url = "maplibre://maps/style",
    .resolved_url = "https://maps.localhost/style.json",
    .kind = MLN_RESOURCE_KIND_STYLE,
    .loading_method = MLN_RESOURCE_LOADING_METHOD_ALL,
    .priority = MLN_RESOURCE_PRIORITY_REGULAR,
    .usage = MLN_RESOURCE_USAGE_ONLINE,
    .storage_policy = MLN_RESOURCE_STORAGE_POLICY_PERMANENT,
  };
}

// Collects what one deferred callback context hands its listener. The listener
// runs on the calling thread, which is a MapLibre thread for registered
// contexts, so its counters are atomic.
typedef struct deferred_probe {
  atomic_size_t records;
  atomic_size_t releases;
  atomic_bool delivered;
  // Keeps the last record for the test thread instead of destroying it.
  bool keep;
  _Atomic(mln_adapter_deferred_call_record*) kept;
} deferred_probe;

static void deferred_listener(
  void* user_data, mln_adapter_deferred_call_record* record
) {
  deferred_probe* probe = user_data;
  if (record == NULL) {
    atomic_fetch_add(&probe->releases, 1);
    return;
  }
  atomic_fetch_add(&probe->records, 1);
  if (probe->keep) {
    atomic_store(&probe->kept, record);
  } else {
    mln_adapter_deferred_call_record_destroy(record);
  }
  mln_test_flag_set(&probe->delivered);
}

static void* deferred_context(uint32_t callback, deferred_probe* probe) {
  void* context = NULL;
  MLN_TEST_OK(mln_adapter_deferred_callback_create(
    callback, deferred_listener, probe, &context, NULL
  ));
  TEST_ASSERT_NOT_NULL(context);
  return context;
}

static mln_log_callback deferred_log_callback(void) {
  void* address =
    mln_adapter_deferred_callback_function(MLN_ADAPTER_DEFERRED_LOG_CALLBACK);
  TEST_ASSERT_NOT_NULL(address);
  mln_log_callback callback = NULL;
  memcpy(&callback, &address, sizeof(callback));
  return callback;
}

static mln_resource_provider_callback deferred_provider_callback(void) {
  void* address = mln_adapter_deferred_callback_function(
    MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK
  );
  TEST_ASSERT_NOT_NULL(address);
  mln_resource_provider_callback callback = NULL;
  memcpy(&callback, &address, sizeof(callback));
  return callback;
}

static void deferred_callbacks_reject_raw_invalid_arguments(void) {
  deferred_probe probe = {0};
  void* context = NULL;
  TEST_ASSERT_NULL(mln_adapter_deferred_callback_function(0));
  MLN_TEST_INVALID(mln_adapter_deferred_callback_create(
    0, deferred_listener, &probe, &context, NULL
  ));
  MLN_TEST_INVALID(mln_adapter_deferred_callback_create(
    MLN_ADAPTER_DEFERRED_LOG_CALLBACK, NULL, &probe, &context, NULL
  ));
  context = &probe;
  MLN_TEST_INVALID(mln_adapter_deferred_callback_create(
    MLN_ADAPTER_DEFERRED_LOG_CALLBACK, deferred_listener, &probe, &context, NULL
  ));
  TEST_ASSERT_EQUAL_PTR(&probe, context);
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&probe.releases));
}

// The log adapter consumes the record at once and delivers a copy that
// outlives the borrowed message.
static void deferred_log_callback_consumes_and_delivers_a_copy(void) {
  deferred_probe probe = {.keep = true};
  void* context = deferred_context(MLN_ADAPTER_DEFERRED_LOG_CALLBACK, &probe);
  char message[] = "borrowed message";

  TEST_ASSERT_EQUAL_UINT32(
    1, deferred_log_callback()(
         context, MLN_LOG_SEVERITY_WARNING, MLN_LOG_EVENT_STYLE, 42, message
       )
  );
  memset(message, 'X', sizeof(message) - 1);

  mln_adapter_deferred_call_record* record = atomic_load(&probe.kept);
  TEST_ASSERT_NOT_NULL(record);
  TEST_ASSERT_EQUAL_UINT32(MLN_ADAPTER_DEFERRED_LOG_CALLBACK, record->callback);
  const mln_adapter_log_callback_arguments* arguments = record->arguments;
  TEST_ASSERT_EQUAL_UINT32(MLN_LOG_SEVERITY_WARNING, arguments->severity);
  TEST_ASSERT_EQUAL_UINT32(MLN_LOG_EVENT_STYLE, arguments->event);
  TEST_ASSERT_EQUAL_INT64(42, arguments->code);
  TEST_ASSERT_EQUAL_STRING("borrowed message", arguments->message);
  mln_adapter_deferred_call_record_destroy(record);

  // A context answers only its own callback typedef, with that callback's
  // failure result.
  const mln_resource_request request = style_request();
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    deferred_provider_callback()(context, &request, unissued_handle)
  );
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.records));

  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
}

// A provider call the adapter cannot copy takes the callback's failure result,
// so native loading keeps the request.
static void deferred_provider_passes_through_an_uncopyable_request(void) {
  deferred_probe probe = {0};
  void* context =
    deferred_context(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, &probe);
  mln_resource_request request = style_request();
  request.prior_data = NULL;
  request.prior_data_size = 4;

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    deferred_provider_callback()(context, &request, unissued_handle)
  );
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&probe.records));
  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
}

static bool wait_for_map_event(
  mln_runtime runtime, uint32_t type, mln_map map, char* message,
  size_t message_capacity
) {
  mln_runtime_event event = {0};
  return mln_test_await_event(
    runtime, type, map, &event, message, message_capacity
  );
}

// A routed deferred provider claims its route, answers HANDLE at once, and
// delivers a record whose adopted handle the host completes later. The style
// load finishes with the host's response.
static void routed_deferred_provider_delivers_a_request_the_host_completes(
  void
) {
  static const char style_url[] = "custom://deferred-provider-style.json";
  deferred_probe probe = {.keep = true};
  void* context =
    deferred_context(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, &probe);
  const mln_adapter_resource_route route = {
    .kind = MLN_RESOURCE_KIND_STYLE,
    .flags = MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL,
    .url = style_url,
  };
  const mln_adapter_routed_resource_provider routed = {
    .routes = &route,
    .route_count = 1,
    .callback = deferred_provider_callback(),
    .user_data = context,
  };
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = mln_adapter_routed_resource_provider_callback,
    .user_data = (void*)&routed,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_adapter_set_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, style_url));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.delivered));

  mln_adapter_deferred_call_record* record = atomic_load(&probe.kept);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, record->callback
  );
  const mln_adapter_resource_provider_callback_arguments* arguments =
    record->arguments;
  TEST_ASSERT_EQUAL_STRING(style_url, arguments->request->requested_url);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, arguments->request->kind);
  const mln_resource_request_handle handle = arguments->handle;
  mln_adapter_deferred_call_record_adopt(record);
  mln_adapter_deferred_call_record_destroy(record);

  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .bytes = inline_style_json,
    .byte_count = sizeof(inline_style_json) - 1,
  };
  MLN_TEST_OK(mln_resource_request_complete(handle, &response, NULL));
  mln_resource_request_release(handle);
  TEST_ASSERT_TRUE(wait_for_map_event(
    runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, map, NULL, 0
  ));

  MLN_TEST_OK(mln_test_adapter_clear_provider(runtime));
  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.records));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A record the host destroys without adopting its handle fails the request,
// so the style load reports an error instead of waiting. The registration
// releases its context once.
static void unadopted_deferred_request_fails_and_releases_once(void) {
  static const char style_url[] = "custom://deferred-provider-dropped.json";
  deferred_probe probe = {0};
  void* context =
    deferred_context(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, &probe);
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = deferred_provider_callback(),
    .user_data = context,
    .release_user_data = mln_adapter_deferred_callback_release,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_adapter_set_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, style_url));
  char message[256] = {0};
  TEST_ASSERT_TRUE(wait_for_map_event(
    runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, message, sizeof(message)
  ));
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.records));

  MLN_TEST_OK(mln_test_adapter_clear_provider(runtime));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
}

MLN_TEST_GROUP {
  RUN_TEST(deferred_callbacks_reject_raw_invalid_arguments);
  RUN_TEST(deferred_log_callback_consumes_and_delivers_a_copy);
  RUN_TEST(deferred_provider_passes_through_an_uncopyable_request);
  RUN_TEST(routed_deferred_provider_delivers_a_request_the_host_completes);
  RUN_TEST(unadopted_deferred_request_fails_and_releases_once);
}
