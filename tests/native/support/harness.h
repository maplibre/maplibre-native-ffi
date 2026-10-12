#ifndef MLN_NATIVE_TESTS_HARNESS_H
#define MLN_NATIVE_TESTS_HARNESS_H

// Registration and execution for the native suites.
//
// Each test file is one group. It defines its cases as `static void` functions
// and runs them from one `MLN_TEST_GROUP { RUN_TEST(case); ... }` block. CMake
// globs the files, names each group after its path, and generates the registry
// the harness walks, so adding a file or a case needs no other edit.

#include <stdint.h>

#include "unity.h"

#ifdef __cplusplus
extern "C" {
#endif

// Runs one case: applies the -f/-n/-x filter, prints it under -l, and arms the
// hang watchdog while it runs. RUN_TEST expands to this.
void mln_test_run_case(UnityTestFunction func, const char* name, int line);

// Monotonic milliseconds, for the per-case times Unity prints and for waits.
uint64_t mln_test_now_milliseconds(void);

// The multiplier MLN_TEST_TIMEOUT_SCALE sets for every wait and the watchdog:
// 1 on hardware, 3 where the runner scripts know the target is slow
// (emulators, simulators, the browser, software renderers).
uint32_t mln_test_timeout_scale(void);

// Arms the hang watchdog for one case: past 30 s × scale it prints
// `MLN_TEST_HANG <file>:<case>` and aborts, so a stall names the case rather
// than running out the runner's timeout. The harness arms it around each case.
void mln_test_watchdog_arm(const char* file, const char* name);
void mln_test_watchdog_disarm(void);
// Names what the case is waiting on for the hang report. `what` must outlive
// the wait, as a string literal does; null clears it. Only the thread that runs
// the cases records a note; a wait on any other thread leaves it unchanged.
void mln_test_watchdog_note(const char* what);

#ifdef __cplusplus
}
#endif

// MLN_TEST_GROUP_NAME comes from CMake per file, so the group's symbol follows
// the file's path. A test file without a group fails to link. The harness is C,
// so a C++ file's group keeps C linkage.
#ifdef __cplusplus
#define MLN_TEST_GROUP_LINKAGE extern "C"
#else
#define MLN_TEST_GROUP_LINKAGE
#endif
#define MLN_TEST_GROUP                                   \
  MLN_TEST_GROUP_LINKAGE void MLN_TEST_GROUP_NAME(void); \
  MLN_TEST_GROUP_LINKAGE void MLN_TEST_GROUP_NAME(void)

#endif
