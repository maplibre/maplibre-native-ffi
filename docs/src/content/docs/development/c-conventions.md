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
A mask whose bits index an open enum domain uses `uint64_t`, because the mask
width limits how many values that domain can hold.

JSON and GeoJSON cross the ABI as UTF-8 bytes, not recursive C structs. Use
`mln_buffer_view` for borrowed input and `mln_buffer` for owned output. Keep
every input length-delimited; do not require a trailing null byte. The parameter
contract states whether a view contains UTF-8 text, serialized JSON, or
arbitrary bytes. Delegate serialized input to the corresponding MapLibre Native
parser or converter, and validate only C-owned shape and lifetime rules.
Document the required JSON number representation when native behavior depends on
whether a number is an unsigned integer, signed integer, or floating point.

Shape structs for future ABI stability. Option and output structs that may grow
use `uint32_t size` fields. Default constructors populate them. Use field masks
or presence booleans for optional values when zero is valid. A runtime event
subscription mask is required instead: an empty mask selects no event types, and
the default constructor sets every type the library reports.

Prefer scalar fields, pointers with length fields, structs, unions, and opaque
handles in public structs—these are friendly to binding generators. Expose
borrowed ABI-owned text with a length or provide an explicit copy or drain API.
Backend-native handles are opaque `void*`; document the backend type and
field-level requirements on the struct field, and ownership and lifetime on the
function that accepts or returns the struct. A backend-native handle is an
address the host already owns, so it stays a `void*` and never becomes a
MapLibre handle id.

A copy-out entry point takes a caller buffer, its capacity, and an out-parameter
for the required length. It writes the required length before it checks the
capacity, so a caller learns the size from a call that could not fit the data.

A null buffer with a capacity of zero is a size probe: the entry point reports
the required length and returns `MLN_STATUS_OK`. This keeps the sizing call
distinct from the `MLN_STATUS_INVALID_ARGUMENT` these functions also use for a
missing object, which a caller otherwise cannot tell apart. A non-null buffer
whose capacity is too small still reports the required length and returns
`MLN_STATUS_INVALID_ARGUMENT`. Entry points whose output length is a documented
constant need no probe.

An `mln_buffer_view` borrows storage only for the call. Parse or copy accepted
input before returning. An `mln_buffer` owns one contiguous result; its view
remains valid until the caller destroys the handle. Each JSON or GeoJSON
document crosses as one generic buffer, so bindings copy bytes instead of
walking a native value tree. Typed fields that accompany a document stay typed
fields on the result.

Preserve loaded style documents byte-for-byte when MapLibre retains the source
bytes. Values reconstructed from native state use compact JSON serialization;
their whitespace, escaping, number spelling, and object member order are not
stable API behavior.

## Graphics Loaders

The shipped library links no graphics loader. It defines the EGL entry points it
calls, and binds those and the GLES and Vulkan tables to the implementation that
the host already loaded. Android and OpenHarmony are the exception, where the
loader is part of the platform, at a fixed location every host on it already
has.

Three rules follow. A build links a loader into the test harness alone, which
drives the graphics API the way a host does. An artifact carries the C API and
nothing else that loads, so repackaging it copies no implementation along, and a
host that loads its own still runs one: handles that one copy mints are opaque
pointers another copy does not own. An artifact carries the C API's own headers
alone, because the headers that a host builds surface descriptors against arrive
with the implementation that it loads.

A local stand-in for the implementation, its headers included, reaches the
install tree through the CMake `loader` component, which a full installation and
the package both leave out. `mise run package-native` checks the three rules
above on every preset.

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
carry no ownership on their own. Runtime, map, projection, operation,
notification-source, and event-batch handles are callable from any native
thread. Render-driver comments name the calls that require a graphics thread.

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

The runtime owns a native scheduler thread and one continuously running MapLibre
`RunLoop`. Runtime and map entry points resolve a handle, acquire its
control-state lease, copy the submission, and commit work to that run loop. They
never make a host thread own or pump MapLibre scheduler state. Map-projection
entry points after creation run on the calling thread over state that creation
captured, serialized by the projection's internal lock.

A render session selects one of two execution contracts at attachment. A
core-worker driver owns a native serial graphics worker. A
caller-graphics-thread driver stores typed work until the host services it where
the context is usable. WGL targets, EGL surfaces, shared EGL textures, existing
WebGL, and browser WebGPU use the caller driver. Transferable Metal and Vulkan
targets, private EGL owned texture targets, and `OffscreenCanvas` WebGL targets
may use a core worker.

Keep render-session control separate from graphics execution. Demand, snapshots,
operations, abandonment, and destruction are any-thread calls. The caller-driver
service call and backend accessors are graphics-thread-affine and serialized.
Runtime and map updates wake the core worker or make the caller mailbox's
notification endpoint ready.

Classify each public function as Immediate, Command, Published snapshot,
Operation, Event batch, or Render-driver call. A binding maps that category to
one target-language shape; it does not add another scheduler or asynchronous
boundary.

The category follows from the declaration, and
`scripts/check-execution-conventions.py` enforces the mapping as a test:

| Declaration                                          | Category           |
| ---------------------------------------------------- | ------------------ |
| `_start` suffix with `mln_operation* out_operation`  | Operation          |
| `_take_result`, `_destroy`, or `_release` suffix     | Immediate          |
| `_drain_` in the name                                | Event batch        |
| `_service_driver_work` suffix                        | Render-driver call |
| `snapshot` in the name reading a live map or session | Published snapshot |
| `uint64_t* out_command_id` parameter                 | Command            |
| anything else                                        | Immediate          |

Name a new function so that its category derives from this table. A call whose
effects surface only through a drained event stream and that hands back no
completion identity is an immediate: `mln_render_session_request_frame` returns
its final status synchronously and reports the frame through frame results, so
it needs no command channel. The checker keeps an exception table for forms the
conventions cannot express; it is empty today, and growth is a design smell.

Pick the category from what the function reads or writes:

- A read of unkeyed, fixed-size map state that changes only through the caller's
  own commands or through load progress is a published-snapshot field. Every
  committed map command publishes a snapshot and reports the published
  generation in its terminal event, so a snapshot at or past that generation
  observes the commit.
- A keyed or parameterized read, a read with an unbounded payload, a read whose
  value follows committed work that the caller did not author, and work whose
  completion is the product are operations. Prefer one info aggregate with a
  found flag over per-field scalar or existence operations, because each
  operation costs every binding a start, wait, and take wrapper.
- A mutation whose entire result is a disposition status is a command in the
  domains that have a command channel: the runtime and the map. A missing id
  reports a not-found status on the terminal event. A disposition-only mutation
  in a domain without a command channel, such as the render session and the
  offline database, stays an operation, because the operation's terminal status
  is that domain's only asynchronous error channel. Add a command channel to
  another domain only when its count of disposition-only mutations justifies a
  new event source.
- A call on state that creation captured into a detached object, which no worker
  touches afterward, is immediate. This choice is per object: reads, setters,
  and close become synchronous together, or their relative order breaks. The map
  projection is the model.

Offer a published snapshot and an ordered operation for the same state only when
each form does distinct work, as camera does: the snapshot serves
latest-published consumers, and the ordered query is the fence. Delete an
ordered form that strictly duplicates the snapshot.

Commands validate and deep-copy every input before returning acceptance. The
runtime assigns an order and command ID at commit. Each accepted command reaches
a terminal disposition, and failures after acceptance carry copied diagnostics
in command events. Operations retain their work independently from their public
observer and expose a permanent terminal status, copied diagnostic, and typed
result transfer.

Published snapshots copy immutable committed state without entering mutable
MapLibre state. Use an ordered operation instead when a query must observe every
preceding command.

Graphics contexts that bind to a thread, such as OpenGL, are made current during
caller-driver service and restored afterward under shared ownership. Dedicated
ownership keeps a session-created context current between renders. WGL and EGL
surface targets use a caller driver. Private EGL owned texture targets and
transferred WebGL targets use a core worker.

On Apple targets each entry point and queued runtime submission drains its own
Objective-C autorelease pool, so a native worker or host render thread does not
need a surrounding run loop. Objects that cross the C boundary are retained
rather than autoreleased, which keeps them valid after the call that produced
them returns.

The native `RunLoop` is the only runtime scheduler. It owns timers, I/O
readiness, queued invocations, and wakeup behavior. Do not alternate a separate
condition-variable queue with `RunLoop::runOnce()`; that queue cannot derive the
next native timer deadline.

Runtime submissions follow these rules:

- One runtime establishes a total commit order across commands, operations,
  barriers, and close.
- Committing eligible work invokes the run loop and wakes it when idle.
- Registry locks protect lookup and state transitions only. Release them before
  run-loop joins, callback quiescence, operation waits, or host notification.
- A close preflight checks live children and pending child reservations before
  committing an irreversible close.
- A runtime barrier completes after every preceding submission reaches a
  terminal disposition, not merely after its run-loop callback starts.
- Queue one event per required host-visible outcome. State consumers may
  coalesce only commands whose public contract permits replacement; every
  replaced command reports `superseded`.
- A subscription mask suppresses an event before its payload and message are
  built. A suppressed event stays out of the queue and does not make its
  notification source readable.

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
- `MLN_STATUS_WRONG_THREAD` for render-driver handles called from a thread other
  than the required graphics thread;
- `MLN_STATUS_UNSUPPORTED` for backends, platforms, entry points, or requested
  behavior unavailable in this build;
- `MLN_STATUS_NATIVE_ERROR` for native MapLibre errors or C++ exceptions
  converted to status.

Every exported `MLN_API` C++ definition must be `noexcept`. Status-returning
entry points use the C API boundary helper to clear thread-local diagnostics on
entry and convert exceptions to `MLN_STATUS_NATIVE_ERROR`.

Set thread-local diagnostic strings for synchronous non-OK returns. Store
operation failures on the operation and report accepted command failures through
copied terminal events.

## Events And Callbacks

The C API preserves MapLibre Native's imperative, observer-driven model. C API
calls return status for synchronous acceptance or failure; drained events report
later native work.

Prefer drained events for native-to-host notifications about map state,
lifecycle, rendering, and errors. A host selects the event types it reads with a
subscription mask, so document the state that each type carries. Options always
read the mask, and a bit outside the documented set of types returns
`MLN_STATUS_INVALID_ARGUMENT`. Use native callbacks for low-level extension
points where MapLibre needs a synchronous decision, an asynchronous request
handle, or process-global integration such as logging.

Event payloads use plain data with documented lifetimes. Each event identifies
its source kind and copied source handle value. Closing a map or disabling an
offline-region observer prevents future publication without changing queued
history. Accepted runtime release consumes the runtime handle and discards its
undrained event stream during native teardown. A drain transfers the complete
queue into an owned batch, which stays readable across later drains and runtime
release until the caller releases it.

Use the six execution categories defined in Ownership And Execution. An
Immediate return is final. A Command return reports copied acceptance. Published
snapshots are immutable synchronous copies. Operations carry one terminal result
and diagnostic. Event batches contain drainable observations. Render-driver
calls execute on the graphics thread named by their target contract.

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

Render-session APIs document driver support, backend handle ownership,
synchronization, borrowed pointer lifetimes, generation fields, backpressure,
and teardown rules. Frame generations are session-scoped counters and are
unrelated to the generation inside a handle id. Attach entry points document
which descriptors are transferable, which graphics calls are thread-affine, and
which context ownership modes the target accepts.

Every accepted frame demand creates one owned terminal result record. Frame
readiness stays level-triggered until the queue drains. Each drain transfers the
complete queue into an independently owned result batch. Host-acquirable owned
textures negotiate a one-to-three-slot ring. An acquired-frame handle leases its
slot until its release operation observes any consumer GPU-completion
synchronization. A private OpenGL owned texture target fixes its depth at one
and grants readback without acquisition or consumer synchronization.

Normal detach routes graphics destruction through the selected driver before
CPU-only handle destruction. Abandon closes control and mailboxes without
graphics calls, returns busy during an in-flight driver call, detaches the map
in CPU state, invalidates accessors, and quarantines resources that require the
lost owner. Releasing an acquired frame after abandon remains CPU-only and
terminates its operation with target lost.

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
