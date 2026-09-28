# Validation of the shared binding compiler

This iteration builds and tests the native library from this checkout at
`build/macos-arm64-metal/install`. Earlier prototype notes that used the parent
checkout's installed library describe different evidence.

## Local results

The migration replaced obsolete handwritten API and converter tests with public
generated-API workflows. Test counts therefore differ from the earlier
prototype. Current runs include 82 generator tests, all three native CTest
targets, 182 .NET tests, 113 Swift tests, 168 Python tests, and Go race and
strict C-pointer checks. Kotlin passes 141 JVM and 139 Native tests. Rust passes
133 integration and 29 core tests. Zig passes 164 tests, with four tests
selecting other backends. Dart passes 80 tests with a clean analyzer. Native
teardown reentry and the GPU sibling-disposal race have direct passing
regressions.

Both Zig examples compile across Metal, OpenGL, and Vulkan. Metal readback
produces a nonuniform image. LWJGL and Compose build and complete bounded Metal
smoke runs. Android example Kotlin sources compile. Bounded GUI runs establish
startup and rendering evidence, not graceful shutdown.

C and all eight binding API references build. The documentation site builds 26
pages with all internal links valid. [Completion](completion.md) records the
latest integration state; full remote CI remains a separate gate.

The allocation-failure harness replaces allocation in the statically linked
native test executable. It proves the injector is active by rejecting an
allocating release, then verifies disposal admission still succeeds. It also
covers callback reentrancy, queued still-image cancellation, deferred parent
retirement, pending map creation, unrelated runtime progress, and rollback of
creation reservations when completion allocation fails.

The compiled value mutation test introduces nested values, a joint center mask,
a bit above 32 bits, and a renamed ABI-size field. It verifies omitted fields,
explicit zero values, default masks, and copy independence. Removing mask reset
makes that test fail. The native capture mutation adds a new result type in a
header and verifies copied data after overwriting its source, alignment,
present-empty versus absent data, and unclaimed owned-result disposal.

## First full CI corrections

The follow-up restores raw .NET and JVM declarations, corrects Android nullable
pointer decoding, recognizes its guarded reachability fence, and migrates the
remaining Rust and Python graphics fixtures and iOS example. The portable
callback arena supports OpenHarmony's standard library and preserves aligned
storage across growth. Queue closure releases existing records without
constructing an allocating temporary container.

Local follow-up validation passes 82 generator fixtures, deterministic complete
generation, all native CTest targets, 182 .NET tests, 79 Dart tests, 164 Python
EGL tests, Android device-test source compilation, and the Swift iOS simulator
example build. The Kotlin iOS simulator suite and ten additional runs of the
previously failing case pass. The original remote assertion remains unexplained;
full exception output is enabled for the next run.

The browser scheduler now treats queued immediate tasks as already due. Its
regression advances the clock between the scheduler cutoff and task inspection.
Reverting only the fix makes that regression fail; restoring it passes the
WebGPU native suite.

## Callback-copy measurement

`tests/bindgen/capture_benchmark.cpp` loads either library through the same
small host executable. Each completion contains 64 layer records with four
64-byte strings each. Seven samples each execute 20,000 create/copy/destroy
cycles, and a checksum verifies the copied payload is consumed.

On this macOS ARM64 host, the median changed from 5,717 ns to 1,473.2 ns per
completion, about 74% less time. A separate compiled test verifies exactly one
heap allocation for the captured completion and all its nested storage. This
measures the callback-copy workload, not rendering throughput or frame rate.

## Limits

Full remote coverage remains active. Follow-up commit `edb4a6593` passes all
Android and OpenHarmony targets and Android packaging. Windows native and Rust
runtime suites pass, with a subsequent test-fixture Clippy error corrected
locally. The follow-up also corrects a Go test counter that raced unrelated
registrations. An intermittent browser map-close timeout reproduced under
single-CPU Linux Chromium. The worker could destroy its run loop before `stop()`
sent a redundant second wake; the follow-up removes that access and tests
immediate destruction. Both complete browser suites and strict Clippy pass, as
do 100 fresh Linux Chromium runs under the reproducing single-CPU schedule. The
no-allocation guarantee covers admission and scheduling; teardown work on other
threads may allocate. Runtime and platform boundaries are described in
[completion boundaries](blockers.md).
