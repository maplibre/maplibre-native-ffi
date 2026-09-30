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

## Support helpers

Test files include `support/test_support.h`, which brings in the helpers below.

| Header     | Provides                                                             |
| ---------- | -------------------------------------------------------------------- |
| `wait.h`   | Deadlines, the pulse, flags, gates, and the completion probe         |
| `env.h`    | Runtime and map fixtures, event draining and waits, fixture files    |
| `render.h` | The render fixture, driver service, and `mln_test_render_step_until` |
| `tables.h` | A runner for validation tables                                       |
| `hooks.h`  | The library's test hooks and the browser run-loop probes             |

Every wait blocks on a signal and gives up at a deadline: 10 seconds times the
timeout scale unless the wait names another. The signal is the pulse, a
process-wide counter that flags, gates, completions, and the fixtures' wakes all
bump. The runtime fixture installs an event wake and the render fixture installs
frame and driver-work wakes, so event and render waits wake as soon as the
library publishes. A waiter also re-checks every few milliseconds, which covers
state that the library publishes without a wake.

Use the helper that matches what the case waits for:

- A flag that another thread sets: `mln_test_flag_set` and
  `mln_test_wait_for_flag`.
- A thread to park until the case releases it: `mln_test_gate`, with
  `mln_test_gate_completion` to park the runtime worker inside a completion.
- An event: `mln_test_await_event` or `mln_test_await_event_matching`.
- Render progress: `mln_test_render_step_until`, which services a caller-driver
  session's driver work between checks.
- Any other condition: `mln_test_await` with a predicate.

The render fixture's context comes from `support/render_<backend>.c`, which
CMake selects for the preset's backend and OpenGL context provider.

### Cases that still wait on a fixed delay

These cases order threads or open a negative window with a fixed delay, which
the helpers cannot replace directly. Each needs a sync point, the render clock
seam, or a fence:

- `runtime_teardown_leaves_other_runtimes_responsive`,
  `resource_transform_lookup_leaves_other_runtimes_responsive`,
  `clearing_resource_provider_waits_for_in_flight_callback`,
  `runtime_teardown_waits_for_in_flight_provider_callback`, and the three cases
  that run `run_release_waits_for_in_flight_cancel_callback`: 200 ms delays
  inside provider, transform, and cancel callbacks.
- `cancel_callback_skips_a_completed_request`: a 200 ms window in which no
  cancel may arrive.
- `a_barrier_completes_after_preceding_work` and
  `runtime_release_waits_for_retired_map_cleanup`: 100 ms windows in which a
  completion must not arrive.
- `demand_coalescing_preserves_boundaries_and_generations`: a 1 ms delay that
  lets a 1 ns frame deadline pass on the real clock.
- `barrier_waits_for_a_demand_parked_by_a_full_ring`: a 50-iteration service
  loop in which a barrier must not complete.
- `fast_pfor_option_gates_mlt_tile_decoding`: 600 render attempts spaced 1 ms
  apart before it concludes that a tile decodes to nothing.
- `acquired_frame_release_after_abandon_is_cpu_only`: a 5 ms delay before
  abandon. A core-worker session publishes its frame result while the driver
  call is still in flight, and abandon returns busy until the call ends. A
  wake-driven wait reaches abandon inside that window about one run in eight on
  Metal, so this case needs the core fix rather than a sync point.

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
