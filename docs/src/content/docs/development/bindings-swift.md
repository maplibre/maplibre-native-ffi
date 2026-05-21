---
title: Swift Binding Conventions
description: Language-specific implementation conventions for Swift bindings.
---

Resources:

- Tracking issue:
  [#44](https://github.com/maplibre/maplibre-native-ffi/issues/44)
- [Imported C and Objective-C APIs](https://developer.apple.com/documentation/swift/imported-c-and-objective-c-apis)
- [Swift C interoperability](https://developer.apple.com/documentation/swift/c-interoperability)

## Architecture

The Swift binding uses Swift's C importer over the public C headers. Keep the
importer private and expose one low-level Swift package product. The binding
adapts ownership, diagnostics, callbacks, copied values, and render targets;
adapters own scheduling, SDK, rendering, and app policy. Use a private C target
for the imported header, an internal Swift support target for status conversion,
diagnostics, descriptor materializers, handle state, callbacks, and copied
results, and a public Swift target for handles, descriptors, events, errors, and
backend interop values.

Do not export C importer symbols. Raw pointers, imported C structs, field masks,
function pointers, and `@convention(c)` trampolines stay internal. Public
modules group C API concepts.

## Public Types

Long-lived native objects are final classes with the shared `Handle` suffix.
Final classes give ARC stable identity and avoid value copies of native owners.
Each class stores the native pointer, release state, parent references,
callbacks, and optional leak context.

Swift value types model copied C data. Descriptors are structs unless identity
or shared mutation is useful. Descriptor APIs set semantic fields; internal
materializers write C `size` fields, masks, arrays, string views, and nested
descriptors. Field-mask domains use optional stored properties, explicit
`clear...()` methods, or a small presence type. Use one style per descriptor.

Closed C enum domains map to Swift enums with explicit raw conversion helpers.
Output domains that may grow preserve unknown raw values, for example
`case unknown(UInt32)`. C bit masks become `OptionSet` types. Native result and
list handles stay internal; readers copy them into Swift values before release.

`NativePointer` is a borrowed opaque address value. Store the address as a
private `UInt` or equivalent bit-pattern value and expose no memory access.
Convert to `UnsafeRawPointer?` or `UnsafeMutableRawPointer?` only inside support
code for APIs whose C contract accepts opaque backend handles.

## Handles, Errors, and Concurrency

Handle release is explicit and fallible. Public handle classes provide
`close() throws`. A successful close calls the C destroy function, releases
Swift-owned adapter state, marks the wrapper closed, and makes later `close()`
calls no-ops. If native destruction fails, `close()` throws and leaves the
native pointer live so callers can retry on the correct owner thread.

`deinit` reports leaks. It may release only native resources whose C release
function is documented as thread-independent and infallible. Thread-affine
resources rely on explicit `close()` because ARC can finalize on an arbitrary
thread or after parents have gone away.

Child handles retain their parent wrapper while native validity depends on it:
maps retain runtimes, render sessions retain maps, and style-scoped state
retains its owner. `MapProjectionHandle` is the exception: it owns a standalone
projection snapshot and does not retain the source `MapHandle` for native
validity.

Public fallible methods use `throws`. Map each native status to a stable
`MaplibreError` case carrying the raw status and copied diagnostic when present.
Status checks read the diagnostic immediately after the failing C call on the
same thread. The binding validates Swift-owned state: closed wrappers, active
borrows, invalid strings, completed requests, and callbacks. The C API validates
native arguments, owner threads, ranges, and MapLibre rules.

Swift 6 concurrency annotations describe the contract. Owner-thread handles are
non-`Sendable`; immutable copied values may conform to `Sendable`. Callback
boxes become `Sendable` only when their closure and captures can run from
MapLibre worker, network, logging, or render-related threads. The binding does
not dispatch or add `@MainActor` to low-level APIs. Native wrong-thread statuses
become Swift errors.

## Memory and Data

Materialize imported C inputs at the call boundary. Use stack variables,
`withUnsafePointer`, `withUnsafeBytes`, `withUnsafeMutableBytes`, temporary
arrays, and scoped UTF-8 storage. Pointers derived from Swift `String`, `Array`,
or `Data` live only inside the `withUnsafe...` closure unless the binding copies
them into owned native storage.

Null-terminated string inputs reject embedded NUL. Explicit-length
`mln_string_view` inputs use UTF-8 bytes plus byte length. Copied byte payloads
use `Data` when Foundation is already part of the package surface or `[UInt8]`
for a pure standard-library shape. Use one shape within each concept. CPU
readback APIs should support caller-owned mutable storage and may offer a copied
image convenience path.

Borrowed native output becomes copied Swift data before the borrow window ends.
Internal guards release native snapshot, result, and list handles in all paths.
Runtime polling returns independent Swift events. Session-owned texture frames
expose backend handles through a frame-scoped view or closure. The view checks
that the explicit frame handle is still live before converting to
`NativePointer`; plain `NativePointer` values from a frame do not escape the
scope.

## Callbacks

Callbacks use noncapturing `@convention(c)` trampolines. Store Swift callback
state in a retained box and pass it through C `user_data` with `Unmanaged`:
registration retains with `passRetained`, trampolines recover with
`fromOpaque(...).takeUnretainedValue()`, and teardown releases exactly once with
`takeRetainedValue()` or an equivalent owner object. The box owns the closure,
response scratch storage, active-upcall counters, and native registration tokens
for the C owner scope. Mutable box state uses locks, atomics, or another
explicit synchronization primitive when callbacks may arrive concurrently.

Trampolines copy or scope callback arguments before invoking Swift. They catch
Swift errors and convert failures to the documented C behavior; Swift errors do
not escape through C frames. If a callback API can throw, the adapter records
diagnostics or returns a C failure status.

Callbacks may arrive on MapLibre worker, network, logging, or render-related
threads. Public callback types use `@Sendable` where the closure can run
cross-thread. Use `@unchecked Sendable` only for callback boxes whose
synchronization invariants are documented and enforced. Resource provider
callbacks copy request fields before user code can retain them. A handled
request object owns the native request reference, enforces one-shot completion,
and releases exactly once. Custom geometry callbacks track active upcalls and
delay state release until in-flight callbacks finish. When replacing a callback,
install the new native descriptor before releasing the old Swift box.

## Render Targets

Render target descriptors are Swift values. Surface and borrowed-texture
descriptors store host-owned backend handles as `NativePointer`; callers keep
those backend objects valid and synchronized for the C API's required lifetime.
Attach methods return `RenderSessionHandle`, one attached target for one map.

Session-owned texture targets use explicit frame handle classes with
`close() throws`. Frame handles keep the session live, release the native frame
once, and reject access after close. Safe accessors return copied metadata;
backend interop uses a frame-scoped view or closure and documents unsafe
synchronization requirements.

## Testing

Swift tests exercise the public Swift API against the real C library. Focus on
throwing status conversion, diagnostic copying, explicit close behavior,
non-`Sendable` owner-thread handles, callback box synchronization, copied event
payloads, and frame-scope invalidation.
