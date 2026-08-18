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
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_request_repaint(first)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("stale"),
    "A released handle should report that it is stale."
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, mln_test_map_close(first));

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_request_repaint(second));

  mln_test_destroy_map(second);
  mln_test_destroy_runtime(runtime);
}

static void a_handle_of_another_kind_is_rejected_by_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_request_repaint(runtime)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("mln_runtime"),
    "A wrong-kind handle should name the kind it actually is."
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("mln_map"),
    "A wrong-kind handle should name the kind that was expected."
  );
  mln_test_destroy_runtime(runtime);
}

static void a_handle_this_process_never_issued_is_rejected(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_request_repaint(MLN_HANDLE_NULL)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    last_error_mentions("null"), "The null handle should report as null."
  );

  // A well-formed map handle whose index is far past anything created.
  const mln_map unissued = (mln_map)0x0200000FFFFFFFFFULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_request_repaint(unissued)
  );

  // A value whose kind byte names no handle type at all.
  const mln_map malformed = (mln_map)0xDEADBEEFDEADBEEFULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_request_repaint(malformed)
  );
}

void run_handles_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(a_released_map_handle_never_names_a_later_map);
  RUN_TEST(a_handle_of_another_kind_is_rejected_by_kind);
  RUN_TEST(a_handle_this_process_never_issued_is_rejected);
}
