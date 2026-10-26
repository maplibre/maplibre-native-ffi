#ifndef MLN_NATIVE_TESTS_TEST_SUPPORT_H
#define MLN_NATIVE_TESTS_TEST_SUPPORT_H

// Everything a native test file uses from the support layer, with Unity, the
// harness, and the C library headers that most cases need.
//
// A test file is one group: it writes each case as a static void function and
// runs it from its MLN_TEST_GROUP block. CMake globs the files. A file whose
// name ends in a backend or platform tag, such as _metal or _emscripten, builds
// only on matching presets; select a whole file this way rather than skip its
// cases at run time. A program goes in exit/ only when process exit is what the
// behavior promises.
//
// The browser, emulator, and simulator runners run many groups in one process,
// so a case leaves no state behind. Create runtimes, maps, and render fixtures
// with the mln_test_create_* and fixture helpers, which the harness reclaims
// after a failure. The log callback, the async log mask, and the network status
// belong to the process: a case that changes one runs its body under
// TEST_PROTECT and then restores the default.
//
// The harness takes Unity's -l, -f, -n, and -x options. A filter matches a file
// path or case name, takes comma-separated alternatives, and fails the run when
// it matches nothing. MLN_TEST_TIMEOUT_SCALE multiplies every wait.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Whether a case can drive the platform's own HTTP client against the loopback
// server in support/http_server.h. The browser has no sockets, and Android's
// client calls Java, which these executables run without; the Kotlin device
// suite covers Android's client instead.
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#define MLN_TEST_PLATFORM_HTTP 0
#else
#define MLN_TEST_PLATFORM_HTTP 1
#endif

#include "env.h"
#include "harness.h"
#include "render.h"
#include "status.h"
#include "tables.h"
#include "unity.h"
#include "wait.h"

#endif
