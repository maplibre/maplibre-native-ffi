---
title: C API Conventions
description: C ABI contract and C/C++ implementation rules for contributors.
sidebar:
  order: 2
---

## API Layout

`include/` is the public C API boundary. Keep implementation-only helpers out of
public headers. Consumers include `maplibre_native_c.h`; domain headers under
`include/maplibre_native_c/` keep declarations maintainable and may be included
directly when useful.

```text
include/                 # public C API headers
  maplibre_native_c.h    # public umbrella header
  maplibre_native_c/     # public domain headers
src/
  c_api/                 # exported C definitions and C boundary validation
  <subsystem>/           # implementation semantics
```

## ABI Rules

The ABI is unstable while `mln_c_version()` returns `0`. Do not add
compatibility shims or version-branching code for changed structs or functions
during this phase.

The public C header targets C23. ABI-crossing enum types use C23
fixed-underlying enum syntax: `int32_t` for status values and `uint32_t` for
non-negative domains and masks unless a native ABI field requires another width.

`mln_json_value` carries the native value alternative, not just a number. Keep
`MLN_JSON_VALUE_TYPE_UINT`, `MLN_JSON_VALUE_TYPE_INT`, and
`MLN_JSON_VALUE_TYPE_DOUBLE` distinct in both directions, because native
consumers match on the exact alternative. `mbgl` reads supercluster
`cluster_id`, `limit`, and `offset` as unsigned and treats another alternative
as absent, so a value that loses its tag produces an empty result with
`MLN_STATUS_OK`. Document the required alternative on any function whose native
behavior depends on it.

Shape structs for future ABI stability. Option and output structs that may grow
use `uint32_t size` fields. Default constructors populate them. Use field masks
or presence booleans for optional values when zero is valid.

Prefer scalar fields, pointers with length fields, structs, unions, and opaque
handles in public structs—these are friendly to binding generators. Expose
borrowed ABI-owned text with a length or provide an explicit copy or drain API.
Backend-native handles are opaque `void*`; document the backend type and
field-level requirements on the struct field, and ownership and lifetime on the
function that accepts or returns the struct. A backend-native handle is an
address the host already owns, so it stays a `void*` and never becomes a
MapLibre handle id.

## Handles

Every MapLibre handle type is `typedef uint64_t`, an opaque id. Each id packs
its handle type, a slot index, and a reuse generation, so an id names one object
for the life of the process and a released id stays distinguishable from every
later one.

`MLN_HANDLE_NULL` is the null handle for every type. A live id always carries a
nonzero type tag, so this value names no object of any type.

Handle entry points report `MLN_STATUS_INVALID_ARGUMENT` and leave the call
without effect for an id that names a released object, an id of the wrong handle
type, and a value this library never issued. `mln_thread_last_error_message()`
distinguishes those cases. Because every handle shares one C type, the type tag
is what rejects a mismatched handle, so document the handle type each parameter
expects.

Handle values are safe to copy, compare, hash, and move between threads, and
carry no ownership on their own. Owner-thread rules govern which thread may call
with a handle, not which thread may hold one.

The bit layout is internal. Hosts pass handles back as issued, and decoding or
synthesizing an id is unsupported.

## Ownership And Execution

Make ownership explicit at every boundary.

Struct definitions describe data shape, required fields, and pointer validity.
Function comments describe whether input pointers are borrowed, copied,
retained, or consumed, and when returned views become invalid.

Borrow host-provided strings and buffers for call-duration inputs. Copy
host-provided strings and buffers that outlive the function or native callback.

Store host-provided callbacks and `user_data` by reference. Document how long
they must remain valid on the registering function. Document the invalidation
point for returned borrowed pointers.

Give owned handles and scoped resources explicit destroy or release functions.
Status-returning functions reject `MLN_HANDLE_NULL`. Void release functions
accept `MLN_HANDLE_NULL` as a no-op, and accept an already-released id as a
no-op, so a host cleanup hook that runs twice stays safe.

Output handle parameters that create or acquire ownership require `*out_handle`
to equal `MLN_HANDLE_NULL` on entry and preserve live host-owned handles on
failure. Document when scoped resource ownership begins, when it ends, and
whether completion may happen inline or later.

The runtime and map use a host-pumped, owner-thread model. Runtime creation
records the owner thread. Runtime, map, map-projection, and render session calls
that touch thread-affine state validate the owner thread.

A map shares its runtime's owner thread. A render session records its own: the
thread that attached it, fixed for the session's lifetime. Attach validates that
the map is live rather than that the caller owns it, so a session may be
attached, driven, and destroyed on a thread that never touches the map. Session
calls from any other thread report the owner-thread status. The host may hand
the map handle to the attaching thread by any means, because a handle is a plain
value and attach resolves it under the C API's own lock, rejecting an id that
names a released map.

Cross-thread dispatch belongs in public functions designed as enqueueing
commands. Document that behavior on the function. Higher-level adapters build
threaded models above the C API.

Map state a render session reaches for is enqueued to the map owner thread
rather than mutated in place, so resizing a session applies the map's logical
size on the map's next pump. Renderer observer callbacks are forwarded to the
map's run loop for the same reason, so the events a frame produces are drained
by a later `mln_runtime_pump()` rather than inside the render call.

Graphics contexts that bind to a thread, such as OpenGL, are made current for
the duration of a session call and released before it returns, so a host keeps
its own context current only on the thread that owns the session. Attach creates
the session's graphics resources on the calling thread, which is why attach
belongs on the thread whose context is current rather than on the map's.

On Apple targets each entry point drains its own Objective-C autorelease pool,
so a host may pump frames from a thread that never returns to a run loop.
Objects that cross the C boundary are retained rather than autoreleased, which
keeps them valid after the entry point that produced them returns.

MapLibre's `RunLoop` is owner-thread scheduler state. Each owner thread may hold
one live runtime. `mln_runtime_pump()` advances that runtime: it parks the owner
thread when asked, then drains the queued tasks, expired timers, and ready I/O
it finds, including work enqueued while it runs. Document pump entry points as
draining rather than as a bounded per-call budget.

One entry point carries both cadence sources: the timeout selects the cadence,
with zero for hosts driven by a callback they do not own and a positive value
for hosts that own their pump thread and take their cadence from the runtime's
own work. Park-and-wake follows these rules:

- The C API owns the parking primitive. Wake signals reach the owner thread
  through runtime state rather than through a host callback, because MapLibre
  raises them from arbitrary threads while it holds locks that every thread
  queueing owner-thread work needs.
- Wake signals set a flag that the pump clears before it returns. Document a
  pump as advancing the runtime, and require the event drain after every return.
- Any-thread wake entry points take a handle that carries its own reference to
  the wake state, never the thread-affine runtime handle.
- Document each blocking entry point's deadlock risk, naming the host locks a
  caller must not hold across it.
- Queue one event per host-visible outcome. An event whose handling acts on the
  latest state, such as a render update, coalesces against an unread one.

## Status And Diagnostics

Status-returning C API functions return `mln_status`. Each function's public
comment lists its status values and meanings.

Use these categories consistently:

- `MLN_STATUS_INVALID_ARGUMENT` for null pointers, unknown enum values, unknown
  flag bits, undersized structs, invalid dimensions, handles that are null,
  released, of the wrong handle type, or never issued, or incorrectly
  initialized output handles;
- `MLN_STATUS_INVALID_STATE` for otherwise valid objects in the wrong lifecycle
  state;
- `MLN_STATUS_WRONG_THREAD` for thread-affine handles called from the wrong
  owner thread;
- `MLN_STATUS_UNSUPPORTED` for backends, platforms, entry points, or requested
  behavior unavailable in this build;
- `MLN_STATUS_NATIVE_ERROR` for native MapLibre errors or C++ exceptions
  converted to status.

Every exported `MLN_API` C++ definition must be `noexcept`. Status-returning
entry points use the C API boundary helper to clear thread-local diagnostics on
entry and convert exceptions to `MLN_STATUS_NATIVE_ERROR`.

Set thread-local diagnostic strings for synchronous non-OK returns. Report
asynchronous native failures through copied runtime events.

## Events And Callbacks

The C API preserves MapLibre Native's imperative, observer-driven model. C API
calls return status for synchronous acceptance or failure; drained events report
later native work.

Prefer polled events for native-to-host notifications about map state,
lifecycle, rendering, and errors. Use native callbacks for low-level extension
points where MapLibre needs a synchronous decision, an asynchronous request
handle, or process-global integration such as logging.

Event payloads use plain data with documented lifetimes. Each event identifies
its source kind and source handle. Queued events never outlive the source handle
they reference: map teardown discards queued events for that map, and runtime
teardown discards runtime-owned event streams before the runtime handle becomes
invalid.

Classify each operation as one of:

- immediate, where the return status is the final result;
- a command, where return status means accepted and later effects arrive as
  events;
- a state snapshot, where the returned data is last-known state;
- a blocking query, used rarely and documented with deadlock risks;
- an event stream, where many events are expected over time.

Logging, resource transform, and resource provider callbacks may run on MapLibre
worker, network, logging, or render-related threads.

A callback API documents:

- which thread may invoke it;
- how long the callback and `user_data` must remain valid;
- whether input pointers are borrowed or copied;
- whether output pointers are copied before return;
- whether it may call back into any C API function;
- what happens when it returns an error or unknown decision value.

Callbacks must not unwind through the C API. Bindings catch host exceptions,
panics, and errors inside the callback and convert them to the callback's
documented return behavior.

Render session APIs document owner thread, render target backend handle
ownership, synchronization, borrowed pointer lifetimes, frame generation or
stale-frame behavior, and teardown rules. Frame generations are session-scoped
counters in frame structs and are unrelated to the generation inside a handle
id. Attach entry points also document that the calling thread becomes the
session's owner thread and what the calling thread's graphics context must
provide.

## Callback Adapter

`include/maplibre_native_c/callback_adapter.h` adapts these synchronous callback
contracts for host runtimes that cannot meet them. It serves hosts with both of
these constraints:

- Host callbacks are delivered asynchronously and return void, so the host
  cannot answer a decision the C API needs immediately, and cannot read a
  borrowed payload that expires when the C callback returns.
- The host has no native compilation unit of its own, because it consumes the
  shared library through a pure foreign-function interface.

A host that compiles native code writes this adaptation there instead, in
whatever form its runtime prefers, and does not use this header.

The layer answers on the host's behalf: it copies borrowed payloads into
native-owned records the host releases explicitly, decides from native-owned
routing tables when a result is needed immediately, and hands records to the
host through void listener functions. Its entry points carry the `mln_adapter_`
prefix and follow every rule in this document, including the callback
documentation requirements above.

This header is public but stays out of the `maplibre_native_c.h` umbrella, so
binding generators that target the umbrella do not emit declarations for a layer
their host does not need. Bindings that need it name the header directly.

Keep test-only entry points out of this layer, as out of every other public
header. A binding that needs to drive native dispatch from its own tests calls
these public entry points directly with the state it registered.
