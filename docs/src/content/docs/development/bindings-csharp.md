---
title: "C# Binding Conventions"
description: Language-specific implementation conventions for C# bindings.
---

Resources:

- Tracking issue:
  [#48](https://github.com/maplibre/maplibre-native-ffi/issues/48)
- [.NET native interoperability](https://learn.microsoft.com/en-us/dotnet/standard/native-interop/)
- [Source-generated P/Invokes](https://learn.microsoft.com/en-us/dotnet/standard/native-interop/pinvoke-source-generation)
- [ClangSharp](https://github.com/dotnet/ClangSharp)

## Architecture

The C# binding exposes a low-level .NET API over the public C API. It targets
`net10.0` for source-generated interop, NativeAOT-compatible call paths,
trimming-friendly metadata, and modern memory helpers.

Keep ClangSharp-generated declarations internal: constants, layouts, opaque
pointer types, and raw functions. Internal support code owns status conversion,
diagnostics, descriptor materializers, UTF-8 helpers, callback state, native
memory guards, and handle state. The public layer is handwritten C# handles,
values, descriptors, callbacks, and exceptions.

Prefer source-generated `LibraryImport` stubs for raw calls that the generator
can express cleanly. Handwritten P/Invokes cover generator gaps and use the same
internal shape. Keep the public API parallel to Java FFM and JNI where
practical: same concepts, same handle suffixes, same copied-value model, and
.NET-specific names and resource patterns.

## Public Types

Long-lived C-owned objects are public sealed classes with the shared `Handle`
suffix. They implement `IDisposable` and expose `Close()` for fallible
deterministic release. `Close()` throws `MaplibreException` on native failure
and leaves the handle live for retry. `Dispose()` uses a non-throwing cleanup
path for `using`, stack unwinding, and best-effort teardown; it marks the
wrapper closed only after successful native release and records or reports
failures without hiding an in-flight exception.

Use `SafeHandle` only at the private boundary when it helps contain pointer
state. Thread-affine destruction can fail and must run on the owner thread, so
`SafeHandle.ReleaseHandle()` and finalizers report leaks for these handles. They
clean only resources whose C release contract is thread-independent and
infallible.

Descriptors are C# classes or structs that store semantic fields. Internal
materializers write `size` fields, masks, nested C layouts, and backing storage.
Field-mask descriptors use explicit nullable fields, presence flags, or clear
methods; callers never set ABI bookkeeping.

Closed C enum domains map to C# enums with explicit raw conversions. Output
domains that may grow preserve unknown raw values in a wrapper type. Public bit
masks use `[Flags]` enums or purpose-built value types. Native result, snapshot,
and list handles stay internal; readers copy into .NET values and release native
handles in `finally`.

`NativePointer` is an immutable value around `nint`. It represents a borrowed
backend-native address, grants no memory access, and transfers no ownership.
Public APIs accept it only where the C API already accepts opaque host-owned
backend handles.

## Handles and Threading

Each handle stores private state: native pointer, live or closed state, parent
references, owner-scope callback state, and optional leak context. A child keeps
its parent wrapper strongly while native validity depends on that parent.
`MapProjectionHandle` is the shared exception: it owns a standalone projection
snapshot after creation and does not retain the source `MapHandle` for native
validity.

Handle methods run on the calling managed thread. The binding does not dispatch
to a UI thread, `SynchronizationContext`, thread pool, or custom scheduler.
Native wrong-thread statuses become `WrongThreadException` with the copied
native diagnostic. Higher-level adapters may add owner-thread execution models
above this layer.

Successful `Close()` releases the native object once, releases managed callback
state, stores a closed marker, and makes later close calls no-ops. If native
destruction fails, the handle remains live for retry. Public methods validate
closed wrappers, active texture frames, callback-scoped borrows, and one-shot
request completion before crossing into C.

## Status, Diagnostics, and Strings

Status-returning calls throw `MaplibreException` subclasses. Exceptions carry a
stable `MaplibreStatus`, the raw status code, and the diagnostic copied
immediately after the failing call on the same thread. Unknown future statuses
preserve the raw value and diagnostic.

Use UTF-8 at the boundary. Null-terminated inputs reject embedded `\0`.
Explicit-length string views materialize UTF-8 bytes plus byte length. Borrowed
native strings, messages, event payloads, resource URLs, and string views are
copied before their native borrow window ends.

## Native Memory

Materialize native inputs at the call boundary. Use stack locals, `stackalloc`,
`Span<T>`, `ReadOnlySpan<T>`, `MemoryMarshal`, `Unsafe`, `NativeMemory`, and
scoped unsafe blocks for temporary ABI storage. Temporary pointers live only for
calls that consume or copy them before returning.

Use object-owned native memory for callback state, reusable native buffers, and
storage the C API borrows beyond one call. Private guards release such storage
exactly once. Pinned arrays and `GCHandle` are short-lived unless a C contract
borrows managed storage for a longer owner scope; then the owner stores and
frees the handle deterministically.

## Callbacks

Callbacks use C-compatible static entry points and binding-owned state. Prefer
`UnmanagedCallersOnly` static thunks when the C signature supports unmanaged
function pointers. Use delegates only when they preserve the low-level model;
store them strongly for the exact native registration scope so the GC cannot
collect the thunk target.

Callback state lives in a `GCHandle`, registry entry, or private native-context
object owned by the relevant runtime, map, style, or request scope. Callbacks
may arrive on MapLibre worker, network, logging, or render-related threads, so
the state they touch is thread-safe. Thunks catch managed exceptions and convert
them to the C callback's documented behavior. Managed exceptions never unwind
through native frames.

When replacing a callback, install the new native descriptor before releasing
the old managed state. If native installation fails, release the replacement
state and keep the previous state active.

Resource providers copy request data before user code can retain it. A handled
`ResourceRequestHandle` owns the provider request reference, supports completion
during the callback or later from an allowed thread, enforces one-shot
completion, and releases exactly once. Pass-through requests return immediately
and do not retain the native request handle. Resource transform callbacks pass
replacement URLs through the C API response helper before returning. Custom
geometry callbacks track active upcalls and delay state release until in-flight
callbacks finish.

## Render Targets

`RenderSessionHandle` represents one attached render target for one map and
keeps the map wrapper alive. Surface and borrowed-texture descriptors store
backend objects as `NativePointer`; callers keep those objects valid and
synchronized for the C API's required lifetime.

Texture readback supports caller-owned `Span<byte>` or arrays for reusable
storage and may offer a copied image convenience method. Session-owned texture
frames use explicit disposable frame handles. A frame handle acquires the native
frame, exposes copied metadata and scoped `NativePointer` values, rejects access
after close, and closes before resize, another render update, detach, or session
destruction.

## Testing

C# tests exercise the public C# API against the real native library. Cover
status and diagnostic mapping, `Close()` retry behavior, parent retention,
string validation, copied events and resource requests, callback lifetime,
one-shot completion, wrong-thread propagation, native memory guards, and texture
frame invalidation. Generated interop compilation and layout tests act as the C#
bindability check for public headers.
