# TypeScript binding

`@maplibre/native-ffi` is the low-level TypeScript binding for the public
MapLibre Native C API on Node.js, Bun, Deno, and ArkTS on OpenHarmony. One
package and one native addon serve all four hosts, on Linux with glibc, macOS,
and OpenHarmony. The binding does not support Windows yet: on Windows x64, the
addon's first double in a process reached JavaScript as a heap address, for a
reason that is unknown. The package exposes explicit native handle lifetimes,
copied value types, Promises for completions, callbacks on the event loop, and
the render backend descriptors that host integrations use.

The binding compiler generates the public API in `src/generated_*.ts` and a C
Node-API addon in `native/generated_*.c`. The handwritten runtime is the wake
shim and the value helpers in `native/realm.c` and `native/runtime.c`, and the
delivery dispatch and handle state in `src/internal/`.

## Build and test

The repository-level mise configuration installs Node, Bun, Deno, and the native
build toolchain. From the repository root:

```bash
mise run //bindings/typescript:test linux-gnu-x64-egl
mise run //bindings/typescript:test linux-gnu-x64-vulkan node
mise run //bindings/typescript:pack linux-gnu-x64-vulkan
mise -E ohos run //bindings/typescript:test ohos-x64-egl
```

The build task builds the native preset, then the addon from
`native/CMakeLists.txt`, whose presets include the root presets, so the addon
uses the same toolchain and target as the native library. The addon embeds the
complete static archive and exports only `napi_register_module_v1`. The test
task runs the suite under Node, Bun, and Deno in turn, or under the one runtime
that its second argument names. For an OpenHarmony preset, it builds, signs, and
installs a test HAP on the emulator, which
`mise -E ohos run //:ohos-emulator:boot` starts, and runs the suite on ArkTS.
The `api` task writes the TypeDoc reference to `build/typedoc`.

Node 24 strips types, and Bun and Deno run TypeScript directly, so the suite
runs the sources without a build. The pack task, with an optional `--version`,
bundles the facade with `vp pack` and writes three tarballs to `build/npm`:

- the facade, `@maplibre/native-ffi`, which carries the bundle, because Node
  strips types only outside `node_modules`;
- the backend package, such as `@maplibre/native-ffi-vulkan`, which pins one
  addon package per platform through `optionalDependencies`;
- the addon package, such as `@maplibre/native-ffi-vulkan-linux-x64-gnu`, which
  declares its `os`, `cpu`, and `libc` and carries the addon and the native
  licenses.

EGL presets pack into `@maplibre/native-ffi-opengl`. The test task packs the
same way, installs the tarballs into a temporary consumer project with npm, and
type-checks the consumers in `test/fixtures/consumer` against the packed
declarations with no host types. The loading case runs those consumers under
each runtime.

The suite tests what the binding adds on top of the C API: handle ownership,
completions, callback delivery across realms, generated value shapes, rendering
through a session, loading, and process exit.
`tests/conformance/typescript.toml` maps each shared conformance case to its
test. Cases register through `test/support/harness.ts`, which runs one case list
under `node:test`, `bun:test`, `Deno.test`, and hypium. Render tests take their
GPU objects from `mln_test_graphics` in `tests/graphics`, through the test-only
addon in `test/support/graphics_addon.c`.

## Distribution

The binding publishes no packages yet, so a project builds it from a checkout
with the build and pack tasks and installs the tarballs. The snapshot channels
of the other bindings keep only the latest snapshot, and npm keeps every version
it publishes, so the binding waits for a release, or for a feed on the
documentation site like the Python and .NET ones.

## Loading

The Node, Bun, and Deno entry resolves the installed backend package, then the
addon package of that backend for the platform and architecture, from the
backend package's own directory. The facade declares the backend packages as
optional peer dependencies, so a strict layout such as pnpm's links the backend
package where the facade resolves it. `MAPLIBRE_NATIVE_FFI_BACKEND` names the
backend when more than one backend package is installed, and
`MAPLIBRE_NATIVE_FFI_ADDON` names an addon file directly, relative to the
working directory. Deno loads the binding with `--allow-ffi`, `--allow-read`,
and `--allow-env`, from a local `node_modules` directory. A realm loads one copy
of the binding: a second copy fails to load, because both would share the
addon's delivery. The loader compares the addon's ABI digest with the facade's
before any other call, and throws an `AbiVersionMismatchError` when they differ.

ArkTS imports `libmaplibre_native_ffi.so` by library name from the application's
`libs/<abi>` directory, through the entry in `src/arkts.ts`. The application
ships the SDK's `libc++_shared.so` beside the addon.

## Ownership and execution

Each owner class releases its handle through the calls that its C API has.
`RuntimeHandle` and `MapHandle` have an asynchronous `close()`, which resolves
once native code retires the handle, and a synchronous `dispose()`.
`RenderSessionHandle` has a synchronous `close()` and `dispose()`, and
`AcquiredFrameHandle` has `release()` and `dispose()`.
`GeojsonSourceDataHandle`, `MapProjectionHandle`, `RenderFrameBatchHandle`,
`ResourceRequestHandle`, and `ResourceTransformRequestHandle` have a synchronous
`close()`. Owners define `Symbol.asyncDispose` and `Symbol.dispose` where the
host defines those symbols, so `await using` closes them; ArkTS defines neither.
A second synchronous close does nothing. Every other call, including an
asynchronous release after an accepted one, reaches native code, which reports a
released handle as stale. Close child maps, render sessions, frames, and request
handles before their parent runtime. A borrowed frame view, such as the
`ScopedVulkanTextureFrame` that `AcquiredFrameHandle.withVulkanTexture()` passes
to its callback, is readable only while that callback runs.

Close every handle explicitly. When the collector reclaims an owner that is
still open, a Node-API finalizer disposes its handle, and the binding reports
the leak. Node, Bun, and Deno receive it as a process warning of type
`ResourceWarning` with code `MAPLIBRE_NATIVE_FFI_LEAK`, which
`process.on("warning")` and `--no-warnings` filter. ArkTS receives it through
`console.warn`.

Map creation, commands, ordered queries, and lifecycle operations return
Promises. A failed call throws, or rejects with, a `MaplibreError` that carries
the status and the call's own diagnostic. A command resolves with a
`CommandCompletion` that reports its disposition as data. The `discarding`
member of a map or render session runs the same commands without a completion:
each returns once native code accepts it, and a later completion on the same map
follows it. Handles, 64-bit integers, and graphics addresses are `bigint`
values.

Every callback reaches the realm's event loop after the native call that raised
it has returned: runtime events, log records, provider requests and cancels,
resource transforms, camera transition ends, and custom source tile fetches and
cancels. Native code answers each synchronous decision itself, and a provider
request or a transform request arrives as an owner that the host answers later.
The HTTP header transform is a native rule table. The render session attach
options have no queue lock, so a Vulkan session's graphics queue belongs to the
session alone. A callback that throws reaches `reportError` where the host
defines it, and `console.error` otherwise; a provider callback that throws also
fails its request.

A pending binding Promise keeps the process alive, and a callback registration
or a wake keeps nothing alive. To wait for a callback, call the `wake` member of
an `AwaitableWake` from it, or pass that member as a render session's frame
wake, and await `next()`, which counts as a pending Promise until the wake
arrives. A render session's frame results stay queued until
`RenderSessionHandle.drainFrameResults()` reads them.

A realm runs on one operating-system thread, so the binding can drive a
caller-driven render session: make its context current on the JavaScript thread
and call `serviceDriverWork()` when its driver-work wake arrives.

## Runtime

A realm is one Node-API environment: the main thread or one worker. The addon
keeps no state per callback. Each realm has one delivery queue, which the first
completion or callback registration creates, and every completion and queued
registration of the realm names that queue and a token. Native code copies each
delivery into the queue and wakes it when it becomes nonempty. The TypeScript
half roots each pending completion and registered host value by its token, and
drops a registration's root when native code retires it.

The wake shim in `native/realm.c` is the one path from a native thread to a
realm. It holds the realm's threadsafe function, a lock, and an open flag. The
queue's wake posts token 0, and a render session's frame or driver-work wake
posts its own token, and posts it again when native code releases the wake. A
post calls the threadsafe function under the lock while the shim is open, and
never waits for JavaScript. The function runs on the realm's JavaScript thread:
for token 0, the realm drains the queue into a batch and dispatches each
delivery by its token and type, so a Promise never resolves inside a native
call. The addon converts each payload with the generated converters; a
completion's value converts through the decoder that its pending entry names,
and a provider or transform request moves to its owner with
`mln_delivery_batch_take()` before the host callback runs. Releasing the batch
disposes what the realm did not take. The threadsafe function is referenced only
while a binding Promise is pending in the realm.

A realm's teardown closes its shim through two paths, and the first to run wins.
The environment cleanup hook closes the shim, aborts the threadsafe function,
releases the queue, which disposes every undelivered owned value and fails every
undelivered request, abandons the realm's render sessions, and finalizes its
other owners. The threadsafe function's finalizer closes the shim too. The addon
registers its hook after it creates the threadsafe function, so the hook runs
before the host frees the function. Once the shim is closed, a native thread's
post does nothing, and native code refuses pushes to the released queue, so a
terminated worker leaves every other realm working. The addon call that releases
a handle also ends its owner token, so a teardown that runs after a close never
finalizes the released handle. The addon never unloads, because native threads
can call into it after any realm ends.

On Node, Bun, and Deno, the first runtime in the main realm installs a
synchronous process `exit` listener, and on Deno an `unload` listener as well.
It closes every realm's shim and abandons every live render session before
`exit()` runs. Node's `process.exit()` skips the environment cleanup hooks,
Deno's `Deno.exit()` skips both those hooks and the `exit` listeners, and Mesa's
EGL can register its exit handler after any handler that the addon registers, so
the listener is the last point at which graphics calls can end. A worker's
`exit` event ends only that worker, whose own teardown releases its queue. ArkTS
has no exit event, and a worker realm ends its sessions in its own teardown.

Two host behaviors leave a small fixed leak for each terminated worker. Deno
2.9.7 never runs a threadsafe function's finalizer, so the cleanup hook alone
closes the shim, and the shim reference that the finalizer would release stays
held. Bun 1.4.3 and Deno 2.9.7 never finalize an external whose JavaScript
object is still alive when a worker ends, so each owner that the worker left
open keeps its small native record after the cleanup hook has disposed its
handle.
