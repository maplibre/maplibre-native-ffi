// Adapter completions: descriptor validation, the failure channel, ownership
// of an owned result through adoption or destruction, and rejection.

#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/test_support.h"

// The arguments of one mln_adapter_completion_create() call.
typedef struct create_arguments {
  uint32_t copy_kind;
  mln_adapter_completion_listener listener;
  mln_completion output;
  bool null_output;
} create_arguments;

static void with_unknown_copy_kind(void* descriptor) {
  ((create_arguments*)descriptor)->copy_kind = 7;
}

static void without_listener(void* descriptor) {
  ((create_arguments*)descriptor)->listener = NULL;
}

static void with_null_output(void* descriptor) {
  ((create_arguments*)descriptor)->null_output = true;
}

static void with_filled_output(void* descriptor) {
  ((create_arguments*)descriptor)->output.size = sizeof(mln_completion);
}

// Creates the descriptor, then rejects an accepted one so that no row leaks.
static mln_status create_completion(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  mln_test_adapter_delivery* delivery = context;
  create_arguments arguments = *(const create_arguments*)descriptor;
  const mln_status status = mln_adapter_completion_create(
    arguments.copy_kind, arguments.listener, delivery,
    arguments.null_output ? NULL : &arguments.output, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    mln_adapter_completion_reject(&arguments.output);
  }
  return status;
}

static void completion_create_rejects_raw_invalid_arguments(void) {
  static const mln_test_validation_case cases[] = {
    {"valid", NULL, MLN_STATUS_OK, NULL},
    {"unknown copy kind", with_unknown_copy_kind, MLN_STATUS_INVALID_ARGUMENT,
     "invalid"},
    {"no listener", without_listener, MLN_STATUS_INVALID_ARGUMENT, "invalid"},
    {"null output", with_null_output, MLN_STATUS_INVALID_ARGUMENT, "invalid"},
    {"filled output", with_filled_output, MLN_STATUS_INVALID_ARGUMENT,
     "invalid"},
  };
  const create_arguments defaults = {
    .copy_kind = MLN_ADAPTER_COMPLETION_COPY_MAP,
    .listener = mln_test_adapter_keep_record,
  };
  mln_test_adapter_delivery delivery = {0};
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    create_completion, &delivery
  );
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&delivery.deliveries));
}

static void completion_copy_failures_use_the_adapter_failure_channel(void) {
  mln_test_adapter_delivery delivery = {0};
  mln_completion completion = mln_test_adapter_completion(
    MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK_RESULT, &delivery
  );

  // A texture readback completion must carry exactly one result. The invalid
  // shape deterministically exercises the adapter's copy-failure path.
  const mln_texture_readback_result value = {0};
  const mln_completion_result result = {
    .size = sizeof(mln_completion_result),
    .status = MLN_STATUS_OK,
    .value = &value,
    .value_count = 2,
  };
  completion.callback(completion.user_data, &result);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&delivery.deliveries));
  TEST_ASSERT_NULL(atomic_load(&delivery.record));
  completion.release_user_data(completion.user_data);
}

// Creates a map through an adapter completion and returns its record, which
// owns the map until the host adopts it.
static mln_adapter_completion_record* create_map_record(
  mln_runtime runtime, mln_test_adapter_delivery* delivery, mln_map* out_map
) {
  mln_completion completion =
    mln_test_adapter_completion(MLN_ADAPTER_COMPLETION_COPY_MAP, delivery);
  MLN_TEST_OK(mln_map_create(runtime, NULL, &completion, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&delivery->delivered));
  mln_adapter_completion_record* record = atomic_load(&delivery->record);
  TEST_ASSERT_NOT_NULL(record);
  MLN_TEST_OK(record->result.status);
  TEST_ASSERT_EQUAL_size_t(1, record->result.value_count);
  *out_map = *(const mln_map*)record->result.value;
  TEST_ASSERT_TRUE(mln_test_adapter_map_is_live(*out_map));
  return record;
}

// A host that fails to construct its result owner destroys the record, and
// the record disposes the map it still owns.
static void destroying_an_unadopted_record_disposes_its_owned_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_adapter_delivery delivery = {0};
  mln_map map = MLN_HANDLE_NULL;
  mln_adapter_completion_record* record =
    create_map_record(runtime, &delivery, &map);

  mln_adapter_completion_record_destroy(record);
  TEST_ASSERT_FALSE(mln_test_adapter_map_is_live(map));
  mln_test_destroy_runtime(runtime);
}

// Adopting transfers the map to the host, so destroying the record afterwards
// leaves the map live.
static void an_adopted_record_leaves_its_result_to_the_host(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_adapter_delivery delivery = {0};
  mln_map map = MLN_HANDLE_NULL;
  mln_adapter_completion_record* record =
    create_map_record(runtime, &delivery, &map);

  mln_adapter_completion_record_adopt(record);
  mln_adapter_completion_record_destroy(record);
  TEST_ASSERT_TRUE(mln_test_adapter_map_is_live(map));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A submission that the C API rejects never reaches the listener. Rejection is
// synchronous, so the listener count is final when the reject returns.
static void a_rejected_completion_never_reaches_its_listener(void) {
  mln_test_adapter_delivery delivery = {0};
  mln_completion completion =
    mln_test_adapter_completion(MLN_ADAPTER_COMPLETION_COPY_MAP, &delivery);
  MLN_TEST_INVALID(mln_map_create(MLN_HANDLE_NULL, NULL, &completion, NULL));
  mln_adapter_completion_reject(&completion);
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&delivery.deliveries));
  mln_adapter_completion_reject(NULL);
}

MLN_TEST_GROUP {
  RUN_TEST(completion_create_rejects_raw_invalid_arguments);
  RUN_TEST(completion_copy_failures_use_the_adapter_failure_channel);
  RUN_TEST(destroying_an_unadopted_record_disposes_its_owned_result);
  RUN_TEST(an_adopted_record_leaves_its_result_to_the_host);
  RUN_TEST(a_rejected_completion_never_reaches_its_listener);
}
