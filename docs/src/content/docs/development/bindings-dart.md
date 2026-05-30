---
title: Dart Binding Conventions
description: Language-specific implementation conventions for Dart bindings.
---

Resources:

- Tracking issue:
  [#51](https://github.com/maplibre/maplibre-native-ffi/issues/51)
- [Dart C interop using `dart:ffi`](https://dart.dev/interop/c-interop)
- [`ffigen`](https://pub.dev/packages/ffigen)

## Architecture

The Dart binding exposes a safe low-level package over the public C API through
`dart:ffi`. Use Dart 3.10+ as the language floor for the low-level API.

Generate private `ffigen` declarations from the public umbrella header.
Generated ABI classes, raw pointers, C structs, field masks, and
function-pointer trampolines stay below the package's public API. Public APIs
expose Dart handle classes, descriptors, copied values, exceptions, callbacks,
events, typed data, and opaque native pointers.

Keep Flutter support separate from this layer. Flutter native platforms may use
the package as their C ABI binding, but widgets, platform channels, Flutter web,
frame scheduling, and UI integration belong in adapters above it.

## Public Types

Long-lived C-owned objects use the shared `Handle` suffix. Handles are Dart
classes with explicit `close()` methods. A successful `close()` releases the
native object once and makes later closes no-ops. Failed native destruction
leaves the handle live so callers can retry or inspect diagnostics.

C option structs become Dart descriptor classes or records. Store Dart values in
the public type, then materialize C structs, `size` fields, field masks, arrays,
and string views in the FFI layer. Events, native result handles, lists, and
snapshots copy into independent Dart values before the native borrow window
ends. JSON, GeoJSON, descriptor values, and runtime events preserve MapLibre
semantics rather than exposing generated C layouts.

Closed C enum domains map to Dart enums with explicit raw-value conversion.
Output domains that may grow keep a stable unknown representation with the raw
native value for diagnostics. Public bit masks use Dart value types or sets; C
field masks stay inside descriptor materializers.

`NativePointer` is a small borrowed value around an address integer. It grants
no memory access, transfers no ownership, and appears only for backend-native
handles accepted by the C API. Convert it to `Pointer<Void>` only inside the FFI
layer.

## Ownership, Memory, and Finalizers

Each handle stores its native pointer, live/released state, parent references
needed for native validity, callback state, and optional leak context. Children
keep parents alive while native validity depends on them. `MapProjectionHandle`
is the shared exception: after creation it owns a standalone projection snapshot
and does not retain its source `MapHandle` for native validity.

Use `NativeFinalizer` for leak reporting. Use a finalizer for cleanup only when
the release function is documented as thread-independent, infallible, and safe
after related objects finalize in any order. Thread-affine MapLibre handles flow
through `close()` on their owner thread. Finalizer tokens never expose the raw
MapLibre handle directly to Dart code; store only the minimal native token or
adapter state needed for leak reporting or a proven safe cleanup path.

Per-call native storage uses `calloc`, arenas, or helper scopes that free all
allocation paths. Temporary UTF-8 strings and descriptors live only until the C
call returns. Null-terminated string inputs reject embedded `NUL`; explicit
string-view inputs pass UTF-8 bytes plus byte length. Copy C strings and string
views before their documented borrow window ends.

Use `Uint8List`, `ByteData`, and other typed-data values for copied Dart-owned
bytes. Use explicit native buffers only when the C API needs a stable address
for a call, frame, or reusable readback target. Typed-data views over native
memory are scoped: accessors check that the owning frame, request, or borrow is
still active, and the binding invalidates them when the scope closes.

## Calls, Threads, and Diagnostics

Ordinary methods call native code synchronously on the calling Dart isolate's
current native thread. The binding does not dispatch ordinary calls to another
thread, event loop, Flutter scheduler, or worker isolate. Native owner-thread
checks remain authoritative; `MLN_STATUS_WRONG_THREAD` becomes a Dart
wrong-thread exception carrying the copied diagnostic.

Dart isolates are not OS threads and do not prove native owner-thread identity.
Thread-affine handles are isolate-local binding objects: they are not serialized
through ports, and internal adapter state rejects use from an isolate or owner
scope that did not create the handle where Dart can detect it. Native
wrong-thread validation remains the authoritative backstop. Higher-level
adapters own request queues, owner-thread executors, isolate coordination, and
Flutter scheduling.

Status-returning C calls either complete normally or throw a stable
`MaplibreException` subclass. Errors store the mapped status category, raw
native status, and the thread-local diagnostic copied immediately after the
failing C call on the same thread. Binding validation covers closed handles,
active scoped borrows, invalid strings, one-shot request completion, and
unsupported callback shapes. The C API validates native state, ranges, and
MapLibre-specific rules.

## Callbacks and Requests

Dart callbacks are stored strongly for the C owner scope that can invoke them.
Internal adapters copy or scope callback arguments, catch Dart exceptions, and
convert failures to the documented C callback behavior. Dart exceptions never
escape through native frames.

Use `NativeCallable.listener` for void MapLibre upcalls that can post copied
arguments to Dart, such as custom geometry fetch and cancel notifications.
Reserve `NativeCallable.isolateLocal` for binding-owned helpers that call back
on the same native thread during the initiating FFI call. Use `ReceivePort` with
`sendPort.nativePort`, usually behind a native shim, when logging, resource
providers, or owner-thread handoff need to return quickly while queuing copied
work for the owning isolate. Keep isolate-group callback support in experimental
adapters, outside the core binding design.

Resource transforms stay synchronous. Implement them with native-owned rewrite
rules or a callback shape that can return quickly without touching thread-affine
handles. Copy borrowed request fields before Dart code can retain them, and pass
replacement URLs through the C API response helper before returning.

Resource provider callbacks copy request data before posting to Dart. Matching
requests own a `ResourceRequestHandle` that enforces one-shot completion and
exactly-once release. Pass-through requests return immediately and do not retain
the native request handle. Handled requests may complete inline or later through
a queue when the C API allows cross-thread completion.

## Rendering and Tests

Render target descriptors contain borrowed backend handles as `NativePointer`.
`RenderSessionHandle` represents one attached target for one map and stays
thread-affine to the map owner thread. CPU readback APIs copy into Dart typed
data or explicit native buffers and return copied metadata.

Session-owned texture targets use explicit frame handles. Frame handles expose
copied metadata and scoped backend `NativePointer` values. Access after frame
close throws, and callers close frames before resize, another render update,
detach, or session destruction.

Test the Dart adaptation layer against real C calls when practical. Cover handle
close semantics, parent retention, native diagnostic mapping, wrong-thread
status propagation, string and typed-data copying, scoped frame invalidation,
callback exception conversion, and one-shot resource request completion.
