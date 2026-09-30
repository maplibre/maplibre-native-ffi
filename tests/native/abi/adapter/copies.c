// Adapter completion copies. The generated table synthesizes one result for
// each copy kind, with every pointer set and every array of two elements. Each
// result passes through an adapter completion, and the delivered record must
// hold equal content in storage of its own.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "adapter_copy_cases_generated.inc"
#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

// Passes one result through an adapter completion and returns the record it
// delivered, or null for the failure channel.
static mln_adapter_completion_record* copy_result(
  uint32_t copy_kind, size_t element_size, const mln_completion_result* result,
  mln_test_adapter_delivery* delivery
) {
  mln_completion completion =
    mln_test_adapter_completion(copy_kind, element_size, delivery);
  completion.callback(completion.user_data, result);
  completion.release_user_data(completion.user_data);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&delivery->deliveries));
  return atomic_load(&delivery->record);
}

static void every_copy_kind_copies_its_result_into_storage_of_its_own(void) {
  static const char diagnostic[] = "copied diagnostic";
  for (size_t index = 0; index < sizeof(mln_adapter_copy_cases) /
                                   sizeof(mln_adapter_copy_cases[0]);
       ++index) {
    const mln_adapter_copy_case* entry = &mln_adapter_copy_cases[index];
    size_t count = 0;
    const void* source = entry->value(&count);
    const mln_completion_result result = {
      .size = sizeof(mln_completion_result),
      .status = MLN_STATUS_OK,
      .generation = 5,
      .diagnostic = MLN_BUFFER_LITERAL(diagnostic),
      .value = source,
      .value_count = count,
    };
    mln_test_adapter_delivery delivery = {0};
    mln_adapter_completion_record* record =
      copy_result(entry->kind, entry->element_size, &result, &delivery);

    TEST_ASSERT_NOT_NULL_MESSAGE(record, entry->type);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, record->result.status, entry->type
    );
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(5, record->result.generation, entry->type);
    TEST_ASSERT_TRUE_MESSAGE(
      mln_adapter_copy_case_view(record->result.diagnostic, result.diagnostic),
      entry->type
    );
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      count, record->result.value_count, entry->type
    );
    TEST_ASSERT_TRUE_MESSAGE(record->result.value != source, entry->type);
    TEST_ASSERT_TRUE_MESSAGE(
      entry->matches(record->result.value, source, count), entry->type
    );
    // The synthesized handles were never issued, so the record must not
    // dispose them.
    mln_adapter_completion_record_adopt(record);
    mln_adapter_completion_record_destroy(record);
  }
}

// A flat copy takes element_size bytes per value, and cannot copy values of
// unknown size.
static void flat_copies_take_element_size_bytes_per_value(void) {
  typedef struct triple {
    uint32_t first;
    uint32_t second;
    uint32_t third;
  } triple;
  const triple values[] = {{1, 2, 3}, {4, 5, 6}};
  const mln_completion_result result = {
    .size = sizeof(mln_completion_result),
    .status = MLN_STATUS_OK,
    .value = values,
    .value_count = 2,
  };

  mln_test_adapter_delivery delivery = {0};
  mln_adapter_completion_record* record = copy_result(
    MLN_ADAPTER_COMPLETION_COPY_FLAT, sizeof(triple), &result, &delivery
  );
  TEST_ASSERT_NOT_NULL(record);
  TEST_ASSERT_EQUAL_size_t(2, record->result.value_count);
  TEST_ASSERT_TRUE(record->result.value != (const void*)values);
  TEST_ASSERT_EQUAL_MEMORY(values, record->result.value, sizeof(values));
  mln_adapter_completion_record_destroy(record);

  mln_test_adapter_delivery unsized = {0};
  TEST_ASSERT_NULL(
    copy_result(MLN_ADAPTER_COMPLETION_COPY_FLAT, 0, &result, &unsized)
  );
}

// A failed result copies its diagnostic and carries no value, whatever value
// the borrowed result pointed to.
static void a_failed_result_copies_its_diagnostic_and_no_value(void) {
  static const char diagnostic[] = "native failure";
  const mln_map map = 1;
  const mln_completion_result result = {
    .size = sizeof(mln_completion_result),
    .status = MLN_STATUS_NATIVE_ERROR,
    .diagnostic = MLN_BUFFER_LITERAL(diagnostic),
    .value = &map,
    .value_count = 1,
  };
  mln_test_adapter_delivery delivery = {0};
  mln_adapter_completion_record* record = copy_result(
    MLN_ADAPTER_COMPLETION_COPY_MAP, sizeof(mln_map), &result, &delivery
  );
  TEST_ASSERT_NOT_NULL(record);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_NATIVE_ERROR, record->result.status);
  TEST_ASSERT_TRUE(
    mln_adapter_copy_case_view(record->result.diagnostic, result.diagnostic)
  );
  TEST_ASSERT_NULL(record->result.value);
  TEST_ASSERT_EQUAL_size_t(0, record->result.value_count);
  mln_adapter_completion_record_destroy(record);
}

MLN_TEST_GROUP {
  RUN_TEST(every_copy_kind_copies_its_result_into_storage_of_its_own);
  RUN_TEST(flat_copies_take_element_size_bytes_per_value);
  RUN_TEST(a_failed_result_copies_its_diagnostic_and_no_value);
}
