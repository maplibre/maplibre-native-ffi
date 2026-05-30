---
title: Go Binding Conventions
description: Language-specific implementation conventions for Go bindings.
---

Resources:

- Tracking issue:
  [#43](https://github.com/maplibre/maplibre-native-ffi/issues/43)
- [Go `cgo` documentation](https://pkg.go.dev/cmd/cgo)
- [`runtime.Pinner`](https://pkg.go.dev/runtime#Pinner)
- [`runtime/cgo.Handle`](https://pkg.go.dev/runtime/cgo#Handle)

## Architecture

The Go binding exposes one low-level package over the public C headers through
`cgo`. Raw `C.*` declarations, `unsafe.Pointer` conversions, exported callback
trampolines, and C helper structs stay internal. Public APIs use Go handles,
values, slices, errors, callbacks, and `NativePointer`.

Keep direct C calls small and regular. Internal helpers own repeated invariants:
status checks, same-thread diagnostic capture, descriptor materialization,
string conversion, callback registries, retained C storage, and finalizer leak
reporting. The Go binding targets Go 1.21 or newer for `runtime.Pinner`;
ordinary call-scoped buffers rely on cgo's per-call pinning.

## Public Surface

Long-lived native objects use the shared `Handle` suffix. They are Go structs
with private native state and explicit `Close() error` methods. A successful
close releases once and makes later closes no-ops. A failed close leaves the
handle live so callers can retry on the owner thread.

Go names follow Go casing while treating maplibre as one word in identifiers. C
option structs become Go descriptor structs; materializers write C `size`
fields, field masks, string views, arrays, and nested descriptors internally.
Closed C enum domains become named Go types with constants and explicit raw
conversion. Output domains that may grow keep an unknown raw value for
diagnostics. User-visible bit masks use named mask types; C field masks stay
behind descriptors.

`NativePointer` is a borrowed opaque backend address:

```go
type NativePointer uintptr
```

It grants no memory access, transfers no ownership, and converts to
`unsafe.Pointer` only at the cgo boundary for APIs that accept opaque backend
handles.

## Handles and Threading

MapLibre owner-thread affinity uses OS-thread identity. Go goroutines may move
between OS threads, so handle methods preserve caller execution and call C on
the current thread. The binding does not dispatch ordinary calls to another
thread. If a call reaches the wrong owner thread, native validation returns
`MLN_STATUS_WRONG_THREAD`, and the binding reports the Go wrong-thread error.

Callers that need deterministic affinity use `runtime.LockOSThread` around the
owner loop. The package may provide a small opt-in owner helper that locks a
goroutine to one OS thread, creates the runtime or map there, runs submitted
owner-thread functions, and pumps runtime events. Keep that helper minimal: it
is an owner loop, not an application scheduler, async framework, render
abstraction, or UI integration layer.

Handle state stores the native pointer, release state, parent references, and
optional leak context. Children keep parents alive while native validity depends
on them. `MapProjectionHandle` is the shared exception: it owns a standalone
projection snapshot after creation and does not retain the source map for native
validity.

Finalizers report leaked handles. They do not destroy thread-affine native
handles, because finalizers run on arbitrary GC threads and may run after
parents. Use `runtime.KeepAlive` after cgo calls when a Go wrapper, callback
state, pinned buffer, or parent must remain reachable until the native call
finishes.

## Status and Diagnostics

Fallible operations return Go `error`. Define stable category sentinels such as
`ErrInvalidArgument`, `ErrInvalidState`, `ErrWrongThread`, `ErrUnsupported`,
`ErrNative`, and `ErrUnknownStatus`; concrete error values wrap these so
`errors.Is` works and callers can inspect the raw native status and copied
diagnostic.

When a C call returns a non-OK status, copy the thread-local diagnostic
immediately on the same OS thread before another C call can replace it. The
status helper uses a short `runtime.LockOSThread` window around the C call and
diagnostic read. Binding-owned validation covers closed handles, active frame
borrows, invalid string shapes, callback-scope misuse, and one-shot request
completion. Native validation covers native arguments, state, ranges, enum
domains, and MapLibre rules.

## cgo Memory

Follow cgo pointer rules exactly. Pass Go pointers to C only for the duration
and shape cgo permits. Use cgo's per-call pinning for ordinary byte slices,
small descriptor backing arrays, and out parameters consumed before return. Use
`runtime.Pinner` only when C may retain a Go pointer across calls, and unpin on
the lifetime that clears the native reference.

Prefer C-owned storage for retained strings, callback `user_data` cells,
response payloads borrowed by C after a callback, and reusable native buffers.
Use `C.CString`, C allocation helpers, or binding-owned arenas behind Go
objects, and free them exactly once. Null-terminated string inputs reject
embedded NUL. Explicit-length C string views use UTF-8 bytes plus byte length.

Native result, snapshot, and list handles stay internal. Readers copy borrowed C
data into Go-owned values and release native handles with `defer`, including
failure paths. Runtime event polling returns copied Go event values independent
of the next native poll.

Slices passed for readback or upload remain caller-owned. Methods validate
length and element size before crossing into C, pass `nil` only where the C API
accepts null, and call `runtime.KeepAlive(slice)` after C returns.

## Callbacks

Public callback APIs store Go functions or interfaces in binding-owned state.
Native code calls exported cgo trampolines. Trampolines convert arguments,
recover panics, invoke user code, and convert failures to the C callback's
documented behavior. Panics never cross C frames.

Use `runtime/cgo.Handle` or a binding registry token for Go callback state that
C stores. Delete the handle exactly once when the native owner scope ends.
Callbacks may arrive on MapLibre worker, network, logging, or render-related
threads; callback implementations must be safe for that use and return quickly.
Callbacks that need map or runtime methods hand work back to the owner thread
before calling thread-affine APIs.

Resource transforms copy request URLs before invoking Go and pass replacement
URLs through the C API response helper before the callback returns. Resource
providers copy request fields before user code can retain them. A handled
resource request owns the C request reference, enforces one-shot completion,
supports completion from any thread allowed by the C API, and releases the
reference exactly once.

## Render Targets

`RenderSessionHandle` represents one attached render target for one map. Surface
and borrowed-texture descriptors carry backend handles as `NativePointer`; the
caller keeps backend objects valid and synchronized for the C API's required
lifetime.

Texture readback accepts caller-owned `[]byte` storage and may add a copied
image convenience method. Session-owned texture targets expose backend objects
through explicit frame handles. Frame handles close on the session owner thread,
reject use after close, and stay live while returned metadata or scoped
`NativePointer` values are used. Close a frame before resize, render update,
detach, or session destruction.

## Testing

Go binding tests exercise the public Go API against the real C library. Cover
`errors.Is` mapping, same-thread diagnostic capture, close-and-retry behavior,
finalizer leak reporting where practical, cgo pointer helpers, callback panic
recovery, request one-shot completion, copied events and results, wrong-thread
errors, and texture frame invalidation. Use C ABI tests for native behavior and
Go tests for adaptation invariants that cgo and Go ownership cannot express.
