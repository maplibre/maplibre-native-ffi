// MapLibre's process-global network status. Every case here changes it, so
// each runs inside restoring_network_status(), which sets it back online even
// when an assertion fails.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void restoring_network_status(void (*body)(void)) {
  if (TEST_PROTECT()) {
    body();
  }
  (void)mln_network_status_set(MLN_NETWORK_STATUS_ONLINE, NULL);
}

#define NETWORK_STATUS_CASE(name)                                   \
  static void name##_body(void);                                    \
  static void name(void) { restoring_network_status(name##_body); } \
  static void name##_body(void)

static uint32_t current_network_status(void) {
  uint32_t status = 0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_network_status_get(&status, NULL));
  return status;
}

NETWORK_STATUS_CASE(the_network_status_round_trips) {
  TEST_ASSERT_EQUAL_UINT32(MLN_NETWORK_STATUS_ONLINE, current_network_status());
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_OFFLINE, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NETWORK_STATUS_OFFLINE, current_network_status()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_OFFLINE, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NETWORK_STATUS_OFFLINE, current_network_status()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_ONLINE, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_NETWORK_STATUS_ONLINE, current_network_status());
}

static const uint32_t unknown_statuses[] = {0, 3, UINT32_MAX};

// A value that names no status is rejected and leaves the status as it was.
NETWORK_STATUS_CASE(the_network_status_rejects_a_value_that_names_no_status) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_set(MLN_NETWORK_STATUS_OFFLINE, NULL)
  );
  for (size_t index = 0;
       index < sizeof(unknown_statuses) / sizeof(*unknown_statuses);
       index += 1) {
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_network_status_set(unknown_statuses[index], &diagnostic)
    );
    TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "network status"));
    TEST_ASSERT_EQUAL_UINT32(
      MLN_NETWORK_STATUS_OFFLINE, current_network_status()
    );
  }

  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_network_status_get(NULL, &diagnostic)
  );
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "out_status"));
}

MLN_TEST_GROUP {
  RUN_TEST(the_network_status_round_trips);
  RUN_TEST(the_network_status_rejects_a_value_that_names_no_status);
}
