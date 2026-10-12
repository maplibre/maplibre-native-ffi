#ifndef MLN_NATIVE_TESTS_TABLES_H
#define MLN_NATIVE_TESTS_TABLES_H

// A runner for validation tables: one row per way a descriptor can be wrong,
// each naming the edit, the status it must produce, and a fragment the
// diagnostic must contain. A failing row reports its label.
//
//   static void with_zero_size(void* descriptor) {
//     ((mln_map_viewport_options*)descriptor)->size = 0;
//   }
//
//   static const mln_test_validation_case cases[] = {
//     {"zero size", with_zero_size, MLN_STATUS_INVALID_ARGUMENT, "size"},
//   };

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "maplibre_native_c.h"
#include "unity.h"

typedef struct mln_test_validation_case {
  const char* label;
  // Edits a copy of the defaults; null submits them unchanged.
  void (*mutate)(void* descriptor);
  mln_status expected;
  // A fragment the call's diagnostic must contain, or null to leave it alone.
  const char* diagnostic;
} mln_test_validation_case;

// Submits one descriptor and reports the status, writing its diagnostic.
typedef mln_status (*mln_test_validation_call)(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
);

// Runs `call` once per case, on a fresh copy of `defaults` that the case's
// mutate edits.
static inline void mln_test_run_validation_table(
  const mln_test_validation_case* cases, size_t case_count,
  const void* defaults, size_t descriptor_size, mln_test_validation_call call,
  void* context
) {
  void* descriptor = malloc(descriptor_size == 0 ? 1 : descriptor_size);
  TEST_ASSERT_NOT_NULL(descriptor);
  for (size_t index = 0; index < case_count; index += 1) {
    const mln_test_validation_case* row = &cases[index];
    memcpy(descriptor, defaults, descriptor_size);
    if (row->mutate != NULL) {
      row->mutate(descriptor);
    }
    mln_diagnostic diagnostic = {.size = sizeof(mln_diagnostic)};
    const mln_status status = call(context, descriptor, &diagnostic);
    if (status != row->expected) {
      free(descriptor);
      TEST_ASSERT_EQUAL_INT_MESSAGE(row->expected, status, row->label);
    }
    if (
      row->diagnostic != NULL &&
      strstr(diagnostic.message, row->diagnostic) == NULL
    ) {
      free(descriptor);
      TEST_FAIL_MESSAGE(row->label);
    }
  }
  free(descriptor);
}

#endif
