#ifndef MLN_NATIVE_TESTS_STATUS_H
#define MLN_NATIVE_TESTS_STATUS_H

// Status checks that name the call they check, so a failure reports which call
// returned what.

#include "maplibre_native_c.h"
#include "unity.h"

// Asserts that the call returns `expected`.
#define MLN_TEST_STATUS(expected, ...) \
  TEST_ASSERT_EQUAL_INT_MESSAGE((expected), (__VA_ARGS__), #__VA_ARGS__)
#define MLN_TEST_OK(...) MLN_TEST_STATUS(MLN_STATUS_OK, __VA_ARGS__)
#define MLN_TEST_INVALID(...) \
  MLN_TEST_STATUS(MLN_STATUS_INVALID_ARGUMENT, __VA_ARGS__)

// Asserts that `actual` is MLN_STATUS_OK, reporting `message` instead of the
// call, such as a table row's label.
#define MLN_TEST_OK_MESSAGE(actual, message) \
  TEST_ASSERT_EQUAL_INT_MESSAGE(MLN_STATUS_OK, (actual), (message))

#endif
