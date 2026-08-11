#include <stdint.h>
#include <string.h>

#include "abi_tests.h"
#include "maplibre_native_c.h"
#include "test_support.h"
#include "unity.h"

// Matches a substring so tests do not depend on the exact diagnostic wording.
static bool last_error_mentions(const char* fragment) {
  const char* message = mln_thread_last_error_message();
  return message != NULL && strstr(message, fragment) != NULL;
}

static void a_released_map_handle_never_names_a_later_map(void) {
  mln_runtime runtime = mln_test_create_runtime();

  mln_map first = mln_test_create_map(runtime);
  mln_test_destroy_map(first);

  // Creating again reuses the slot the destroy freed, so the rejection below
  // comes from the generation rather than from an empty slot.
  mln_map second = mln_test_create_map(runtime);
  TEST_ASSERT_NOT_EQUAL_UINT64(first, second);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_repaint(first)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("stale"),
    "A released handle should report that it is stale."
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, mln_map_destroy(first));

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_request_repaint(second));

  mln_test_destroy_map(second);
  mln_test_destroy_runtime(runtime);
}

static void a_handle_of_another_kind_is_rejected_by_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_wake_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_repaint(source)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("mln_wake_source"),
    "A wrong-kind handle should name the kind it actually is."
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("mln_map"),
    "A wrong-kind handle should name the kind that was expected."
  );

  mln_wake_source_destroy(source);
  mln_test_destroy_runtime(runtime);
}

static void a_handle_this_process_never_issued_is_rejected(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_repaint(MLN_HANDLE_NULL)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("null"), "The null handle should report as null."
  );

  // A well-formed map handle whose index is far past anything created.
  const mln_map unissued = (mln_map)0x0200000FFFFFFFFFULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_repaint(unissued)
  );

  // A value whose kind byte names no handle type at all.
  const mln_map malformed = (mln_map)0xDEADBEEFDEADBEEFULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_repaint(malformed)
  );
}

static void releasing_a_scoped_handle_twice_is_a_no_op(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_style_id_list layers = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_list_style_layer_ids(map, &layers)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, layers);

  mln_style_id_list_destroy(layers);
  mln_style_id_list_destroy(layers);
  mln_style_id_list_destroy(MLN_HANDLE_NULL);

  size_t count = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_style_id_list_count(layers, &count)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_runtime_sourced_event_names_its_runtime_handle(void) {
  mln_runtime runtime = mln_test_create_runtime();

  mln_offline_operation_id operation = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_offline_regions_list_start(runtime, &operation)
  );

  bool saw_completion = false;
  for (int attempt = 0; attempt < 200 && !saw_completion; ++attempt) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 50));
    mln_runtime_event_batch batch = mln_runtime_event_batch_default();
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
    );
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      if (event->source_type != MLN_RUNTIME_EVENT_SOURCE_RUNTIME) {
        continue;
      }
      TEST_ASSERT_EQUAL_UINT64_MESSAGE(
        runtime, event->source,
        "A runtime-sourced event should carry the mln_runtime handle."
      );
      if (event->type == MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED) {
        saw_completion = true;
      }
    }
  }
  TEST_ASSERT_TRUE_MESSAGE(
    saw_completion, "The offline list operation should report completion."
  );

  mln_test_destroy_runtime(runtime);
}

void run_handles_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(a_released_map_handle_never_names_a_later_map);
  RUN_TEST(a_handle_of_another_kind_is_rejected_by_kind);
  RUN_TEST(a_handle_this_process_never_issued_is_rejected);
  RUN_TEST(releasing_a_scoped_handle_twice_is_a_no_op);
  RUN_TEST(a_runtime_sourced_event_names_its_runtime_handle);
}
