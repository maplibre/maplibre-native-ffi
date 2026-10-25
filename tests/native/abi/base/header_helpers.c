// The header-only helpers that C callers use in place of their own: a view of
// a C string, and an element of an array whose view reports its own stride.

#include "support/test_support.h"

static void a_cstr_view_spans_the_text_without_its_null_byte(void) {
  const char* text = "places";
  const mln_buffer_view view = mln_buffer_view_from_cstr(text);
  TEST_ASSERT_EQUAL_PTR(text, view.data);
  TEST_ASSERT_EQUAL_size_t(6, view.size);

  const char* empty = "";
  const mln_buffer_view empty_view = mln_buffer_view_from_cstr(empty);
  TEST_ASSERT_EQUAL_PTR(empty, empty_view.data);
  TEST_ASSERT_EQUAL_size_t(0, empty_view.size);
}

static void a_null_cstr_makes_an_empty_view(void) {
  const mln_buffer_view view = mln_buffer_view_from_cstr(NULL);
  TEST_ASSERT_NULL(view.data);
  TEST_ASSERT_EQUAL_size_t(0, view.size);
}

// A stride wider than the element, as a newer library reports to a caller that
// compiled against older headers.
typedef struct widened_record {
  uint32_t value;
  uint32_t appended;
} widened_record;

static void a_strided_element_steps_by_the_reported_stride(void) {
  const widened_record records[] = {{10, 11}, {20, 21}, {30, 31}};
  const void* items = records;
  const uint32_t stride = sizeof(widened_record);
  for (size_t index = 0; index < 3; index++) {
    const uint32_t* value = MLN_STRIDED_AT(uint32_t, items, stride, index);
    TEST_ASSERT_EQUAL_PTR(&records[index].value, value);
  }
}

static void the_headers_declare_the_abi_version_the_library_reports(void) {
  TEST_ASSERT_EQUAL_UINT32(MLN_C_VERSION, mln_c_version());
}

MLN_TEST_GROUP {
  RUN_TEST(a_cstr_view_spans_the_text_without_its_null_byte);
  RUN_TEST(a_null_cstr_makes_an_empty_view);
  RUN_TEST(a_strided_element_steps_by_the_reported_stride);
  RUN_TEST(the_headers_declare_the_abi_version_the_library_reports);
}
