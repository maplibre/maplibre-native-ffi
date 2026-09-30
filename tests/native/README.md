# Native tests

Native semantics live in C. These Unity suites test the library's behavior once,
through its public C ABI, so that each language binding tests only what the
binding itself adds: handle ownership, callbacks, completions, generated shapes,
and platform integration.

A behavior belongs here when the C API can express it. That includes unsafe
shapes that no binding can construct, such as null pointers, undersized structs,
unknown enum or flag values, and stale handles. It also includes the semantic
contracts that every binding would otherwise test again: completion delivery,
disposal order, event payloads, render-session driving, resource providers, and
style edits.

## Layout

| Directory   | Contents                                                               |
| ----------- | ---------------------------------------------------------------------- |
| `abi/`      | The ABI suite, one directory per domain, linked to the shipped library |
| `internal/` | White-box tests that link the static library and include `src/`        |
| `support/`  | The harness and the helpers that the suites share                      |
| `fixtures/` | Files that the suites read at run time                                 |

The ABI suite, `mln_native_abi_tests`, includes public headers only and links
the shared library where the platform builds one, so it exercises the export
boundary that hosts link against. Its domain directories are `base`,
`completion`, `runtime`, `resources`, `map`, `projection`, `style`, `render`,
`backend`, `adapter`, `platform`, and `plugin`, and the harness runs them in
that order. `cmake/mln_ffi_tests.cmake` lists them, and a test file outside them
fails the configure step.

The fixtures under `map/issue12432/` and `offline_database/` are copies of
MapLibre Native's test fixtures at the same paths. The runner scripts push or
embed `fixtures/` as a whole, and the suites find it through
`MLN_FFI_TEST_FIXTURE_DIR`.

`internal/` holds `disposal_allocation.cpp`, a fault-injection executable with
its own runner. It links the static library so that its `operator new`
replacement applies to native disposal without altering the shipped library.

## Adding tests

A test file is one group. Write each case as a `static void` function, and run
the cases from the file's one group block:

```c
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void a_stale_handle_is_rejected(void) { /* ... */ }

MLN_TEST_GROUP {
  RUN_TEST(a_stale_handle_is_rejected);
}
```

CMake globs `abi/<domain>/*.c`, names each group after its path, and generates
the registry that the harness walks. A new file or case needs no other edit.

A file whose name ends in a backend or platform tag builds only on matching
presets: `_metal`, `_vulkan`, `_opengl`, `_webgpu`, `_egl`, `_wgl`, `_webgl`, or
`_emscripten`. Select a whole file this way rather than skipping its cases at
run time.

The build enforces the registration rather than trusting review:

- `-Werror=unused-function` (MSVC: `/we4505`) turns a case that no `RUN_TEST`
  reaches into a compile error.
- `-Werror=missing-prototypes` rejects a case written without `static`, since
  the support headers declare everything the suite legitimately exports.
- A file without its group block fails to link, because the registry names the
  group of every globbed file.

## Running tests

`mise run test [preset]` builds a preset and runs its suites. The harness
accepts Unity's options:

| Option    | Effect                                                  |
| --------- | ------------------------------------------------------- |
| `-l`      | Lists every case as `<file>:<case>` and exits           |
| `-f TEXT` | Runs the cases whose file path or name contains `TEXT`  |
| `-n NAME` | Runs the case named exactly `NAME`                      |
| `-x TEXT` | Skips the cases whose file path or name contains `TEXT` |

`TEXT` may list alternatives separated by commas, and `<file>:<case>` selects
one case in one file. A filter that matches no case fails the run.

Each target runs the ABI suite in the shape that suits it:

- **Desktop:** one CTest entry per file, run with `-f /abi/<domain>/<file>.c`,
  so `ctest --parallel` spreads the suite across processes.
- **Browser:** four CTest entries, each a page that runs a shard of domains:
  base, completion, and runtime; resources, map, projection, and style; render
  and backend; adapter, platform, and plugin.
- **iOS and tvOS simulators:** one CTest entry for the suite, then one for the
  plugin group.
- **Android, OpenHarmony, and musl:** the runner script runs the executable
  once, then again for the plugin group.

The plugin group runs last, in an invocation of its own, because a plugin
registration lasts for the rest of the process.

## Timeouts

A watchdog bounds each case at 30 seconds times `MLN_TEST_TIMEOUT_SCALE`. When a
case exceeds it, the harness prints `MLN_TEST_HANG <file>:<case>`, with what the
case was waiting on when a wait helper recorded it, and aborts the process. The
scale is 1 on hardware. The runner scripts and CI set it to 3 for emulators,
simulators, the browser, and software renderers.

CTest bounds each desktop entry at 120 seconds and each browser shard at 180
seconds. The emulator runners bound each executable at 300 seconds.

## Handle hygiene

`mln_test_create_runtime`, `mln_test_create_map`,
`mln_test_create_map_with_options`, and `mln_test_render_fixture_create` record
what they create for the calling thread, and the matching destroy helpers clear
those records. The harness's `tearDown` calls
`mln_test_reclaim_thread_resources()`, which destroys whatever is left in render
session, map, runtime order.

This matters because a failing assertion longjmps out of the test body, skipping
the test's own cleanup. A runtime left live would make every later test on that
thread fail to create one, turning one real failure into a cascade. Reclaiming
in `tearDown` keeps the failure count honest. When a test that otherwise passed
leaves handles behind, `tearDown` fails it so the leak lands on the test that
caused it.
