# Raw C API tests

These Unity tests exercise `maplibre_native_c.h` directly from C. They stay
below the language bindings so they can cover raw ABI behavior that bindings
hide on purpose.

Tests belong here when they require unsafe C API shapes that a binding cannot
construct: null input or output pointers, undersized structs, unknown raw enum
or flag values, preinitialized output handles, and stale raw handles. Semantic
behavior belongs in each applicable binding's test suite whenever its public API
can express the scenario.

## Registration contract

Each `*_abi.c` holds `static void <name>(void)` tests plus one
`run_<file>_tests(void)` entry point that starts with
`UnitySetTestFile(__FILE__)` and then calls `RUN_TEST` once per test in the
file. `abi_tests.h` declares the entry points; `main.c` calls them between
`UNITY_BEGIN()` and `UNITY_END()`.

Adding a test means writing it `static` and adding its `RUN_TEST` call. Adding a
file means creating `<name>_abi.c`, declaring `run_<name>_tests(void)` in
`abi_tests.h`, and calling it from `main.c`; CMake globs the sources, so the
build list needs no edit.

The build enforces this rather than trusting review:

- `-Werror=unused-function` (MSVC: `/we4505`) turns a test that no `RUN_TEST`
  reaches into a compile error, because nothing references it.
- `-Werror=missing-prototypes` keeps that net closed by rejecting a test written
  without `static`, since `abi_tests.h` and `test_support.h` declare everything
  the suite legitimately exports.
- A configure-time check in `cmake/mln_ffi_tests.cmake` fails with a clear
  message when a globbed `*_abi.c` has no matching call in `main.c`.

The compiler carries this instead of Unity's `generate_test_runner.rb` (Ruby is
outside this repo's toolchain) or a regex checker (`render_backend_abi.c` guards
both its tests and their `RUN_TEST` calls behind backend `#if` blocks, which
only a preprocessor reads correctly).

## Handle hygiene

`mln_test_create_runtime`, `mln_test_create_map`,
`mln_test_create_map_with_options`, and `mln_test_render_fixture_create` record
what they create for the calling thread, and the matching destroy helpers clear
those records. `tearDown` in `main.c` calls
`mln_test_reclaim_thread_resources()`, which destroys whatever is left in render
session, map, runtime order.

This matters because a failing assertion longjmps out of the test body, skipping
the test's own cleanup. A runtime left live would make every later test on that
thread fail to create one, turning one real failure into a suite-wide cascade.
Reclaiming in `tearDown` keeps the failure count honest. When a test that
otherwise passed leaves handles behind, `tearDown` fails it so the leak lands on
the test that caused it.
