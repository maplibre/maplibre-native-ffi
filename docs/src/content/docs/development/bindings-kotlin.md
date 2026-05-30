---
title: Kotlin Binding Conventions
description: Language-specific implementation conventions for Kotlin bindings.
---

Resources:

- Tracking issue:
  [#47](https://github.com/maplibre/maplibre-native-ffi/issues/47)
- [Kotlin Multiplatform project structure](https://kotlinlang.org/docs/multiplatform/multiplatform-discover-project.html)
- [Kotlin/Native C interop](https://kotlinlang.org/docs/native-c-interop.html)
- [Kotlin/Native definition files](https://kotlinlang.org/docs/native-definition-file.html)

## Architecture

The first Kotlin binding target is Kotlin/Native. It exposes a safe low-level
API over the public C API and keeps generated cinterop declarations private.
This page describes the Kotlin/Native concrete choices that refine the shared
[binding conventions](/maplibre-native-ffi/development/bindings/).

Build `nativeMain` with Kotlin/Native `cinterop` against the public umbrella
header. Treat successful cinterop generation and compilation as the
Kotlin/Native bindability check for the C headers. Keep the generated package in
an internal namespace, and wrap it through small internal support packages for
status conversion, diagnostics, descriptor materialization, memory helpers,
callback state, and native-library loading.

Kotlin/Native is the reference binding implementation for now. Prefer idiomatic
Kotlin public APIs over matching the existing Java FFM or JNI surface. Build
`nativeMain` without leaking cinterop or platform pointer types into public API.
When a shared `commonMain` layer appears, put copied value models, exceptions,
and platform-neutral API there; keep cinterop and platform bridge code in
platform source sets. Adapters above this layer own coroutine, scheduler,
rendering, and application policy.

## Public Surface

Long-lived native objects use the shared `Handle` suffix. Public operations stay
close to the C model and shared binding names unless idiomatic Kotlin spelling
is clearer. Prefer Kotlin property and nullability conventions when they do not
change the low-level contract.

C option structs become Kotlin descriptor classes or data classes. Mutable
field-mask descriptors use nullable public properties; optional validation lives
in property setters where needed. Immutable copied values use `data class` types
where value equality is useful. Internal materializers write C `size` fields,
masks, string views, arrays, and nested descriptors; callers set semantic fields
only.

Closed C enum domains map to Kotlin enums with explicit raw conversions. Output
domains that may grow preserve unknown raw values with a stable wrapper or
`Unknown` case. C bit masks become purpose-built value types or enum sets.
Generated C constants and cinterop enum representations stay internal.

Represent backend-native addresses with a public `NativePointer` value. It is a
borrowed opaque address, grants no memory access, and transfers no ownership.
Public APIs use it only where the C API accepts opaque backend handles. Internal
Kotlin/Native code converts between `NativePointer`, `COpaquePointer?`, and
`NativePtr` at the cinterop boundary; `NativePtr` stays out of public APIs.

Use signed `Int` and `Long` public values at the binding boundary, then validate
before converting to unsigned native fields. Reject negative sizes, counts, enum
sentinels, and unsigned numeric inputs that do not explicitly preserve native
bit patterns with clear exceptions instead of wrapping them.

Use `Long` for native `uint64_t` values that need Java-analogous API shape.
Preserve the bit pattern for values that may use the full native range, such as
`JsonValue.UInt`, `FeatureIdentifier.UInt`, offline operation IDs, render frame
generation IDs, frame IDs, Metal pixel formats, byte ranges, and native pointer
addresses. Document those fields as bit-pattern values on the public API.

## Handles, Status, and Threading

Each handle stores the native pointer, release state, parent references needed
for native validity, Kotlin callback state, and optional leak context.
Successful `close()` releases the native object once and marks the wrapper
closed; later `close()` calls no-op. Public releasable types implement
`AutoCloseable` so callers may use stdlib `use { }` or explicit `try`/`finally`.
If native destruction reports a status, `close()` reports that status through
the public error mechanism and leaves the handle live when a retry is valid.

Public fallible operations throw Kotlin `MaplibreException` subclasses that map
one C status category to a stable Kotlin type and carry the copied native
diagnostic. Optional `Result<T>` helpers may wrap the throwing API. Read the
thread-local diagnostic immediately after a non-OK C status on the same thread.
Validate Kotlin-owned state before the C call: closed wrappers, active scoped
borrows, one-shot resource completion, invalid string shapes, and callback
lifetime.

Owner-thread-affine calls run on the calling native thread. The binding does not
dispatch internally. Native `MLN_STATUS_WRONG_THREAD` becomes the Kotlin
wrong-thread error. Adapters above this layer may add coroutines, UI-thread
handoff, or owner-thread executors.

Retain parent wrappers while children need native validity.
`MapProjectionHandle` follows the shared exception: after creation it owns a
standalone snapshot and does not depend on the source `MapHandle` for native
validity.

## Kotlin/Native Memory

Kotlin/Native cinterop APIs are experimental. Keep
`@OptIn(ExperimentalForeignApi::class)` in `nativeMain` internals and actual
implementations, not in public common APIs. Public APIs do not expose
`CPointer`, `COpaquePointer`, `CValue`, `CValuesRef`, `NativePlacement`,
`StableRef`, platform C type aliases, generated struct classes, or `NativePtr`.

Use storage by lifetime:

- `memScoped` for temporary structs, strings, out parameters, and scratch
  buffers used by one C call;
- `nativeHeap` for storage whose address outlives one call and is released by
  the owning wrapper;
- `ByteArray.usePinned` or `refTo()` for primitive arrays borrowed for one call;
- explicit native buffers for reusable off-heap readback or upload storage.

Cinterop may map `const char*` parameters to `String`. Use that conversion only
when the C API consumes or copies the string before returning and the binding
has rejected embedded `NUL` characters. Configure `noStringConversion` where
automatic conversion would hide pointer lifetime, byte length, or nullable
pointer semantics. Explicit-length string views materialize UTF-8 bytes plus a
byte length at the call boundary.

Native result, snapshot, and list handles stay internal. Readers copy borrowed
strings, arrays, events, diagnostics, and payloads into Kotlin-owned values,
then release the native handle exactly once even when copying fails.

## Callbacks

Create C callback entry points with non-capturing `staticCFunction`. Pass
callback state through C `user_data` as `StableRef` values. Create each
`StableRef` when registering the callback and dispose it exactly once after
native code can no longer call it. Store Kotlin callback objects strongly for
the C owner scope: process-global logging, runtime-scoped resource providers,
map/style-scoped custom geometry sources, or request-scoped completion handles.

Callbacks may arrive on MapLibre worker, network, logging, or render-related
threads. Callback state must be safe for those threads and must return quickly.
Callbacks that need map or runtime methods hand work back to the owner thread
before calling thread-affine APIs.

Catch Kotlin exceptions inside every callback trampoline and convert them to the
C callback's documented failure behavior. Exceptions never cross native frames.
Resource transform callbacks copy request URLs before invoking Kotlin and pass
replacement URLs through the C API response helper before returning. Resource
provider callbacks copy request fields before user code can retain them. A
handled request owns the native request reference only while Kotlin is
responsible for it, enforces one-shot completion, and releases that reference
exactly once.

## Rendering and Tests

`RenderSessionHandle` represents one attached render target for one map. Surface
and borrowed-texture descriptors carry backend objects as `NativePointer`; the
caller keeps those backend objects valid and synchronized for the lifetime
required by the C API.

Texture readback supports reusable explicit native buffers and a convenience
path that returns copied Kotlin image data. Session-owned texture targets expose
backend objects through explicit frame handles. Frame accessors return scoped
`NativePointer` values, reject use after frame close, transfer no ownership, and
expose no general native memory access.

Kotlin/Native tests should exercise the public Kotlin API against the real C
library. Focus on adaptation invariants that cinterop cannot express: handle
release, parent retention, diagnostic copying, wrong-thread propagation, string
validation, callback `StableRef` disposal, one-shot request completion, and
frame scope invalidation.
