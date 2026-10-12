#ifndef MLN_NATIVE_TESTS_UNITY_CONFIG_H
#define MLN_NATIVE_TESTS_UNITY_CONFIG_H

// Unity's options for the native suites. Unity's own translation unit and every
// test that includes unity.h read this header, so the options agree on both
// sides; cmake/mln_ffi_tests.cmake points UNITY_INCLUDE_CONFIG_H here.

// -l, -f, -n, and -x. The harness runs each case through these, so a CTest
// entry, an emulator run, or a developer selects cases by file or name.
#define UNITY_USE_COMMAND_LINE_ARGS

// Appends each case's wall time to its result line. The timing macros come from
// here rather than Unity's defaults, which trip the sign-conversion and
// extra-semicolon warnings Unity builds itself with under Clang.
#include <stdint.h>

#ifdef __cplusplus
extern "C" uint64_t mln_test_now_milliseconds(void);
#else
uint64_t mln_test_now_milliseconds(void);
#endif

#define UNITY_INCLUDE_EXEC_TIME
#define UNITY_TIME_TYPE uint64_t
#define UNITY_EXEC_TIME_START() \
  (Unity.CurrentTestStartTime = mln_test_now_milliseconds())
#define UNITY_EXEC_TIME_STOP() \
  (Unity.CurrentTestStopTime = mln_test_now_milliseconds())
#define UNITY_PRINT_EXEC_TIME()                                         \
  do {                                                                  \
    UnityPrint(" (");                                                   \
    UnityPrintNumberUnsigned((UNITY_UINT)(Unity.CurrentTestStopTime -   \
                                          Unity.CurrentTestStartTime)); \
    UnityPrint(" ms)");                                                 \
  } while (0)

// Unity 2.6 makes double support opt-in; without it TEST_ASSERT_*_DOUBLE fails
// unconditionally.
#define UNITY_INCLUDE_DOUBLE

// Handles are 64 bits on every target, so a 32-bit target still needs the
// UINT64 assertions that Unity otherwise infers from pointer width.
#define UNITY_SUPPORT_64

// Unity buffers its per-test lines when stdout is a pipe, so a run that never
// returns would take its progress with it. Flushing each line leaves the last
// case it started in the log.
#define UNITY_USE_FLUSH_STDOUT

// Every case runs through the harness, which applies the command-line filter,
// answers -l, and arms the hang watchdog around the case.
#define RUN_TEST(func) mln_test_run_case(func, #func, __LINE__)

#endif
