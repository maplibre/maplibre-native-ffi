# Native tests

These Unity suites test the library's behavior once, through its C API, so that
each binding suite tests only what its binding adds. A behavior belongs here
when the C API can express it, including shapes that no binding can construct,
such as null pointers, undersized structs, unknown enum values, and stale
handles.

## Run the suites

`mise run test [preset]` builds a preset and runs every suite on it. The harness
accepts Unity's options, and a filter that matches no case fails the run:

| Option    | Effect                                                  |
| --------- | ------------------------------------------------------- |
| `-l`      | Lists every case as `<file>:<case>` and exits           |
| `-f TEXT` | Runs the cases whose file path or name contains `TEXT`  |
| `-n NAME` | Runs the case named exactly `NAME`                      |
| `-x TEXT` | Skips the cases whose file path or name contains `TEXT` |

`TEXT` may list alternatives separated by commas. A case that hangs prints
`MLN_TEST_HANG <file>:<case>` with what it was waiting on, and aborts.
`MLN_TEST_TIMEOUT_SCALE` multiplies every wait and the watchdog, and the runners
set it to 3 on slow targets.

Desktop runs one CTest entry per file. The browser, emulator, and simulator
runners run many groups in one process, so state that a case leaves behind
reaches the cases after it.

## Layout

| Directory   | Contents                                                                 |
| ----------- | ------------------------------------------------------------------------ |
| `abi/`      | The ABI suite: public headers only, linked to the shipped library        |
| `internal/` | C++ cases that include `src/` and link the static library                |
| `support/`  | The harness and shared helpers; each header documents its helpers        |
| `fixtures/` | Files that the suites read through `MLN_FFI_TEST_FIXTURE_DIR`            |
| `plugin/`   | A plugin built as its own shared library, registered as a host would     |
| `exit/`     | Programs that return from `main` with native work live; exit is the test |

`abi/` has one directory per domain, which `cmake/mln_ffi_tests.cmake` lists in
run order. The fixtures under `map/issue12432/`, `offline_database/`, and
`storage/pmtiles/` are copies of MapLibre Native's test fixtures at the same
paths.

## Add a test

A test file is one group. Write each case as a `static void` function, and run
it from the file's group block:

```c
#include "support/test_support.h"

static void a_stale_handle_is_rejected(void) { /* ... */ }

MLN_TEST_GROUP {
  RUN_TEST(a_stale_handle_is_rejected);
}
```

CMake globs the files, so a new file or case needs no other edit. A file whose
name ends in a backend or platform tag, such as `_metal` or `_emscripten`,
builds only on matching presets. Select a whole file this way rather than
skipping its cases at run time.

## Rules

- The ABI suite calls every exported function by name.
  `mise run check-export-calls` fails on one that it misses, unless
  `tests/uncalled-exports.txt` lists it, and on a listed one that it calls.
  Calls from `internal/` do not count.
- A case goes in `internal/` only when no public signal can order it. First look
  for a completion that a later command fences, a gate completion, or a stepped
  frame. The internal suite uses the seams in `src/testing`, which
  `mise run check-exports` keeps out of the shipped library's exports.
- Create runtimes, maps, and render fixtures with the `mln_test_create_*` and
  fixture helpers. The harness reclaims what they recorded after a failure, and
  fails a passing case that leaves any behind.
- Wait with the helpers in `support/wait.h`, `env.h`, and `render.h`, which
  block on a signal until a deadline. Serve every request from a fixture, a
  resource provider, or the loopback server in `support/http_server.h`.
- The log callback, the async log mask, and the network status belong to the
  process. A case that changes one runs its body under `TEST_PROTECT` and then
  restores the default.
- Render cases take their GPU objects from
  [`tests/graphics`](../graphics/README.md), through `support/render.h` or
  `support/host_graphics.h`. The browser presets create their contexts in
  JavaScript instead, so a file that includes `host_graphics.h` excludes its
  cases under `__EMSCRIPTEN__`.
- A program goes in `exit/` only when process exit is what the behavior
  promises.
- `abi/platform/transport.c` checks the HTTP client that each target ships.
  OpenHarmony's client runs only under the `ci:ohos` label, so a change to that
  file's expectations needs the label.
