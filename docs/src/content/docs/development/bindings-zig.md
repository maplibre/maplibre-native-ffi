---
title: Zig Binding Conventions
description: Language-specific implementation conventions for Zig bindings.
---

Resources: [Zig C interop](https://ziglang.org/documentation/master/#C) and
[Zig allocators](https://ziglang.org/documentation/master/#Choosing-an-Allocator).

## Architecture

The Zig binding exposes one safe low-level package over the public C API.

Keep `@cImport` declarations from `maplibre_native_c.h` private. Raw C handles,
C structs, C field masks, C function pointers, and `callconv(.c)` trampolines
stay below the public API. Public APIs use Zig values, handles, slices,
allocators, function pointers, context pointers, and error unions.

Direct C calls are fine inside the package. Add private helpers only for
repeated invariants such as status checking, diagnostic capture, string
conversion, native result guards, callback state, and descriptor
materialization. Link to `maplibre-native-c` through Zig build configuration.

## Public Surface

Long-lived native objects use the shared `Handle` suffix: `RuntimeHandle`,
`MapHandle`, `MapProjectionHandle`, and `RenderSessionHandle`. Use `close()` for
C-owned handles because native destruction can fail. Use `deinit()` for pure
Zig-owned values that release allocator-backed storage.

C option structs become Zig value descriptors. Optional fields are preferred for
field-mask domains when they keep the type clear. Materializers write C `size`
fields, field masks, string views, arrays, and nested descriptors internally.

Closed C enum domains map to Zig enums with explicit raw conversions. Native
output domains that may grow preserve unknown raw values. User-visible bit masks
use purpose-built wrappers or `packed struct(u32)` values. C field masks stay
behind descriptors.

`NativePointer` is a borrowed non-null opaque backend address backed by
`*anyopaque`. Optional backend handles use `?NativePointer`. It grants no memory
access, transfers no ownership, and appears only where the C API already accepts
or returns opaque backend handles.

JSON and GeoJSON values are owned Zig value trees when the binding needs to
preserve MapLibre value semantics: integer width, object member order, and
duplicate keys.

## Handles and Threading

Zig callers manage native ownership explicitly with `var` handles and
reverse-order `defer` cleanup. Lifecycle handles are owned Zig structs that
store the native pointer and Zig adapter state. Successful `close()` releases
the native object, releases Zig-owned adapter state, and stores `null`; later
`close()` calls on the same handle no-op. Failed native destruction leaves the
pointer live so callers can retry.

Treat lifecycle handles and texture frame handles like other Zig resource
structs: pass them by pointer and avoid copying them. A copied handle is a
duplicate owner; using that copy after the original closes or releases, or
closing or releasing both copies, is invalid.

The Zig binding does not retain parents with counters by default. Callers keep
parents live while child handles are live. Default destruction order is child
before parent: maps before runtimes, render sessions before maps, texture frames
before render sessions, and style-scoped callback state before the owning map or
style replacement. Add targeted guards only where native validation and cleanup
order are not enough.

`MapProjectionHandle` owns a standalone projection snapshot after creation and
does not depend on the source `MapHandle` for native validity.

The binding does not dispatch calls to another thread. Owner-thread-affine calls
run on the calling Zig thread. Native `MLN_STATUS_WRONG_THREAD` maps to
`error.WrongThread` and updates diagnostics when available.

## Status and Diagnostics

Status-returning C calls map to stable Zig errors: `InvalidArgument`,
`InvalidState`, `WrongThread`, `Unsupported`, `NativeError`, and
`UnknownStatus`. Binding-owned validation uses stable errors for closed handles,
active borrows, invalid string shapes, already-completed requests, and ABI
version mismatches. Public functions compose these with allocator, I/O, and
platform errors as needed.

Zig error tags do not carry payloads. Native diagnostic payloads live in an
explicit `DiagnosticStore`. Status checks copy `mln_thread_last_error_message()`
immediately on the same thread, before another C call can replace it. Without a
store, the API still returns the stable Zig error tag and discards the payload.

Handle constructors accept an optional non-owning diagnostic store, and child
handles inherit it unless an API offers an override. Free functions and one-shot
helpers accept `?*DiagnosticStore` directly. Callers keep the store live for
every handle that uses it.

## Memory and Events

Public methods materialize native inputs at the call boundary. Temporary storage
uses stack values, fixed buffers, temporary arrays, arenas, or other call-scoped
storage. Null-terminated string inputs reject embedded `NUL` bytes.
Explicit-length string views use UTF-8 bytes plus byte length.

APIs that allocate variable-size copied output take a `std.mem.Allocator`.
Returned owned values document their deallocation path. Native result, snapshot,
and list handles stay private; internal guards release them exactly once after
copying borrowed data into Zig-owned values.

Runtime polling returns owned Zig event values. Payloads, messages, string
views, and source metadata are copied before the next native poll. Unknown event
and payload domains preserve raw native values. Map events identify maps with
Zig-owned source identity such as `MapId`, not public borrowed native handles.

## Callbacks

Public callback APIs use Zig function pointers plus context pointers. Internal
`callconv(.c)` trampolines copy or scope callback arguments, invoke Zig code,
convert Zig errors to the C callback's documented behavior, and keep callback
state alive for the native owner scope.

Callbacks may arrive on MapLibre worker, network, logging, or render-related
threads. Callback state invoked from those threads must be safe for that use.
Callbacks return quickly and hand work back to the owner thread before calling
thread-affine map or runtime APIs.

When replacing a callback, install the new native descriptor before releasing
the old Zig state. If native installation fails, release the replacement state
and keep the previous callback active.

Resource transforms copy request URLs and pass replacement URLs through the C
API response helper before the callback returns. Resource providers copy request
fields before user code can retain them. Handled requests enforce one-shot
completion or release. Custom geometry callbacks track active upcalls and delay
state release until they finish.

## Render Targets

`RenderSessionHandle` represents one attached render target for one map. Surface
and borrowed-texture descriptors contain borrowed backend handles as
`NativePointer`; callers keep backend objects valid and synchronized for the C
API's required lifetime.

Texture readback supports caller-owned `[]u8` storage and an allocator-backed
owned image convenience path. Session-owned texture targets expose backend
objects through explicit frame handles. Frame handles use `release()` and follow
the same no-copy owned-resource rule as lifecycle handles. Backend pointer
accessors are scoped to the live frame handle and documented with a `Safety:`
section when callers must uphold synchronization or lifetime rules.

## Testing

Zig binding tests are the primary Zig-path test suite. They exercise the public
Zig API against the real `maplibre-native-c` library and cover both native C API
behavior and Zig adaptation at that boundary.
