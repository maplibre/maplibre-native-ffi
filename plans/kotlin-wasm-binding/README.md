# Add a Kotlin/Wasm browser target to the Kotlin binding

The Kotlin binding gains a `wasmJs` target for browsers. Kotlin runs on the page
thread, where Compose for Web runs, and calls a prelinked Emscripten module of
the C API. The generated common API stays one API over a primitive native-call
shim per platform, and wasm becomes the fourth shim beside FFM, cinterop, and
JNI.

The page cannot run code on a native thread, so it is an async-only host, as a
Dart isolate and a Node-API realm are. It takes the same path that they take:
every registration with a queue delivery names the core delivery queue, and the
browser module's wake shim only tells the page to drain it. The shim holds no
per-callback state and copies nothing. `tools/bindgen/async_hosts.py` chooses
each registration's form for Kotlin as it does for Dart and TypeScript.

## Decisions

| Question         | Decision                                                                                                                                                                                                                                                                                                                                                  |
| ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Host thread      | Kotlin and the Emscripten module run on one agent, the module's main runtime thread. The page thread is the tested host. A dedicated Worker that hosts both follows the same rules.                                                                                                                                                                       |
| Native transport | `kotlin_native.py` generates one top-level `js()` extern per C function and one C wrapper per function in the JNI pattern, which takes and returns records by address. The externs reach the module through a `Symbol.for` global. Pointers and sizes cross as `Int` through a checked narrowing; 64-bit integers and handles cross as `Long`.            |
| Module artifact  | A CMake project in `bindings/kotlin/wasm` whose presets include the root presets. It links the installed archives and link flags, the generated wrappers, and the wake shim into an ES module factory with its `.wasm`, with the JavaScript graphics library of the build's backend. A test variant adds the canvas fixtures and a contention probe.      |
| Module loading   | `suspend fun Maplibre.loadModule(url, pthreadPoolSize)` imports the module, checks the ABI version, and creates the module's delivery queue. Callers share one load. Until it returns, every generated call fails with an invalid-state status.                                                                                                           |
| Memory           | Memory growth stays off, at the installed fixed 512 MB. Scalars go through one cached `DataView`; bulk copies stage through Kotlin's own memory in one JavaScript copy. An address size of 4 selects the existing ILP32 layouts.                                                                                                                          |
| Delivery         | One `mln_delivery_queue` per module. A queue-form registration and every completion write its queue and a `NativeRoots` token in place of their callbacks. The page drains each batch and runs the same generated `Upcalls` site that a threaded target's native thread calls, chosen by the delivery's type and the token's registration (`Deliveries`). |
| Wake shim        | `mln_kw_runtime.c` posts each wake to a private `em_proxying_queue` that the page runs only from its event loop. Token 0 is the delivery queue's wake; any other token is an `mln_wake` registration's, posted once per wake and once more for its release. A thread start posts its call's status.                                                       |
| Runtime creation | `MLN_BINDING("blocks=thread_start")` makes the generated `runtimeCreate` a `suspend` function on every target. Threaded targets call it inline. Wasm runs the call on a detached pthread. The wait does not cancel, and a cancelled caller disposes the runtime that arrives.                                                                             |
| Callback API     | Common code takes each registration in the form that every target serves. A threaded target serves the same types through callbacks that return what queue mode answers. The `threadedMain` source set adds direct forms for the answers that a callback computes on its thread.                                                                          |
| Finalizers       | `FinalizationRegistry` callbacks run as page tasks and dispose directly. Nothing abandons sessions on unload: the browser ends every agent of a page together, and abandoning on `pagehide` breaks back-forward cache restore.                                                                                                                            |
| Graphics imports | `BrowserGraphics` imports a page's `GPUDevice`, `GPUCanvasContext`, `GPUTexture`, and `GPUTextureView`, and registers a `WebGL2RenderingContext`, as the handles that render target descriptors take. The host closes each import after its sessions.                                                                                                     |
| Test portability | `commonTest` runs on wasm. `runSuspendTest` returns `TestResult`: `runBlocking` on threaded targets, and `runTest` over `Dispatchers.Default` on wasm. `threadedTest` holds the tests whose subject is a second Kotlin thread or a direct form.                                                                                                           |
| Test runner      | Kotlin Gradle's `wasmJsBrowserTest` on Karma with cross-origin isolation, the module served beside the tests, and Chromium flags for the module's backend. A canary task passes only when a deliberately failing test fails the run.                                                                                                                      |
| CI               | The Emscripten jobs run the suite on both presets against the packaged native artifact, and the WebGL job also runs the canary.                                                                                                                                                                                                                           |

Each registration form maps to one common type and, where a callback decides
native's answer, one direct type:

| Registration                                                                   | `async_hosts` form | Common type                                          | `threadedMain` direct type               |
| ------------------------------------------------------------------------------ | ------------------ | ---------------------------------------------------- | ---------------------------------------- |
| Log handler                                                                    | queue              | `LogHandler`, whose callback returns `Unit`          | `DirectLogHandler`, which returns `UInt` |
| Resource provider                                                              | queue              | `ResourceProvider`, which keeps every routed request | `DirectResourceProvider` with a decision |
| Resource transform, request cancel, camera end, custom sources, runtime events | queue              | The callback types                                   | None                                     |
| HTTP header transform                                                          | rules              | `HttpHeaderTransform` with header rules              | `DirectHttpHeaderTransform`              |
| Wake                                                                           | wake               | `Wake`                                               | None                                     |
| Queue lock                                                                     | disabled           | `QueueLock`, which Kotlin/Wasm refuses               | None                                     |

A queue-shaped callback's site answers what queue mode answers: a decision's
accepted outcome, or the callback's `queue_result`, which the log callback's
header annotation names. The site owns each handle that the call passes with
ownership before it looks up the registration, and releases the handle when the
callback throws or the registration is gone. The page's drain takes the handle
from its batch before it runs the site, so both targets end an orphaned request
the same way.

The rejected alternatives cost more than they remove:

| Candidate                                                   | Consequence                                                                                                                                     | Decision                     |
| ----------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------- |
| Kotlin on a `-sPROXY_TO_PTHREAD` pthread that may block     | Compose for Web runs Kotlin on the page, so the map would need a second Kotlin module and a message layer.                                      | Rejected.                    |
| Wakes on the system proxying queue                          | The page runs the system queue inside every futex spin of a C call, so a wake would run Kotlin inside that call.                                | Rejected; a private queue.   |
| Queue mode on every target                                  | Kotlin matches Dart and TypeScript exactly, but every callback gains a hop to another thread, and a threaded host loses computed answers.       | Rejected.                    |
| Direct forms in common code that fail on wasm               | One API on every target, but the failure moves from compile time to run time.                                                                   | Rejected.                    |
| A wake shim that coalesces wakes per registration           | Fewer page tasks, at the cost of per-registration state in the shim. The delivery queue already wakes only when it goes from empty to nonempty. | Rejected; one task per wake. |
| A handwritten `createRuntime` that copies options to a lane | A redundant API, and a copy that drifts when the options struct gains a field.                                                                  | Rejected; an annotation.     |
| Memory growth                                               | Another thread's growth leaves the page's heap views stale, and a check on every access costs about seven times the access.                     | Rejected.                    |

## Page-thread rules

- Kotlin never waits for a thread to start. A thread that the page starts runs
  only after the page yields when the pool is empty.
- Every wake reaches the page through the shim's private queue, which the page
  runs from its event loop. The page runs the system queue inside a futex spin,
  and `DeliveryTest` shows that no wake runs inside a spinning call.
- The remaining waits on the page wait for threads that already run: brief
  mutexes inside C calls, a session abandon that waits for tile work, and a
  request release that waits for a running cancel callback.
- A host releases a request before it waits for the request to retire, because
  no other thread can release it.

A page that loads the module must be cross-origin isolated, because the module's
threads share its memory.

## WebGPU on the page

A WebGPU session is driven from the caller's graphics thread, and emdawnwebgpu
keeps each WebGPU object in the realm of the agent that created it. The page is
Kotlin's thread, so the page requests the device, and the suite imports it
through `BrowserGraphics`. A readback submits its texture copy and the copy's
buffer map in one driver service, and the service returns. Chromium runs the map
callback when the page yields to its event loop, and the next service completes
the readback. Headless Chromium loses every WebGPU device of a page once a
WebGPU canvas renders, so the WebGPU run gives `CanvasSurfaceTest` a page of its
own.

## Native changes

| File                                                                                                                   | Change                                                                                                                                                        |
| ---------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `include/maplibre_native_c/runtime.h`                                                                                  | `MLN_BINDING("blocks=thread_start")` on `mln_runtime_create`. `MLN_BINDING` expands to nothing, so the library is unchanged.                                  |
| `include/maplibre_native_c/logging.h`                                                                                  | `queue_result=1` on `mln_log_callback`: the answer that queue mode gives, which the schema requires of a delivered callback with a result and no decision.    |
| `src/render`, `include/maplibre_native_c/render_*.h`, `texture.h`                                                      | A driver result can finish on a later driver service, and the WebGPU readback finishes on the service after its buffer map, instead of sleeping through JSPI. |
| `tests/native/abi/render/main_thread_readback_webgpu.c`, `tests/native/abi/resources/main_thread_release_emscripten.c` | Readback from the browser main thread, and a request release on it while a cancel callback runs.                                                              |

## Open work

- The binding publishes no wasm KLIB and no module package. A module ships per
  backend, as the TypeScript addon does.
- `loadModule` imports the module by URL, so a host serves the `.mjs` and
  `.wasm` as static files. Bundlers that rewrite pthread worker URLs are
  untested.
- The browser's fetch follows redirects itself, so the HTTP header rules reach
  no request on wasm.
- A dedicated Worker host follows the page-thread rules, and no test runs one.
- CI runs Chromium only. Transferred-canvas WebGL in a worker needs Safari 17 or
  newer.
