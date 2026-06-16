---
title: Binding specification
description: Specification for low-level language bindings over the C API.
sidebar:
  order: 3
---

Specification for language binding subprojects that expose MapLibre Native
through the public C API.

## Scope

Every binding gives host code a safe, low-level way to use MapLibre Native
through `maplibre_native_c.h`.

A binding exposes MapLibre concepts directly, keeps native ownership and
borrowed data safe in the target language, reports native failures through the
target language's error model, and tests the supported C API domains through the
public binding.

The required binding layer is a low-level FFI API. It MUST NOT add
application-framework policy, UI/view lifecycle integration, general async APIs,
or scheduler models above the C API concepts.

A subproject that needs host-thread confinement uses the owner-thread helper
design in Threading. No other execution model belongs in the binding layer.

---

## Priorities

When requirements conflict, resolve them in this order:

1. Target-language safety idioms over cross-binding alignment.
2. Cross-binding alignment over target-language syntactic idioms.

Cross-binding alignment means bindings expose the same C API concepts, ownership
rules, operation boundaries, and error semantics. It does not require identical
syntax, names, or package structure.

Bindings are minimal and complete:

- Minimal: public APIs map to C API concepts and binding-owned safety policy.
  They expose one correct low-level public API for each C API operation and do
  not add redundant workflows, shortcuts, higher-level composites, or alternate
  ownership, threading, callback, allocation, or error contracts.
- Complete: a binding that claims support for a C API domain exposes that
  domain's public operations, input shapes, event payloads, status behavior, and
  ownership rules through public wrapper types. Unsupported domains stay absent
  from the safe public API.

## Architecture

Every binding splits implementation into three layers:

| Layer                  | Responsibility                                                                                       |
| ---------------------- | ---------------------------------------------------------------------------------------------------- |
| Raw C layer            | Generated or tool-imported declarations for the public C headers, raw structs, constants, and calls. |
| Internal support layer | Status conversion, diagnostics, handle state, memory guards, callback state, native loading, shims.  |
| Public binding layer   | Handles, input types, values, events, callbacks, errors, and render-target APIs for users.           |

Requirements:

- Raw C declarations MUST stay outside the safe public API. If the target
  language cannot hide raw declarations, they MUST live under a generated
  interop namespace and be excluded from the supported safe API surface.
- Host FFI carrier types and raw entrypoints MUST stay outside the safe public
  API. Public APIs expose backend addresses only through `NativePointer`.
- Public examples and tests MUST use the public binding layer, not raw C calls,
  except bindability and layout tests.
- Raw C declarations MUST be generated or tool-imported from public headers.
  Generated files MUST be mechanically reproducible from public headers,
  metadata, or generator inputs checked into the repository.
- Handwritten support code MUST follow repeatable patterns that can be audited
  across domains.
- Native library loading and ABI-version checks MUST fail with a binding error
  before a public wrapper stores an incompatible native handle. The expected ABI
  version MUST come from generated binding metadata or a checked-in binding
  constant, and ABI mismatch MUST use a stable public error category.

---

## Naming

This specification uses generic concept names. Bindings apply target-language
naming and packaging conventions while preserving the concept and ownership
semantics.

Use `MapLibre` when referring to the project or upstream library in prose.
Inside code identifiers, treat `maplibre` as one word and apply the target
language's normal casing rules:

```text
MaplibreMap
maplibre_map
maplibreMap
MAPLIBRE_MAP
```

Do not split the brand into separate identifier words such as `MapLibreMap` or
`map_libre_map`.

Long-lived C-owned opaque handle concepts include:

- `RuntimeHandle`
- `MapHandle`
- `MapProjectionHandle`
- `RenderSessionHandle`
- `OfflineOperationHandle`
- `ResourceRequestHandle`

`Handle` means the public value owns or controls an explicitly releasable native
resource with identity across operations. The representation can vary by
binding; the ownership contract does not.

---

## Handle Lifetime

Public handles are for values that own or control native state across calls,
such as runtimes, maps, render sessions, offline operations, resource requests,
and acquired texture frames. Input values, events, diagnostics, query results,
snapshots, and native-filled structs become copied language values. Native
snapshot, result, and list handles remain internal implementation details.

### Owned handles

Every public wrapper that owns or controls native state across calls MUST store:

- private native identity;
- live/releasing/closed state;
- the native destroy function or bridge release path;
- parent references or lifetime evidence required for native validity;
- callback state owned by that handle's native scope;
- leak-reporting context when the binding has non-deterministic cleanup hooks.

Native identity MUST NOT be public. The implementation can represent it with a
native pointer, bridge-owned handle, or private table ID, but every public
operation uses the same ownership rules.

Public release follows this operation:

1. If the wrapper is already closed, return success without calling native code.
2. If another release is in progress, wait for it or return the binding's
   in-progress error before calling native code.
3. Mark the wrapper as releasing so public methods fail before calling native
   code.
4. Keep owner-scoped support state live, including parent references, callback
   state, and request registries.
5. Invoke the matching native release path.
6. If native release succeeds, mark the wrapper closed and make later release
   calls no-op.
7. If native release fails, restore the live state and return the native error
   with diagnostics. Consuming or move-based release APIs return the live owner
   state so callers can retry.

Deterministic cleanup hooks follow the same release operation when they can
report release failure through the target language's normal error path.
Non-deterministic cleanup hooks report leaks for thread-affine handles. They
MUST NOT destroy runtime, map, projection, or render-session handles from
cleanup hooks. Infallible language destructors that attempt best-effort release
MUST preserve the explicit release contract and MUST NOT mask native errors from
the explicit release path.

### Parent validity

Bindings MUST preserve native parent validity while child wrappers are live:

Child wrappers retain the parent owner state whenever native validity depends on
the parent. Releasing a parent while children are live MUST fail without
consuming or destroying the parent.

`MapProjectionHandle` is the exception: after creation it owns a standalone
projection snapshot. It MUST remain valid after the source map closes and MUST
release with `mln_map_projection_destroy()`.

### Handle copying

Owned handle wrappers are affine: the safe public API MUST NOT create duplicate
owners. Reference-copy languages MUST make all references share one owner state.
Value-copy languages MUST make owned handles non-copyable or move-only. Public
code MUST NOT be able to fabricate live handles, ID-backed operation tokens, or
request tokens from raw integers, raw addresses, public fields, or ordinary
constructors.

---

## Status And Diagnostics

Every status-returning C call maps to the target language's normal error
mechanism.

Status handling follows this operation:

1. Run binding-owned validation before calling C when the binding can detect the
   failure itself.
2. Report binding-owned validation through the same public error family as
   native failures, with a fresh binding diagnostic and no stale thread-local C
   diagnostic.
3. When a C call returns non-OK, copy `mln_thread_last_error_message()` on the
   same thread before any later diagnostic-writing C call.
4. Convert each known non-OK status to the corresponding documented public error
   kind. The error kind is stable API and is separate from the diagnostic
   string.
5. Convert unknown future status values to the documented unknown-status error
   kind and carry the raw native value.
6. Expose the copied diagnostic through the public error. When the target
   language's error mechanism cannot carry payloads, as with Zig error tags,
   expose a diagnostic store for the failing call and document how child handles
   inherit it.

Bindings validate binding-owned state before calling C. They do not duplicate
MapLibre or native validation that the C API already performs.

---

## Type Mapping

Bindings translate C data shapes into public language values without exposing
ABI bookkeeping.

### Input Structs and Values

C option structs map to language-owned public types, such as classes, records,
or structs.

Public callers set semantic fields. ABI `size` fields, masks, and raw nested C
storage stay inside C struct materializers.

C struct materialization follows this operation:

1. Call the C default initializer for each defaultable C struct before setting
   public fields.
2. Set semantic fields from the public value.
3. Encode optional fields with explicit present/absent state so present zero
   values remain distinguishable from absent values.
4. Keep native input storage alive for the full C borrow window, including
   nested input trees and interior pointers.

Structs that C fills and returns by value become copied language values.

### Enums and masks

Public enum values convert to C through a complete named-case mapping. Public
values represent valid C enum inputs.

When C returns an enum value that the binding does not know yet, the binding
preserves the raw value instead of collapsing it to a known case. Public input
paths reject values outside the C input contract before crossing into C.

Public bit masks use a named public type that supports combining, testing, and
empty values. C field masks stay internal to C struct materializers.

### Strings

Public string inputs use UTF-8 at the C boundary. Null-terminated `const char*`
inputs reject embedded `NUL`. `mln_string_view` inputs pass UTF-8 bytes and byte
length, and allow embedded `NUL` when the C contract allows it. Borrowed C
strings and string views are copied before their native borrow window ends.

### JSON and GeoJSON

Structured JSON and GeoJSON models MUST preserve MapLibre value semantics:
object member order, repeated member names, signed and unsigned integer width,
floating-point values, booleans, nulls, strings, arrays, and nested objects. Raw
JSON or GeoJSON text inputs MUST pass through as text without reparsing or
reformatting unless the public API is explicitly a structured-value API.

Bindings MUST preserve the full unsigned 64-bit domain for unsigned JSON and
GeoJSON integer values. Languages without a native unsigned 64-bit integer type
MUST expose those values through a documented representation that preserves the
raw value. The binding documentation MUST state how callers compare and format
representations that are not naturally unsigned.

### Native pointers

`NativePointer` represents a borrowed opaque backend-native address.

`NativePointer` transfers no ownership and grants no general memory access.
Public APIs accept it only where the C API accepts host-owned opaque backend
handles. Conversion from raw addresses or raw pointers is internal or exposed as
an unsafe or borrowed backend-interop constructor that states the backend
lifetime and synchronization requirements.

Backend pointers returned from acquired texture frames perform active-frame
checks before exposing the pointer.

---

## Data Ownership

Bindings keep C borrow windows explicit and expose stable language-owned values.

### Temporary native storage

Bindings materialize most native input at the call boundary.

Native input materialization follows this operation:

1. Allocate or borrow temporary native input storage.
2. Initialize out parameters and callback response structs to the C API's
   neutral value before user code runs.
3. Keep temporary storage alive until the C call returns or the full documented
   native borrow window ends.
4. Release temporary native allocations on every failure path.

Public objects store temporary pointers only when the object owns that native
storage and releases it deterministically.

### Output values

Bindings expose C outputs as language-owned values unless the result owns or
controls native state across calls.

Native snapshot, result, and list handles are internal implementation details.
Plain value outputs with no interior borrowed pointers are copied by value.

Outputs backed by native storage follow this operation:

1. Acquire the native snapshot, result, list, or event.
2. Copy public data into language-owned values before the native borrow window
   ends.
3. Preserve unknown event and payload domains as raw values with copied payload
   bytes when the C API exposes those bytes.
4. Release native snapshot, result, and list handles exactly once after copying,
   including failure paths.

Runtime event polling returns values independent of the next native poll.
Map-originated events identify a live source map when identity can be proven. If
lookup misses, they carry no public map handle or only copied source metadata.

## Callbacks And Requests

Callbacks and request handles preserve C lifetimes while protecting
host-language state.

### Callback lifetime

Callback state is retained by the native scope that can invoke it and remains
live until replacement, clearing, owner release, native unregistration, and
in-flight invocations can no longer reach it.

Callback invocation follows this operation:

1. Copy callback arguments into language-owned values before user code receives
   them. Lexical views are allowed only when the public type prevents retention
   beyond the invocation.
2. Host-language failures must not unwind or otherwise escape across the C
   callback boundary. If the public callback returns a recoverable host failure,
   convert the failure to the C callback's documented behavior.
3. Synchronize callback state that native can invoke concurrently.
4. Return promptly. Callback code hands owner-thread work back to the owner
   thread before calling runtime or map APIs.

Callback replacement installs the new native registration before releasing old
callback state. If installation fails, the old callback remains active and the
replacement state is released. Clearing, replacing, or closing prevents new
upcalls, waits for in-flight upcalls, and releases callback roots after native
can no longer invoke them.

If a leaked native owner can still reach callback user data, non-deterministic
cleanup reports the leak and keeps callback memory reachable from native alive.
Style-scoped callback retention follows current native source ownership, not
stale event timing or source ID reuse alone.

### Resource transforms

Resource transform callbacks are synchronous.

Resource transform invocation follows this operation:

1. Copy request URL and metadata into language-owned values before user code
   receives them.
2. Initialize the native response shape to pass-through.
3. If user code returns a replacement URL, copy it into the native response
   shape before the callback returns.
4. If user code returns no rewrite or fails validation, keep pass-through
   behavior.
5. Host-language failures must not unwind or otherwise escape across the C
   callback boundary. If the public handler returns a recoverable host failure,
   convert the failure to the C callback's documented behavior.

### Resource providers

Resource providers decide whether a request passes through to the native
provider or is handled by the binding.

Resource provider invocation follows this operation:

1. For pass-through requests, return pass-through without retaining the native
   request handle.
2. For handled requests, copy request fields before user code can retain them.
3. Retain the native request reference until completion, cancellation handling,
   or release.
4. Treat inline completion during the provider callback as handled ownership,
   even if the callback return path would otherwise pass through.
5. Allow deferred or cross-thread completion when the C API allows it, without
   changing one-shot or release behavior.

Handled request completion is terminal. A request can complete once; a
completion that reaches C consumes the completion path even when native returns
non-OK. Release runs once, waits for in-flight completion or cancellation
checks, and makes later completion or cancellation checks fail before crossing
into C. Stale public request handles cannot affect later native requests.

---

## Threading

The C API owner-thread model is visible at the binding layer. Ordinary public
methods call C synchronously on the calling native thread and surface the C
owner-thread status when called from the wrong thread.

### Owner-thread helpers

Provide an owner-thread execution helper when the host language can move a
logical task across native threads or cannot otherwise give safe callers a
stable native owner thread for a runtime/map lifecycle. Bindings with stable
native caller identity expose ordinary methods and wrong-thread errors without
this helper.

The helper follows this design:

1. It owns or binds one native owner thread before creating thread-affine
   handles.
2. It runs submitted operations by calling the ordinary low-level binding
   methods on that owner thread.
3. It serializes submitted operations with event polling and close on that owner
   thread.
4. It returns the ordinary binding result or error shape, including copied
   native diagnostics.
5. Closing rejects new submissions, releases thread-affine handles on the owner
   thread, and leaves later submissions in the binding's closed-state error
   shape.

### Event polling

The public event API is explicit: host code pumps native runtime work, then
polls one queued runtime event. Polling returns one copied event or empty.

Event polling follows this operation:

1. Initialize the native event struct and `has_event` out parameter before
   calling C.
2. Call the C poll function on the runtime owner thread.
3. If no event is available, return the language's empty result.
4. Copy the event type, source type, status code, message bytes, payload bytes,
   and typed payload fields before another poll can invalidate native event
   storage.
5. Decode known typed payloads only after validating their native size. Preserve
   unknown event and payload domains with their raw values and copied payload
   bytes.
6. Resolve event source identity through binding-owned runtime state. A
   map-originated event may reference an existing public map wrapper or copied
   map identity only when the binding can prove that identity. It never creates
   a public handle from the native source pointer.
7. Apply binding-owned state updates triggered by the event before returning the
   copied event.

### Transferability

When the language can declare or enforce cross-thread transferability, ordinary
owner-thread handles MUST be non-transferable. A transferable owner-thread
helper handle is allowed only when every operation is submitted back to the
bound native owner thread. Copied immutable values can be transferable when
their contents are independent of native owner-thread state. Unchecked or unsafe
concurrency conformance MUST name the synchronization invariant that makes it
sound.

---

## Rendering

Rendering bindings expose render sessions, frame lifetimes, and readback without
taking ownership of caller-owned backend resources.

### Render sessions

Render-session attach APIs cover the C API session families:

- Surface sessions render and present through a host surface.
- Session-owned texture sessions render into a texture or image created by the
  session.
- Caller-owned texture sessions render into a host-owned texture or image.

Attach follows this operation:

1. Materialize the backend-specific public descriptor into the matching C
   descriptor.
2. Pass backend-native host resources as `NativePointer` values.
3. Call the matching C attach function on the map owner thread.
4. Return a distinct `RenderSessionHandle` for the map's one live render
   session.
5. Surface unsupported backend, unsupported render-target mode,
   existing-session, wrong-thread, and native errors through the binding's
   status mapping.

For host-owned backend resources, the binding does not release or synchronize
those resources. The caller keeps them valid for the C API's documented borrow
window.

The public handle exposes:

- `resize` for session kinds that support resize;
- `render_update` for the latest available map render update;
- `detach`, which keeps the public handle live after backend resources detach;
- `close` or `destroy`, using the owned-handle release operation.

### Texture frames

Session-owned texture frames are scoped borrows.

Frame acquisition follows this operation:

1. Acquire the native frame and create an explicit frame handle.
2. Copy public metadata from the native frame.
3. Expose backend handles only through active-frame checked accessors.
4. While the frame is active, reject nested frame acquisition and every exposed
   session operation whose C contract forbids execution during an active frame.
5. Release follows the owned-handle release operation. Failed native frame
   release leaves the frame live for retry.
6. If wrapper construction fails after native frame acquisition, release the
   acquired native frame.

Copyable frame handles include stale-handle protection so old frame copies
cannot expose backend handles after release or after a later frame reuses the
same storage.

### Readback

CPU texture readback accepts caller-owned mutable storage.

Readback returns copied `TextureImageInfo` metadata. Buffer-capacity failures
preserve the caller's buffer ownership and map to the binding's error mechanism.
Public buffer reads return copied or read-only views unless the binding proves
exclusive mutable access.

---

## Test Cases

Each binding test suite includes the tests below. Conditional tests become
required when the binding has the named host-language mechanic, configured
render backend, or C API platform support.

Public behavior tests use public binding APIs and real C calls when behavior
crosses the binding/C boundary. Tests focus on high-value native workflows and
binding-owned safety invariants, not trivial constant assertions or exhaustive
invalid-input matrices for validation owned by C.

### Test execution strictness

Missing dependencies, configured render backends, platform setup, or CI
capabilities are test failures, not skips. Skips are limited to tests that are
inapplicable because the configured backend or platform support is absent, or
the target is outside the binding's documented supported platforms.

Skips MUST be declared explicitly in the test file or through a shared
capability check. Each skip states the inapplicable backend, platform, or
host-language condition. Individual tests MUST NOT convert setup, loading,
rendering, or native-call failures to skips.

### Test seams

Tests SHOULD use public binding APIs for public behavior. Internal test seams
are allowed for behavior that cannot be produced reliably through the public
native library:

- ABI mismatch before public handle creation;
- native status conversion for status categories whose C producers are
  nondeterministic, backend-dependent, or native-exception-only;
- unknown future status, enum, event, or payload values;
- native destroy, request release, frame release, and callback-install failure;
- allocation or copy failure after a native snapshot/list/result handle is
  acquired;
- in-flight or concurrent callback and release races that require deterministic
  scheduling.

Internal seams MUST assert the same public error, lifetime, and cleanup behavior
that a real native failure would expose.

### Loading

| ID      | Test                                                                                                                                          |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-001 | ABI-version mismatch returns the binding's ABI-version error before storing a public native handle, using an internal loader or version seam. |

### Status and diagnostics

| ID      | Test                                                                                                                                      |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| BND-020 | Each native status category maps to the expected public error category.                                                                   |
| BND-021 | Unknown native status preserves the raw status value, using an internal conversion hook when no real C call can produce a future status.  |
| BND-022 | A native diagnostic is copied immediately and remains available after a later C call changes thread-local state.                          |
| BND-023 | Binding-owned closed-handle validation returns the documented public error before crossing into C.                                        |
| BND-024 | Invalid string input containing embedded `NUL` is rejected for null-terminated C inputs.                                                  |
| BND-025 | Binding-owned validation produces a fresh binding diagnostic and does not expose stale native thread-local diagnostics.                   |
| BND-026 | A public failing call that performs binding cleanup or support work still reports the original native diagnostic, not a later diagnostic. |

### Handle lifetime

| ID      | Test                                                                                                                                                           |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-040 | Runtime creation followed by explicit release destroys the native handle exactly once; every public alias observes release state, and a second release no-ops. |
| BND-041 | A failed native destroy leaves the handle live; a later successful release destroys the native handle.                                                         |
| BND-042 | A child handle retains parent owner state, and parent release fails while child handles are live.                                                              |
| BND-043 | `MapProjectionHandle` remains usable after the source map closes and then releases successfully.                                                               |

### Input Structs, Values, and Copied Data

| ID      | Test                                                                                                                             |
| ------- | -------------------------------------------------------------------------------------------------------------------------------- |
| BND-060 | Representative input-struct tests per family initialize C defaults, `size` fields, field masks, and nested inputs.               |
| BND-061 | Optional field-mask inputs distinguish absent values from present zero values.                                                   |
| BND-062 | Unknown output enum values preserve the raw native value, using an internal conversion hook when no real C call can produce one. |
| BND-063 | Borrowed native strings and string views are copied before their native borrow window ends.                                      |
| BND-064 | JSON values round-trip scalar and nested container values without type loss.                                                     |
| BND-065 | GeoJSON values copy nested geometries, features, properties, and identifiers.                                                    |
| BND-066 | Native snapshot/list/result handles are released on success and on copy failure, using fault injection for copy failure.         |
| BND-067 | Structured JSON preserves object member order, repeated member names, and signed or unsigned integer width.                      |
| BND-068 | Unknown enum values preserve their raw value and are rejected before crossing into C when used in public input APIs.             |

### Runtime and events

| ID      | Test                                                                                                                         |
| ------- | ---------------------------------------------------------------------------------------------------------------------------- |
| BND-080 | `run_once` drives native event processing through the public runtime API, and repeated event polling reaches an empty queue. |
| BND-081 | Map style loading returns the expected copied map event through polling and identifies the correct public map identity.      |
| BND-082 | Event message and payload data remain valid after the next event poll.                                                       |
| BND-083 | Unknown event or payload domains preserve raw values and copied bytes when the C API exposes those bytes.                    |
| BND-084 | Offline operation completion returns copied result data and leaves failed take-result handles retryable.                     |
| BND-085 | Offline region observation returns copied status/error events through the public runtime event model.                        |
| BND-086 | A map-originated event with no provable live public map exposes no public map handle or borrowed native pointer.             |
| BND-087 | Known typed event payloads validate native payload size before reading payload fields.                                       |

### Map, camera, projection, style, and query

| ID      | Test                                                                                                                                |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| BND-100 | Map creation applies public map options, extent, and mode, then releases through the runtime parent relationship.                   |
| BND-101 | Style URL and style JSON loading succeed through public map APIs and return copied style-loaded events through polling.             |
| BND-102 | Camera set/get, animated camera commands, and transition cancellation produce the expected native camera state and statuses.        |
| BND-103 | Projection helpers round-trip screen, lat/lng, and projected-meter values through copied public values within documented tolerance. |
| BND-104 | Representative invalid map and projection inputs propagate native invalid-argument diagnostics through the public error shape.      |
| BND-105 | Style source, layer, image, and feature-state workflows add, update, query/list, and remove public input values and copied IDs.     |
| BND-106 | Query workflows return copied feature geometry, properties, feature state, source/layer identifiers, and unknown IDs.               |

### Logging and callbacks

| ID      | Test                                                                                                                                                        |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-120 | Log callback install invokes the registered callback, clear prevents later invocation, and replacement invokes only the replacement callback.               |
| BND-121 | Host-language failures do not unwind or escape across the C callback boundary, and recoverable callback failures are converted to documented C behavior.    |
| BND-122 | Each exposed callback family preserves the previous callback and releases replacement state when replacement fails.                                         |
| BND-123 | Callback state remains synchronized for callback families whose C contract allows concurrent invocation.                                                    |
| BND-124 | Custom geometry or style-scoped callback teardown handles style reload, source removal, source ID reuse, map close, and in-flight upcalls without late use. |

### Resources

| ID      | Test                                                                                                                                                |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-140 | Resource transform can rewrite a URL and can be cleared after registration.                                                                         |
| BND-141 | Resource transform request data is copied into language-owned values before user code receives it.                                                  |
| BND-142 | Resource provider pass-through delegates to native loading without retaining a request handle.                                                      |
| BND-143 | Resource provider handled request can complete inline and load a style.                                                                             |
| BND-144 | Resource provider handled request can complete later and load a style.                                                                              |
| BND-145 | Handled request can complete from another thread.                                                                                                   |
| BND-146 | Completing a handled request twice reports the binding's already-completed error before crossing into C.                                            |
| BND-147 | Releasing a handled request makes later completion and cancellation checks fail as closed.                                                          |
| BND-148 | Request cancellation is observable before a late completion, and late completion maps native status.                                                |
| BND-149 | Resource error responses become copied runtime loading-failure or offline-error events.                                                             |
| BND-150 | Inline completion during the provider callback finalizes handled ownership even when the callback's later return path would otherwise pass through. |
| BND-151 | Stale request handles cannot complete, cancel, or release later native requests.                                                                    |
| BND-152 | Completion that reaches C is terminal even when native completion returns a non-OK status.                                                          |
| BND-153 | Releasing a request waits for in-flight completion or cancellation checks before native release.                                                    |

### Rendering

| ID      | Test                                                                                                                                                      |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-160 | Supported render-backend queries gate configured workflows and unsupported backend/mode errors.                                                           |
| BND-161 | Render-target descriptors materialize extents and `NativePointer` backend handles without taking ownership.                                               |
| BND-162 | Surface, session-owned texture, and caller-owned texture attach paths call the matching C session family and report the same public session handle shape. |
| BND-163 | Attaching a second render session to the same map reports invalid state.                                                                                  |
| BND-164 | `render_update` maps invalid state without closing the session.                                                                                           |
| BND-165 | Resize updates extent through the public render session API.                                                                                              |
| BND-166 | CPU readback copies metadata; undersized buffers fail without losing ownership, and sufficiently sized reusable buffers receive image bytes.              |
| BND-167 | Owned texture frame acquire returns an explicit frame handle with copied metadata and active-checked backend handles.                                     |
| BND-168 | Owned texture frame access after release fails before exposing backend handles.                                                                           |
| BND-169 | Failed frame release leaves the frame live and a later successful release closes it.                                                                      |
| BND-170 | Nested frame acquisition and every exposed session operation forbidden during an active frame fail while a frame is active.                               |
| BND-171 | Caller-owned texture descriptors do not release or mutate caller-owned backend handles during session close.                                              |
| BND-172 | Wrapper construction failure after native frame acquisition releases the native frame.                                                                    |
| BND-173 | Stale frame handles cannot expose backend handles after release or reuse.                                                                                 |

### Conditional tests

The following tests apply only when a binding has the named host-language
mechanic or helper.

#### Host cleanup hooks

When the host language can run cleanup outside explicit release, include:

| ID      | Test                                                                                                                             |
| ------- | -------------------------------------------------------------------------------------------------------------------------------- |
| BND-044 | Non-deterministic cleanup hooks report leaked thread-affine handles rather than destroying them.                                 |
| BND-048 | Best-effort cleanup failure is reported through the binding's documented leak or failure channel and explicit release can retry. |

#### Cross-thread public handle use

When safe public code can call owner-thread-affine APIs from the wrong native
thread or race release on the same owner-thread handle, include:

| ID      | Test                                                                                                     |
| ------- | -------------------------------------------------------------------------------------------------------- |
| BND-046 | Concurrent releases call native release at most once and public calls fail while release is in progress. |
| BND-190 | Owner-thread-affine calls from a different native thread report the binding's wrong-thread error.        |
| BND-191 | Runtime wrong-thread errors include the copied native diagnostic.                                        |

#### Owner-thread execution adapters

When the subproject ships an owner-thread execution adapter, include:

| ID      | Test                                                                       |
| ------- | -------------------------------------------------------------------------- |
| BND-192 | The adapter confines create, pump, event polling, and close to one thread. |
