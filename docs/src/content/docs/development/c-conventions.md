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

Shape structs for future ABI stability. Option and output structs that may grow
use `uint32_t size` fields. Default constructors populate them. Use field masks
or presence booleans for optional values when zero is valid.

Prefer scalar fields, pointers with length fields, structs, unions, and opaque
handles in public structs—these are friendly to binding generators. Expose
borrowed ABI-owned text with a length or provide an explicit copy or drain API.
Backend-native handles are opaque `void*`; document the backend type and
field-level requirements on the struct field, and ownership and lifetime on the
function that accepts or returns the struct.

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
Status-returning functions reject null handles. Void release functions accept
null as a no-op.

Output handle parameters that create or acquire ownership reject non-null
`*out_handle` values and preserve live host-owned handles on failure. Document
when scoped resource ownership begins, when it ends, and whether completion may
happen inline or later.

The runtime and map use a host-pumped, owner-thread model. Runtime creation
records the owner thread. Runtime, map, map-projection, and render session calls
that touch thread-affine state validate the owner thread.

Cross-thread dispatch belongs in public functions designed as enqueueing
commands. Document that behavior on the function. Higher-level adapters build
threaded models above the C API.

MapLibre's `RunLoop` is owner-thread scheduler state. Each owner thread may hold
one live runtime. `mln_runtime_run_once()` pumps that runtime's run loop.

## Status And Diagnostics

Status-returning C API functions return `mln_status`. Each function's public
comment lists its status values and meanings.

Use these categories consistently:

- `MLN_STATUS_INVALID_ARGUMENT` for null pointers, unknown enum values, unknown
  flag bits, undersized structs, invalid dimensions, invalid handles, or
  incorrectly initialized output handles;
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
ownership, synchronization, borrowed pointer lifetimes, generation or
stale-frame behavior, and teardown rules.
