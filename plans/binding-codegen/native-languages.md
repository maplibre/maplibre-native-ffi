# Native language binding generation

Rust, Go, Swift, and Zig now generate all 264 public C declarations. Rust, Go,
Swift, and Zig have passing local binding suites.

## Rust

The public binding uses generated operations and values. Handwritten code owns
completion delivery, handle reservations, native view scopes, diagnostic
capture, and callback policy. The shared core retains these runtime mechanisms
for Python. Handwritten camera, style, options, event, graphics, and resource
value conversion layers are removed.

Owned completion results adopt their native handle before delivery. Immediate
owners with asynchronous readiness return a handle and completion tuple.
Borrowed batches reserve the owner while copying. GPU getters execute a host
closure inside the native view gate, including protection from sibling frame
disposal. Explicit release reserves ownership without holding a mutex across C;
rejection and panic restore the owner. Callback policy intersects nested native
contracts and defers finalization away from callback stacks.

Local macOS ARM64 Metal validation passed 133 binding integration tests and 26
focused core tests. Strict Clippy passes. The Metal example compiles. Five
native emitter fixtures pass, including Rust compilation after header parameter
names collide with internal generator locals. Repeated generation produces
identical output after formatting. Other target runtime results require CI.

## Zig

The generated public module replaces the handwritten camera, style, graphics,
resource, logging, and per-handle conversion modules. One owner runtime handles
transactional release, copied-handle invalidation, parent retention, borrowed
copy reservations, provider decisions, and finalization outside callback stacks.
Callback registrations keep separate caller and native roots, including inline
native release. Cancellation registrations use generation tokens so native
callbacks cannot dereference a released host root. Graphics views use the native
frame scope gate.

All ordinary generated functions compile against the installed Metal artifact.
Public workflow probes pass creation, rejected release, closed copied handles,
event snapshots after batch release, inline provider completion, repeated inline
close, and selective callback reentry. These probes also caught and fixed an
arena ownership transfer that preceded result allocations. The main agent
migrated both examples: all three desktop backend builds passed, Metal readback
produced a nonuniform image, and interactive Metal modes ran without errors.

The full Metal suite passes 164 tests, with four tests skipped because they
target Vulkan or OpenGL. The suite exercises the generated public API for
camera, projection, styles, source data, queries, offline regions, resources,
logging, events, rendering, and lifecycle. A scoped GPU regression checks
rejected release during a read, sibling disposal, invalidation of later reads,
and scope cleanup when the host callback returns an error. Vulkan and OpenGL
test binaries compile; those backend runtimes still require their native CI
artifacts.

All 82 generator fixtures pass. Public parameters use header names with separate
internal temporaries. Nested retained inputs fail closed without an adapter;
nullable buffers preserve absent versus present-empty values. Zig callback
contexts are explicit borrowed pointers, and the native retirement callback
releases accepted contexts. Rust callbacks capturing their owner use a weak Arc
reference to avoid a reference-count cycle.

## .NET callback ownership follow-up

Persistent descriptor registrations use weak generation tokens and owner-held
roots. A callback can capture its owning map without keeping that map globally
reachable. Successful close and finalization retain accepted roots through the
native release callback; late admission into a retiring owner uses the same
retirement list. Global logging retains its process-wide registration
explicitly. Constructor and attachment roots belong to the newly adopted handle.
The host suite passes 182 tests, including a captured-map collection workflow
and scoped registration retirement, late acceptance, and stale callback
containment.

## Portability and value-shape review

The shared scalar carrier follows typedef chains before Clang expands them to
host ABI types. A cross-host mutation fixture now requires all eight emitters to
produce identical source for equivalent `long` and `long long` expansions of
fixed-width integers, pointer-sized counts, aliases, and enum carriers. Go's
compiled fixture also checks absent, present-empty, and nonempty counted text
and nullable arrays. Rust input arrays preserve nonnull empty pointers; Python
nullable arrays map `None` to a null pointer.

Batch views expose owned event messages while the native stride and message
arena remain conversion controls. Rust, Zig, and .NET omit the duplicated arena
copy. The public Rust sibling-frame regression found a native teardown race
while this cleanup was being validated. The native retirement admission repair
passes the full Rust suite and 25 repeated focused runs without retries in the
test.

## Kotlin Native capture lifetime check

The callback capture test now constructs and submits source options in a normal
function, keeping those arguments out of the test's coroutine frame. It awaits
map close completion before checking teardown and collects once at each release
boundary instead of polling the collector. All 139 macOS Metal native tests
pass. A mutation that kept a released callback root on its owner list failed
immediately after source removal, with two live captures instead of one; the
production release implementation was then restored.

A standalone Kotlin 2.4.20 debug executable reproduced the distinction without
native code: a callback passed directly before an inline `await()` stayed live
after its owner was cleared and GC ran, while construction in a normal helper
released it. Both forms collected the callback when `await()` suspended.

The Android API 26 capture failure had a different cause: `System.gc()` deferred
collection until a finalization request. The test now calls
`Runtime.getRuntime().gc()` once and awaits the cleanup action. The image-upload
lifetime test uses the same collector entry point. Android cleanup registrations
clear their phantom referent and action after dequeueing, so an idle cleanup
worker cannot retain the last cleaned graph. All 127 minified tests pass on the
isolated API 26 ARM64 emulator with EGL. Temporary thread, weak-reference, and
logging probes were removed.

## Android Vulkan build and package matrix

The Kotlin/Native binding and Vulkan runtime KLIBs build for ARM32, ARM64, and
x64 with the installed native prefixes:

```sh
for preset in android-arm-vulkan android-arm64-vulkan android-x64-vulkan; do
  mise -E android run --skip-deps //bindings/kotlin:build "$preset"
done
```

The multi-ABI JavaCPP binding, Vulkan runtime AAR, and release example APK also
build with all three ABIs:

```sh
mise -E android run --skip-deps //bindings/kotlin:android-build vulkan armeabi-v7a,arm64-v8a,x86_64 --prebuilt
mise -E android run --skip-deps //examples/android-map:build vulkan armeabi-v7a,arm64-v8a,x86_64 --prebuilt
```

Archive inspection confirms JNI and native libraries for all three ABIs in the
AARs and APK. The ARM32 and ARM64 shared libraries first needed restoration with
`cmake --install build/<preset> --component native`; the Flutter hook's shared
output-directory cleanup had removed them. Kotlin packaging does not mutate the
native prefixes.

With `ANDROID_SERIAL` selecting the physical Pixel 8 on API 37, the final
minified Vulkan instrumentation suite passes all 127 tests after the callback
cleanup fix:

```sh
mise -E android exec --no-deps -- ./gradlew \
  -Pmaplibre.android.backend=vulkan \
  -Pmaplibre.android.abis=arm64-v8a \
  -Pmaplibre.android.prebuiltBuildRoot=build \
  -Pmaplibre.android.testMinify=true \
  :bindings:kotlin:connectedAndroidDeviceTest
```

## Kotlin rendering fixtures

Common rendering tests require a fixture for the selected backend. Fixture
initialization failures now fail the test instead of returning without running
its assertions. JVM fixtures support Metal, Vulkan, EGL, and WGL; Android device
fixtures support Vulkan and EGL. Desktop Kotlin/Native uses the shared driver
bootstrap in `tests/graphics` for Vulkan and EGL, and Apple targets retain their
Metal fixture. Android Kotlin/Native remains a compile-only test target and
fails explicitly if its rendering fixture is invoked.

The shared bootstrap creates graphics driver resources only. Every MapLibre
operation still goes through the public Kotlin binding. EGL caller-thread tests
create a shared current context; the bootstrap's dedicated-context path remains
available to other bindings. Test JNI libraries are packaged only in Android's
device-test APK. Test teardown awaits both map and runtime retirement.

The macOS Metal, Vulkan, and EGL suites each pass 141 JVM tests and 139 Native
tests. The minified Pixel 8 Vulkan suite passes all 127 tests, including the
previously bypassed generic rendering, readback, projection, query, and GPU view
lifetime workflows. WGL compiles locally; its execution requires Windows CI.
Linux uses the same desktop Native bootstrap and JVM Vulkan fixture, with
runtime validation delegated to the Linux matrix.

Dart now selects a WGL fixture on Windows and applies its scoped GPU view checks
to OpenGL texture views. A temporary Win32 window initializes the driver within
one synchronous call; a WGL pbuffer, device context, and share context remain
owned across asynchronous test steps. No window survives an `await`. The fixture
fails if the driver's pbuffer extensions are unavailable. Local Dart analysis
and kernel compilation pass, along with all 80 Metal host tests; Windows
execution remains for CI.
