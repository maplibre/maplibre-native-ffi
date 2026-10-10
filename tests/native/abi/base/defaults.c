// Record defaults. A binding that builds a record from its language's
// defaults, rather than from the record's default function, reads each field's
// annotated `default=` and treats an unannotated field as zero. The generated
// cases check the default functions against those annotations.

#include "default_cases_generated.inc"
#include "support/test_support.h"

#define MLN_RUN_DEFAULT_CASE(name) RUN_TEST(name);

MLN_TEST_GROUP { MLN_DEFAULT_CASES(MLN_RUN_DEFAULT_CASE) }
