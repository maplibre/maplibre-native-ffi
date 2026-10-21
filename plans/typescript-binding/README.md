# Generate a TypeScript binding for Node-API hosts

The TypeScript binding serves Node.js, Bun, Deno, and ArkTS on OpenHarmony from
one package and one native addon. The binding compiler generates it from the
same semantic model as the other eight bindings: a TypeScript emitter writes the
public API, and a native emitter writes a C Node-API addon. Every callback
reaches the JavaScript event loop through the core's delivery queue, and a
handwritten wake shim in the addon is the binding's only path from a native
thread. [The binding README](../../bindings/typescript/README.md) describes that
runtime, the build, and the tests.

The binding also tests a claim of PR #760. PR #553, a TypeScript prototype, and
PR #557, a Kotlin/Wasm prototype, stalled on thread-affine runtimes, bespoke
callback delivery, and large handwritten surfaces. PR #760 moves execution into
the native core, delivers every callback through one queue that an async-only
host drains, and generates the bindings from the C headers. This binding builds
on PR #760 without changing the native core, and [Blockers](#blockers) maps each
blocker that the prototypes hit to the PR #760 mechanism that removes it and to
the test that shows it.

## Decisions

| Question                 | Decision                                                                                                                                                                                                                                  |
| ------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Native transport         | A C11 Node-API addon at `NAPI_VERSION` 8. `typescript_native.py` generates its operations and converters; a handwritten C runtime owns the wake shim, owner tokens, and teardown.                                                         |
| Addon build              | A CMake project in `bindings/typescript/native` whose presets include the root presets. The root presets name their toolchain files through `${fileDir}`.                                                                                 |
| Callback delivery        | One `mln_delivery_queue` per realm carries every completion and callback, as for Dart. `async_hosts.py` chooses each registration's form for both bindings: a queue and a token, a wake, native rules, or a disabled default.             |
| Event-loop scheduling    | One Node-API threadsafe function per realm. The queue's wake posts token 0, which drains the queue; a render session's wake posts its own token.                                                                                          |
| Event-loop liveness      | The threadsafe function is referenced while a binding Promise is pending, including a pending wait on an `AwaitableWake`. A callback registration or a wake keeps nothing alive.                                                          |
| Events and frame results | Runtime events arrive as event handler calls on the event loop. Frame results stay queued until `drainFrameResults()` reads them after a frame wake. `AwaitableWake` turns a callback or a wake into a Promise that holds the event loop. |
| Realm teardown and exit  | An environment cleanup hook or the threadsafe function's finalizer closes the shim, and the hook releases the queue. A synchronous `exit` listener, and on Deno an `unload` listener, abandons every session before `exit()` runs.        |
| Close and collection     | Explicit `close()` and `dispose()`, with `Symbol.asyncDispose` and `Symbol.dispose`. An owner keeps its ID, a released flag, and its parent. Node-API finalizers dispose abandoned handles through owner tokens.                          |
| Synchronous answers      | Native code answers every decision of a queued registration. The HTTP header transform is a native rule table, and the Vulkan queue lock stays disabled.                                                                                  |
| Package and loading      | A facade, one package per backend, and one addon package per backend and platform, all at one exact version. Node, Bun, and Deno resolve the installed backend package, then its addon; ArkTS imports the addon by library name.          |
| Test rendering           | Graphics objects from `tests/graphics` through a test-only Node-API addon. Core-worker readback for REND-01; driver service on the JavaScript thread for REND-02.                                                                         |
| Test harness             | One suite written against a harness with declared host capabilities, with entry points for `node:test`, `bun:test`, `Deno.test`, and the hypium runner.                                                                                   |
| Browser and WebAssembly  | Out of scope, as [Browser and WebAssembly](#browser-and-webassembly) explains.                                                                                                                                                            |

The rejected transports cost more than they remove:

| Candidate                                    | Consequence                                                                                                                                                                                                                | Decision                   |
| -------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------- |
| napi-rs over the Rust core crate, as PyO3    | napi-rs registers through Node's module path, and OpenHarmony needs the separate napi-ohos crate. The shared Rust core runs host code on native threads, which this binding cannot use. Rust adds a toolchain and no code. | Rejected.                  |
| JavaScript struct encoders over generic call | Node-API exposes no stable address for nested JavaScript memory, so a generic call needs a slot ABI and pointer relocation: the dispatch layer of PR #553.                                                                 | Rejected.                  |
| Generated C converters per value plan        | Every operation compiles to a direct call. The converters share value plans with `native_capture.py`, and the build uses the native preset's toolchain.                                                                    | Use for the addon.         |
| A Node-API wake in the native core           | Mirrors the Dart wake in `binding_support.h`, and adds a Node-API dependency to the core.                                                                                                                                  | Rejected; the addon posts. |

## Blockers

Test paths are relative to `bindings/typescript`, and a path under `tests/`
starts at the repository root. The suite runs under Node, Bun, and Deno through
`mise run //bindings/typescript:test <preset>`, and on the OpenHarmony emulator
through `mise -E ohos run //bindings/typescript:test ohos-x64-egl`.

| Blocker                                                                                   | PR #760 mechanism                                                                                                                           | Test                                                                                                                                                                                                                        |
| ----------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| B1. A runtime belonged to its owner thread, one per thread, with a host pump              | Each runtime has its own executor thread. Operations accept calls from any thread and finish with completions.                              | `test/realms.test.ts::two runtimes on one thread and in two workers load styles without a pump`, and `test/event-loop.test.ts::a script that awaits a style-loaded event and closes its runtimes exits on its own`          |
| B2. A normalized dispatch ABI carried every call through slots                            | Each C call takes a diagnostic of its own, so the addon calls each C function directly.                                                     | `tests/bindgen/test_typescript_emitter.py::test_each_operation_calls_its_own_function`                                                                                                                                      |
| B3. Bespoke record queues and a notifier delivered callbacks, and realms collided         | One delivery queue per realm, whose deliveries native code copies, and generated payload converters.                                        | `test/realms.test.ts::provider requests and tile fetches reach only the realm that registered them`                                                                                                                         |
| B4. A notifier could be freed while a native thread called it (issue #540)                | A wake's release runs after its calls have returned, and a released queue refuses pushes. The realm's teardown closes its shim first.       | `test/realms.test.ts::terminating a worker mid-burst leaves the other realm working`                                                                                                                                        |
| B5. Finalizers could not destroy thread-affine handles                                    | Disposal works from any thread and accepts live children. Owner tokens give finalizers a disposal path.                                     | LIFE-04, `test/platform.test.ts::an abandoned runtime with a live map is disposed and reported when collected`, and COMP-03, `test/completions.test.ts::a dropped map creation is disposed once collected`                  |
| B6. One failed test left a runtime that broke every later test                            | No limit on runtimes per thread, and abandoned runtimes are disposed.                                                                       | `test/realms.test.ts::two runtimes on one thread and in two workers load styles without a pump`, and LIFE-04                                                                                                                |
| B7. Each prototype hand-wrote its public API                                              | The compiler generates operations, values, conversions, delivery dispatch, and native capture from the headers.                             | `mise run bindings:check`, which runs the compiler with `--require-complete` and reports no unsupported function for TypeScript                                                                                             |
| B8. Logging, transforms, and provider decisions needed immediate answers                  | Native code answers each decision of a queued registration, and a provider or transform request arrives with its handle for a later answer. | CB-05, `test/resource-provider.test.ts::a provider answers the requests its routes match after its callback returns`, and `test/log.test.ts::a log record reaches the event loop after its native call returns`             |
| B9. A style URL never reached a local origin under the test runtime                       | File sources run on native threads, with no host pump.                                                                                      | `test/resource-provider.test.ts::a style URL reaches a local origin with a rule-transformed header without a pump`                                                                                                          |
| B10. Render tests needed per-runtime graphics FFI                                         | Core-worker owned-texture sessions render and read back on a native thread and complete as Promises.                                        | REND-01, `test/render.test.ts::an owned texture renders a frame that reads back as pixels`, and REND-02, `test/render.test.ts::a caller-driven session is serviced from the event loop`                                     |
| B11. ArkTS needed loader glue, a UTF-8 shim, hand-built HAPs, and the SDK `ets` component | None. Converting strings in C removes the UTF-8 shim, and `scripts/build-hap.sh` builds the HAP from mise.                                  | The `ohos-x64-egl` run of the `test` task                                                                                                                                                                                   |
| B12. Deno and Bun loading quirks, and a referenced wake that held the process open        | Native code allows exit with live runtimes. The binding references the event loop only while a binding Promise is pending.                  | `test/event-loop.test.ts::a runtime left open does not keep the process alive`, and PLAT-01, `test/platform.test.ts::loads the addon from its installed package`                                                            |
| B13. Wasm worker threads could not call into the host agent                               | Queue delivery, which needs only a wake on the host's side, and native retirement. A Wasm transport is out of scope.                        | CB-01, `test/callbacks.test.ts::a callback root is released by a removal, a map close, or a refusal`, and `tests/bindgen/test_typescript_emitter.py::test_registrations_reach_the_realm_through_the_shared_async_host_plan` |
| B14. The browser page thread could not block on a synchronous, thread-affine API          | The header contracts complete operations later. Only the immediate, snapshot, and event-batch categories return synchronously.              | `tests/bindgen/test_typescript_emitter.py::test_only_synchronous_categories_return_without_a_promise`. The browser half is out of scope.                                                                                    |
| B15. Kotlin/Wasm needed layouts from the linked module and a browser module artifact      | None for Node-API. The frontend preserves layouts for another target, but no Wasm emitter exists.                                           | Out of scope.                                                                                                                                                                                                               |

Of the synchronous calls in B14, runtime creation and a render session's driver
service, abandon, and dispose may wait on native threads. Each of them is a call
that a host makes rarely or that its rendering already schedules.

## Native changes

A native-core change that this binding needs is listed here with its reason, as
evidence against the claim that PR #760 removes the blocker. The native-core
ledger is empty: no file under `src/` or `include/` changed, as
`git diff --name-only 8661488cb -- src include` against the head of PR #760
shows.

The shared build configuration that changed outside the core:

| File                     | Change                                                                                                                      | Bearing on PR #760                                                                                |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| `CMakePresets.json`      | Toolchain files use `${fileDir}` instead of `${sourceDir}`, so a project that includes the presets gets the root toolchain. | None. It concerns a second CMake project, which any binding with its own native build would need. |
| `mise.linux.toml`        | The glibc floor check moves into `scripts/check-glibc-floor.sh` and takes a library path.                                   | None.                                                                                             |
| `.mise/plugins/ohos-sdk` | The plugin also unpacks the `ets` component and checks that it carries `es2abc`.                                            | None. It concerns building an ArkTS application, which PR #760 does not cover.                    |

The shared tooling that changed besides the build configuration:

| File                                                                        | Change                                                                                                                                                            |
| --------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `mise.toml`, `mise.lock`                                                    | Pinned Bun and Deno.                                                                                                                                              |
| `mise.ohos.toml`, `mise.ohos.lock`                                          | Pinned hypium, the OpenHarmony test runner.                                                                                                                       |
| `pnpm-workspace.yaml`, `pnpm-lock.yaml`                                     | The binding joins the workspace, with `node-api-headers`, `@types/node`, `typescript`, and `typedoc`.                                                             |
| `tools/bindgen`, `tests/bindgen`                                            | The TypeScript emitters and their registration, `async_hosts.py`, which the Dart emitter now shares, and the compile-and-run fixtures with a stub delivery queue. |
| `vite.config.ts`                                                            | Lint ignores the generator's TypeScript fixtures.                                                                                                                 |
| `ci/workflow.toml`, `ci/workflow.py`, `ci/snapshots.toml`, `ci/select.toml` | The suites, the OpenHarmony test command, the documentation snapshot input, and the coverage units' descriptions.                                                 |
| `scripts/check-conformance.py`, `scripts/coverage.py`                       | The binding joins their lists.                                                                                                                                    |
| `docs/`                                                                     | The API reference task and sidebar entry, the install section, the snapshot versions in the versioning page, and the support-matrix label.                        |

## Open work

- The binding does not support Windows as a host. On Windows x64, with WGL and
  with Vulkan, the addon's first `napi_create_double` call in a process
  delivered a heap address in place of its double: the render backend mask
  reached JavaScript as 9.198717177704e-312, and the next call, for the OpenGL
  context provider mask, delivered the right value. Render backend detection
  failed, and child processes hung, because their first double was a delivery
  kind that the runtime could not dispatch. Binding every delay-loaded import
  when the addon loads, with `__HrLoadAllImportsForDll("node.exe")`, did not fix
  it, and the cause is unknown. Windows ARM64 with Vulkan passed all 42 cases
  under each of Node, Bun, and Deno before the binding dropped Windows.
- The binding publishes no packages. npm keeps every published version, so
  snapshots wait for a feed on the documentation site, and releases for the
  first release.
- ArkTS has no package. An application builds the addon from a checkout and
  bundles the ArkTS entry.

## Browser and WebAssembly

A browser binding costs more than a new transport:

- Browsers have no Node-API.
- `mln_runtime_create` blocks, and returns a wrong-thread status on the browser
  main thread.
- The wasm32 ABI needs its own layouts, and the browser needs a linked module.
- Each runtime adds an executor thread to the Emscripten pthread pool.

Two choices keep a later port open. The generated facade reaches the addon only
through the interface in `generated_native.ts`, so another transport can
implement it. The addon is Node-API C, which emnapi implements on Emscripten,
and its one native-thread path is the wake shim, which an Emscripten task that
runs on the page thread can replace.
