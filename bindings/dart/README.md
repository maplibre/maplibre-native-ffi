# Dart binding

`maplibre_native_ffi` is the low-level Dart binding for the public MapLibre
Native C API. The package exposes explicit native handle lifetimes, copied value
types, copied runtime event spans, ordinary futures, resource callbacks, and the
render backend descriptors used by host integrations.

## Build and test

The repository-level mise configuration installs Dart and the native build
toolchain. From the repository root:

```bash
mise run //bindings/dart:test
mise run //bindings/dart:test linux-gnu-x64-vulkan
mise run //bindings/dart:build:mobile android-arm64-egl
mise run //bindings/dart:build:mobile ios-arm64-metal
mise run //bindings/dart:build:mobile ios-simulator-arm64-metal
```

The test task builds the selected CMake preset, points the build hook at its
install prefix, analyzes the package, and runs the Dart tests. The private raw
declarations in `lib/src/internal/c/maplibre_native_c.g.dart` are checked in so
Git and pub package consumers receive a complete library.
`mise run bindings:generate` writes them with the rest of the generated binding,
and `mise run bindings:check` fails on any diff.

The suite tests what the binding adds on top of the C API: handle ownership,
completions, queued callback registrations, generated value shapes, rendering
through a session, and loading and shutdown. `tests/conformance/dart.toml` maps
each shared conformance case to its test. Every test builds on the fixture in
`test/support/fixture.dart`, which closes each handle it opens in a teardown and
installs a resource provider that answers every request with an error, so no
test reaches the network. Render tests take their GPU context from
`mln_test_graphics` in `tests/graphics`. `test/log_test.dart` is the only suite
that installs process-global state, because `dart test` runs suites concurrently
in one process.

The mobile build task creates a temporary Flutter host, builds the selected
native preset, and verifies that Flutter packages its code asset. Device and
simulator iOS use separate presets because their dynamic libraries target
different Apple SDKs.

On macOS, the EGL target uses prebuilt ANGLE libraries that the native build
pads with extra Mach-O header space (`scripts/pad-macos-angle.py`), so Dart
native assets can rewrite their install names.

The native library reaches Dart as a code asset that `hook/build.dart` declares,
which is how the generated `@Native` declarations resolve it. The hook takes its
settings from `hooks.user_defines` in the root pubspec, as the
[install guide](https://maplibre.org/maplibre-native-ffi/install/#dart)
describes. This package's pubspec and the Flutter smoke host set `local_build`
to `.dart_tool/native`, which the test tasks link to the selected preset's
install prefix.

## Publishing

A published package downloads the native archives of its own release rather than
carrying them, because pub.dev has no per-platform packages. The release
workflow runs `mise run //bindings/dart:prepare-release <tag>` on the tagged
commit, which writes `hook/native_release.json` from the release's `SHA256SUMS`,
stamps the version into `pubspec.yaml`, and adds it to `CHANGELOG.md`. The
workflow then publishes with an OIDC token from the tag push. Git ignores the
pin file, and `.pubignore` keeps it in the package. `LICENSE` links to the
repository's license, and pub publishes the file that it names.

The test task runs `mise run //bindings/dart:check-publish`, which prepares a
copy of the package outside Git and checks it with `dart pub publish --dry-run`.

pub.dev enables automated publishing only for a package that exists, so a
maintainer publishes the first version by hand: run `prepare-release` for a
published release tag, then `dart pub publish` from this directory. Then, on the
package's admin page, enable publishing from GitHub Actions for
`maplibre/maplibre-native-ffi` with the tag pattern `v{{version}}` and the
`pub.dev` environment.

## Android host integration

The code asset carries the library on Android as it does everywhere else, so an
Android application packages the ABI it points the hook at and the platform
loader is never asked for one by name. The library also carries the Java code
that its TLS stack calls, so the application packages nothing else. The host
calls `mln_android_init` with its JNI environment and application context before
creating a runtime.

## Ownership and execution

Synchronous `close()` is idempotent. Runtime and map close are asynchronous; a
repeated release reaches native and reports `Status.invalidState`. An acquired
frame ends with `release()` or `dispose()`. Callback roots stay alive through
native teardown. Close child maps, render sessions, frames, and request handles
before their parent runtime. A scoped backend value is valid until its frame or
owner closes.

Close every handle explicitly. When the collector reclaims a handle that is
still open, a native finalizer disposes it, and the binding writes
`Leaked <type> native handle 0x<id>; close it explicitly.` to standard error.
That warning comes from a Dart finalizer, which runs from the event loop after
the collection, and it is skipped for handles that the isolate's shutdown
disposes.

A map creates a projection handle synchronously from its latest published
snapshot, and every later projection call is also synchronous. Any isolate can
make those calls, `close()` included, and each one runs on the calling isolate's
thread. A projection never observes map changes made after its creation, and it
remains usable after its source map and runtime close.

Map creation, commands, ordered queries, and lifecycle operations return
`Future` values, and a command copies its input before it returns. Snapshot
methods synchronously copy immutable state. Register a `RuntimeEventHandler`
with `setEventHandler` to receive event spans on the isolate's shared delivery
queue, and select event types with `setEventMask`. Events and completions use
one wake and drain loop.

Runtime, map, camera, and projection calls remain valid when Dart resumes an
isolate on another native thread after `await`. Attach a render session directly
from its map on the isolate that will own the graphics session.

Dart hosts render through core-worker sessions, on Metal, on Vulkan, or on an
EGL context that the session creates for itself. That EGL context shares nothing
with the host, so an EGL session renders to a surface or offers its texture as
readback rather than as frames to sample. A caller-driven session belongs to the
native thread that first services it, and `serviceDriverWork()` from any other
thread throws `MaplibreException` with `Status.wrongThread`. Dart can resume an
isolate on another thread after any `await`, so an isolate has no thread that it
can keep for such a session. A WGL texture shares the host's context, which only
a caller-driven session can drive, so a Dart host on Windows renders through
Vulkan.

A pending completion keeps its isolate alive, so every future that the binding
returns resolves before the isolate finishes. Callback registrations leave the
isolate free to finish: a wake, a resource provider or transform, a custom
source callback, and the log callback. A program that waits for one of those
callbacks keeps the isolate alive itself, for example with a `ReceivePort` that
it closes once the callback runs.

Resource-request completion is one-shot. Calling `complete()` or `close()`
releases the provider reference even when completion reports a native error.
Closing a request without completing it fails the request, so the load reports
an error instead of waiting.

Native code never waits on a Dart callback. Resource providers and the log
callback answer native code immediately and run on the registering isolate
later, as do cancel callbacks. An exception a callback throws goes to the
registering zone's error handler; a resource provider that throws also fails its
request. For the same reason, the render session attach options have no queue
lock: a lock must be held when its callback returns, so a Vulkan session's
graphics queue belongs to the session alone.

`ResourceRequestHandle.setCancelCallback()` registers one callback per request
that runs when MapLibre discards a request the provider left open. The binding
queues the callback to the isolate and zone that registered it. A request that
is already cancelled returns true, stores nothing, and never runs the callback.
Register, complete, and release such a request on that isolate. An exception the
callback throws goes to the registering zone's error handler.

Unsigned C `uint64_t` JSON values, feature identifiers, and camera transition
IDs use Dart `BigInt` so the complete native range is preserved. Native byte
views are copied into Dart-owned lists before a call returns; the scoped pointer
accessors that expose native addresses are explicitly unsafe and end with the
frame or handle that owns them.

Errors carry the generated `Status` and a copied diagnostic in one
`MaplibreException`. An unknown status retains its `rawValue`.
