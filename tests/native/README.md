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

Every function that a public header exports is called by name somewhere in the
ABI suite's sources, which are every file here outside `internal/`. A call from
the internal suite does not count, since that suite links the static library
rather than the export. `mise run check-export-calls` fails on an exported
function that no ABI test calls unless `tests/uncalled-exports.txt` lists it.
Adding a call to a listed function means dropping its line from that file in the
same change.

## Layout

| Directory   | Contents                                                               |
| ----------- | ---------------------------------------------------------------------- |
| `abi/`      | The ABI suite, one directory per domain, linked to the shipped library |
| `internal/` | White-box tests that link the static library and include `src/`        |
| `support/`  | The harness and the helpers that the suites share                      |
| `fixtures/` | Files that the suites read at run time                                 |
| `plugin/`   | The test plugin, built as a shared library of its own                  |
| `exit/`     | Programs whose process exit is the check, one per file                 |

The ABI suite, `mln_native_abi_tests`, includes public headers only and links
the shared library where the platform builds one, so it exercises the export
boundary that hosts link against. Its domain directories are `base`,
`completion`, `runtime`, `resources`, `map`, `projection`, `style`, `render`,
`backend`, `adapter`, `platform`, and `plugin`, and the harness runs them in
that order. `cmake/mln_ffi_tests.cmake` lists them, and a test file outside them
fails the configure step.

The fixtures under `map/issue12432/`, `offline_database/`, and
`storage/pmtiles/` are copies of MapLibre Native's test fixtures at the same
paths. The runner scripts push or embed `fixtures/` as a whole, and the suites
find it through `MLN_FFI_TEST_FIXTURE_DIR`.

The plugin group registers `plugin/square_plugin.c` the way a host loads a
plugin. The plugin is a shared library that includes only MapLibre Native's
plugin header and links nothing of this library. The suite passes the function
that `mln_plugin_get_register_function_v1()` returns to the plugin's entry
point, and the plugin registers its layer type through that pointer. Emscripten
builds link the plugin statically, as they do the library.

The adapter group tests the Dart entry points of `callback_adapter.h` without a
Dart VM. The host passes those entry points the address of Dart's
`NativeApi.postCObject`, so the cases pass a fake that records each message in
the `Dart_CObject` layout. Where the VM would run a native-pointer finalizer for
an undelivered message, the case runs it.

Each program in `exit/` makes its calls and returns from `main` at once, so its
process exits with no harness teardown in between, and a crash or a nonzero
status at exit fails its entry, `native-exit/<file>` under CTest. It links the
shipped library, as the ABI suite does. A behavior belongs there only when
process exit is what it promises. The programs leave native work running as they
exit: live runtimes and maps loading a style, and the resource provider, wakes,
and log callback all installed. The render-session program renders on a core
worker and on a host graphics thread, and abandons both sessions mid-frame just
before it returns, since a host ends every session's graphics calls before it
exits. `exit/probe.h` holds what the programs share, and `exit/render_probe.h`
the render-session program, which each backend's tagged file attaches. A program
whose name ends in a backend tag builds only on matching presets, as a suite
file does.

## The internal suite

The internal suite, `mln_native_internal_tests`, is C++ that includes `src/`
headers and links the static library. It holds the cases that no public fence
can order, and the white-box cases for internal modules with contracts of their
own, such as the completion state machine and disposal. A case belongs here only
when the ABI suite cannot express it: first look for a public signal, such as a
completion that a later command fences, a gate completion, or a stepped frame.

Each file directly under `internal/` is one group, registered the way the ABI
suite's files are. Its helpers live in `internal/support/`:

| Header                  | Provides                                                      |
| ----------------------- | ------------------------------------------------------------- |
| `sync_points.hpp`       | `SyncPointScope`, which counts and holds the library's points |
| `allocation_faults.hpp` | `AllocationFaults`, which fails every allocation on a thread  |
| `driver_blocker.hpp`    | `DriverBlocker`, which parks a render driver inside its call  |
| `resources.hpp`         | Resource provider, transform, and offline download helpers    |
| `checks.hpp`            | A wait on a C++ predicate, and checks for library threads     |

The library's seams are compiled into every build, and nothing exports them, so
the objects under test are the shipped ones:

- `src/testing/sync_point.hpp` names the points where a case can observe or park
  a thread, such as a map's pool shutdown, a writer about to take a resource
  registration exclusively, a release blocked on a running cancel callback, a
  render driver call that has published its work and is about to end, a GeoJSON
  worker about to slice a tile, or a projection close waiting for a call. With
  no handler installed, reaching a point costs one relaxed atomic load. A point
  that marks a wait fires only when the thread has to wait. A case that waits
  for such a point fails at its deadline when a change removes the wait.
- `src/testing/render_clock.hpp` is the clock that frame demand deadlines run
  on. A case advances it rather than waiting for a deadline to pass.

`mise run check-exports [preset]` reads the installed library's symbol table and
fails when it exports anything beyond the public C API, or any name that belongs
to a seam. Every CI job that builds a library checks that library, because
`mise run build` and `mise run archive-native` run the check. The hygiene job
builds no library and has nothing to check. A coverage build, configured with
`MLN_FFI_ENABLE_COVERAGE`, also exports `mln_ffi_coverage_write_profile`, and
the check allows that one name when the preset's CMake cache enables coverage.

A static archive keeps hidden symbols visible to a static link. For a preset
that installs only an archive, such as Emscripten, the check compares the
archive's `mln_` C names alone. The seams have C++ names outside that set, so
the seam check covers shared libraries only.

The resource cases set a `custom://` style URL on a map to make the library call
a provider or transform on a file source thread, with no network. The cases in
which runtime teardown races the callback start an offline download of that
style instead: a runtime releases only once its maps are released, and a
download requests the style with no map. The sync points then order the callback
against the thread that it races.

The suite's replacement `operator new` covers the static library's own
allocations, and throws only on a thread inside an `AllocationFaults` scope.

## Adding tests

A test file is one group. Write each case as a `static void` function, and run
the cases from the file's one group block:

```c
#include "support/test_support.h"

static void a_stale_handle_is_rejected(void) { /* ... */ }

MLN_TEST_GROUP {
  RUN_TEST(a_stale_handle_is_rejected);
}
```

CMake globs `abi/<domain>/*.c`, names each group after its path, and generates
the registry that the harness walks. A new file or case needs no other edit.

A file whose name ends in a backend or platform tag builds only on matching
presets, in either suite: `_metal`, `_vulkan`, `_opengl`, `_webgpu`, `_egl`,
`_wgl`, `_webgl`, or `_emscripten`. Select a whole file this way rather than
skipping its cases at run time.

The build enforces the registration rather than trusting review:

- `-Werror=unused-function` (MSVC: `/we4505`) turns a case that no `RUN_TEST`
  reaches into a compile error.
- `-Werror=missing-prototypes` rejects a case written without `static`, since
  the support headers declare everything the suite legitimately exports.
- A file without its group block fails to link, because the registry names the
  group of every globbed file.

## Running tests

`mise run test [preset]` builds a preset and runs its suites. On every target
that runs its suites through CTest, which covers desktop, simulator, and
Emscripten targets, it first runs `cargo test` on the host for the Rust platform
crate's pure helpers, such as redirect resolution. The harness accepts Unity's
options:

| Option    | Effect                                                  |
| --------- | ------------------------------------------------------- |
| `-l`      | Lists every case as `<file>:<case>` and exits           |
| `-f TEXT` | Runs the cases whose file path or name contains `TEXT`  |
| `-n NAME` | Runs the case named exactly `NAME`                      |
| `-x TEXT` | Skips the cases whose file path or name contains `TEXT` |

`TEXT` may list alternatives separated by commas, and `<file>:<case>` selects
one case in one file. A filter that matches no case fails the run.

Each target runs the suites in the shape that suits it:

- **Desktop:** one CTest entry per file, run with `-f /abi/<domain>/<file>.c` or
  `-f /internal/<file>.cpp`, so `ctest --parallel` spreads the suites across
  processes. Each exit program is one more entry.
- **Browser:** four CTest entries for the ABI suite, each a page that runs a
  shard of domains: base, completion, and runtime; resources, map, projection,
  and style; render and backend; adapter, platform, and plugin. The internal
  suite runs as one more page.
- **iOS and tvOS simulators:** one CTest entry for the ABI suite, one for its
  plugin group, and one for the internal suite.
- **Android, OpenHarmony, and musl:** the runner script pushes and runs the ABI
  suite and then the internal suite, then each exit program, then the ABI suite
  again for the plugin group.

The plugin group runs last because a plugin registration lasts for the rest of
the process. Every target except the browser also runs it in an invocation of
its own; the browser runs it at the end of the fourth shard.

## Timeouts

A watchdog bounds each case at 30 seconds times `MLN_TEST_TIMEOUT_SCALE`. When a
case exceeds it, the harness prints `MLN_TEST_HANG <file>:<case>`, with what the
case was waiting on when a wait helper recorded it, and aborts the process. The
scale is 1 on hardware. The runner scripts and CI set it to 3 for emulators,
simulators, the browser, and software renderers.

CTest bounds each desktop entry at 120 seconds, each browser page at 180
seconds, and a simulator's ABI suite, plugin, and internal suite entries at 300,
120, and 120 seconds. The browser and simulator runners stop ten seconds short
of their entry's bound, so the runner reports the timeout rather than CTest. The
emulator runners bound each executable at 300 seconds.

## Support helpers

Test files include `support/test_support.h`, which brings in Unity, the harness,
the C library headers that most cases use, and the helpers below.

| Header     | Provides                                                             |
| ---------- | -------------------------------------------------------------------- |
| `wait.h`   | Deadlines, the pulse, flags, gates, and the completion probe         |
| `env.h`    | Runtime and map fixtures, event draining and waits, fixture files    |
| `render.h` | The render fixture, driver service, and `mln_test_render_step_until` |
| `status.h` | Status checks that name the call they check                          |
| `tables.h` | A runner for validation tables                                       |

Check a status with `MLN_TEST_OK`, `MLN_TEST_INVALID`, or `MLN_TEST_STATUS`,
which report the call that returned the wrong status. `MLN_TEST_AWAIT_OK` and
`MLN_TEST_AWAIT_COMMAND` submit a command and wait for its terminal status, and
`MLN_TEST_RENDER_AWAIT` does the same for a render session operation, servicing
the fixture until it completes. Each declares the `completion` that its
expression passes.

The resource and platform suites also include two headers of their own:

| Header          | Provides                                                              |
| --------------- | --------------------------------------------------------------------- |
| `resources.h`   | Resource configuration commands, and a provider that records requests |
| `http_server.h` | A scripted HTTP server on 127.0.0.1 that logs each request's headers  |

The scripted provider answers the URLs that a case lists, fails every other
request as not found, and copies each request it sees, including the prior cache
metadata that a revalidation carries. The loopback server answers from a route
table and closes each connection after one response. A route can carry an ETag
that it answers 304 for, serve byte ranges, or hold its response until the case
releases it. The browser has no sockets, so its build leaves the server out, and
its transport cases run against the runner's routes instead.

The transport cases in `abi/platform/transport.c` check whichever HTTP client a
target ships: the Rust transport on Linux, Windows, and Android, the
`NSURLSession` client on Apple targets, and MapLibre's own `platform/ohos`
client on OpenHarmony. OpenHarmony runs only under the `ci:ohos` label, so a
change to the table's expectations needs that label to reach its client.
`mln_test_temp_path` in `env.h` names a file in the temporary directory for a
case that writes one.

The host-target fixtures in `support/host_graphics.h` need their own include, as
described under [GPU objects](#gpu-objects).

The render files also include `support/frames.h`. It requests forced frames,
waits until every pending demand has a result and one drain holds as many as the
case expects, renders and acquires one frame, releases a frame, and reads the
latest frame back. A case that needs a negative check on the driver, such as a
barrier that must still be pending, fences the driver first with a maintenance
command, which runs after every work item the driver already holds.

The map files also include `support/map.h`, which provides
`mln_test_render_still_image`. That helper requests a still image from a static
or tile map and keeps a forced frame demand in flight until the image completes.
`mln_test_render_pending_still_image` does the same for a request the case
already made.

The style and render suites also include `support/style.h`. It renders one frame
at a time until a condition holds, and copies rendered and source feature
queries, with the caller's geometry and options, and list queries out of their
borrowed results. It also serves URL resources through a resource provider that
fails every request it has no route for, so a case that adds a URL source never
reaches the network. `MLN_TEST_EXPECT_COMMAND_FAILED` expects a command to fail
after it was accepted, and `MLN_TEST_EXPECT_COMMAND_REJECTED` expects its
submission to fail. A copied result that overflows its buffer fails the case
instead of truncating.

The backend validation files also include `support/attach_table.h`. It runs a
validation table over one attach or set_target call, with the rows that every
attach shares: a null map, descriptor, options, output, or completion,
undersized options, and an occupied output. A row that an attach rejects must
leave its output session as it was. The header also defines the
undersized-descriptor, undersized-extent, and overflowing-extent rows for any
descriptor type. A set_target table runs on every build. A build without the
backend checks the descriptor before it reports the missing backend, so its rows
name no session. A build with the backend checks the session first, so its rows
name a live session of the replacement's kind from `support/host_graphics.h`.

The adapter cases also include `support/adapter.h` directly. It provides a
completion listener that keeps its record, committed resource provider changes,
and a map that the handle fixtures do not record, for a case in which an adapter
owner disposes it.

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
  `mln_test_gate_completion` to park whichever thread delivers a completion. A
  completion runs on the submitting thread when the work finishes before the
  submission returns, so a case that must park the runtime worker belongs in the
  internal suite, which posts a task to the worker directly.
- An event: `mln_test_await_event` or `mln_test_await_event_matching`.
- Render progress: `mln_test_render_step_until`, which services a caller-driver
  session's driver work between checks.
- A static or tile map that has loaded and rendered everything it requested:
  `mln_test_render_still_image`.
- Any other condition: `mln_test_await` with a predicate.

A runtime barrier completes once every earlier submission has a terminal result.
It does not wait for work that the runtime worker does on its own, such as
handling a frame a render session finished, so it cannot fence that work. A map
command queued after the work can: its completion runs after it.

### GPU objects

On every native backend, the render fixture's context comes from
[`tests/graphics`](../graphics/README.md), the library that the binding suites
load for their GPU objects too. `support/render_graphics.c` attaches the owned
texture that `mln_test_render_fixture_create` returns. `support/host_graphics.h`
adds fixtures for the other target kinds: a borrowed texture that a case can
read back through `mln_test_render_fixture_read_texture`, and a presentation
surface. For a retarget, `mln_test_render_fixture_new_texture` and
`mln_test_render_fixture_new_surface` make another target on the fixture's
graphics object, and `mln_test_render_fixture_set_texture` and
`mln_test_render_fixture_set_surface` hand a target to the backend's set_target.
The fixture destroys the targets it made after its session. A case that builds
its own descriptors creates a graphics object with `mln_test_graphics_create`
and reads its handles from there.

The browser presets have none of these. Their contexts come from JavaScript, so
`support/render_webgl.c` and `support/render_webgpu.c` create them, and a file
that uses `host_graphics.h` excludes its cases under `__EMSCRIPTEN__`.
`support/render_egl.c` holds the dedicated EGL fixtures, whose sessions create
their own context on a display from `tests/graphics`.

### Cases that work around a core defect

- The cache cases in `abi/resources/provider.c`,
  `abi/resources/ambient_cache.c`, and `abi/platform/transport.c` keep their
  first map until the second has loaded. Without a cache path, a runtime's
  ambient cache is an in-memory database that lives only while a map or an
  earlier offline or cache operation holds it, so releasing the only map empties
  the cache. The workaround goes once the runtime holds its database for its
  whole life, or `runtime.h` documents this lifetime.

## Process-global state

The log callback, the async log mask, and the network status belong to the
process, and the browser, emulator, and simulator runs share one process across
every group. Only `abi/base/logging.c` and `abi/runtime/network_status.c` change
them. Each case in those files runs its body under `TEST_PROTECT` and then
restores the default, so a failed assertion still leaves the next group the
state that it expects.

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
