---
title: Language binding
description: Specification for low-level language bindings over the C API.
sidebar:
  order: 6
---

Specification for language binding subprojects that expose MapLibre Native
through the public C API.

This specification defines the common low-level binding contract. Existing
bindings are not grandfathered; new work moves bindings toward this document
rather than preserving divergent local patterns.

## Scope

### What every binding provides

- One safe low-level public API over `maplibre_native_c.h`.
- Direct exposure of MapLibre concepts: runtime, map, camera, style, resources,
  events, diagnostics, render sessions, render targets, query results, and
  offline operations.
- Explicit deterministic release for every public wrapper that owns a native
  handle.
- Native status and diagnostic translation through the target language's normal
  error mechanism.
- Copied language values for native events, snapshots, lists, strings, JSON,
  GeoJSON, query results, offline region data, and other C-owned borrowed
  output.
- Explicit lifetime control for callbacks, resource requests, and scoped render
  frame borrows.
- Tests for every binding-owned ownership, threading, copying, callback, and
  error-mapping invariant listed in [Test cases](#test-cases).

### What this layer is not

The binding is a low-level FFI layer. It MUST NOT add application-framework
policy, UI/view lifecycle integration, or scheduler/async execution models above
the C API concepts.

---

## Priorities

When requirements conflict, resolve them in this order:

1. Target-language safety idioms over cross-binding alignment.
2. Cross-binding alignment over target-language syntactic idioms.

Cross-binding alignment means bindings expose the same C API concepts, ownership
rules, operation boundaries, and error semantics. It does not require identical
syntax, names, or package structure.

Bindings expose one correct low-level way to perform each C API operation. A
binding adds another public path only when that path enforces a target-language
safety rule that the primary path cannot express.

Bindings are minimal and complete:

- Minimal: public APIs map to C API concepts and binding-owned safety policy.
  They do not add redundant workflows, shortcuts, or higher-level composites.
- Complete: a binding that claims support for a C API domain exposes that
  domain's public operations, descriptor shapes, event payloads, status
  behavior, and ownership rules.

## Architecture

Every binding splits implementation into three layers:

| Layer                  | Responsibility                                                                                      |
| ---------------------- | --------------------------------------------------------------------------------------------------- |
| Raw C layer            | Generated or handwritten declarations for the public C headers, raw structs, constants, and calls.  |
| Internal support layer | Status conversion, diagnostics, handle state, memory guards, callback state, native loading, shims. |
| Public binding layer   | Handles, descriptors, values, events, callbacks, errors, and render-target APIs for users.          |

Requirements:

- Raw C declarations MUST stay outside the safe public API. If the target
  language cannot hide raw declarations, they MUST live under a generated
  interop namespace and be excluded from the supported safe API surface.
- Host FFI carrier types such as raw memory segments, arenas, method/function
  handles, generated layout wrappers, foreign references, unsafe pointers, and
  raw entrypoints MUST stay outside the safe public API. Public APIs expose
  backend addresses only through `NativePointer`.
- Public examples and tests MUST use the public binding layer, not raw C calls,
  except bindability and layout tests.
- Generated code MUST be mechanically reproducible from public headers,
  metadata, or generator inputs checked into the repository.
- Handwritten support code MUST follow repeatable patterns that can be audited
  across domains.
- Native library loading and ABI-version checks MUST fail with a binding error
  before a public wrapper stores an incompatible native handle. The expected ABI
  version MUST come from generated binding metadata or a checked-in binding
  constant, and ABI mismatch MUST use a stable public error category.

---

## Public Surface

### Naming

Bindings follow
[Binding Conventions](/maplibre-native-ffi/development/bindings/#naming).

Long-lived C-owned opaque handles use the `Handle` suffix:

- `RuntimeHandle`
- `MapHandle`
- `MapProjectionHandle`
- `RenderSessionHandle`
- `OfflineOperationHandle`
- `ResourceRequestHandle`

`Handle` means the public value owns or controls an explicitly releasable native
resource with identity across operations. This includes pointer-backed handles,
ID-backed handles, registry-backed request tokens, and operation handles.

### Domain coverage

A binding that supports a domain MUST expose the domain through public wrapper
types, not raw ABI structs.

| Domain             | Required public concepts                                                                                        |
| ------------------ | --------------------------------------------------------------------------------------------------------------- |
| Library globals    | C ABI version check, supported render-backend mask, network status, logging.                                    |
| Diagnostics/errors | Stable status categories, raw unknown status preservation, copied diagnostic message.                           |
| Runtime            | Runtime options, create/close, `run_once`, event polling, resource transforms, resource providers, offline ops. |
| Map                | Map options, create/close, style URL/JSON, debug options, render events, camera, projection, query operations.  |
| Style              | Style sources, layers, images, feature state, source/layer ID lists, style JSON values.                         |
| Values             | Camera values, geometry, GeoJSON, JSON, screen coordinates, bounds, edge insets, tile IDs, offline values.      |
| Rendering          | Render sessions, extent, backend descriptors, texture frames, readback, backend support queries.                |

Domain support is explicit. A binding supports a domain only when it documents
the domain and exposes the domain's public concepts through the binding layer.
Unsupported domains stay absent from the safe public API.

### One public shape per concept

Each concept uses one public representation:

- one public status/error family;
- one public JSON value model;
- one public GeoJSON value model;
- one public native-pointer value;
- one public runtime event model;
- one public render session model;
- one public descriptor style per concept.

Package re-exports and type aliases name the same underlying public
representation. They MUST NOT create another ownership, threading, callback,
allocation, or error contract.

---

## Handle Lifetime

### Owned handles

Every public wrapper for a long-lived C-owned handle or handle-like native token
MUST store:

- private native identity;
- live/releasing/closed state;
- the native destroy function or bridge release path;
- parent references or lifetime evidence required for native validity;
- callback state owned by that handle's native scope;
- optional leak-reporting context.

Native identity MUST NOT be public. The implementation can represent it with a
native pointer, bridge handle, or registry token, but every public operation
uses the same ownership rules.

Release requirements:

- Public release MUST be deterministic and explicit, with a name that follows
  the target language's resource-release convention.
- Release MUST call the matching C destroy function exactly once after a
  successful native release.
- Release MUST no-op after a successful release.
- If native release returns a non-OK status, the wrapper MUST remain live so the
  caller can retry and inspect diagnostics.
- If the public release operation consumes or moves the wrapper, a failed native
  release MUST return the live owner state for retry.
- Public methods MUST reject use after successful release before crossing into
  C.
- Public methods MUST reject use while a release is in progress before crossing
  into C. Concurrent release attempts MUST synchronize so at most one native
  release is in progress, successful release wins exactly once, and failed
  release restores the live state.
- Parent references, callback state, request registries, and other owner-scoped
  support state MUST remain live until native owner release succeeds.
- Non-deterministic cleanup hooks MUST report leaks for thread-affine handles.
  They MUST NOT destroy runtime, map, projection, or render-session handles from
  cleanup hooks.
- Infallible language destructors that attempt best-effort release MUST preserve
  the explicit release contract and MUST NOT mask native errors from the
  explicit release path.

### Parent validity

Bindings MUST preserve native parent validity while child wrappers are live:

| Child                       | Parent validity requirement                                                                        |
| --------------------------- | -------------------------------------------------------------------------------------------------- |
| `MapHandle`                 | Runtime remains live while the map is live.                                                        |
| `RenderSessionHandle`       | Map remains live while the render session is live.                                                 |
| Style-scoped callback state | Owning map/style remains live until native callback unregistration and in-flight callbacks finish. |
| Resource provider request   | Native request remains retained until completion, cancellation handling, or explicit release.      |
| Session-owned texture frame | Render session remains live and the frame remains acquired until the frame handle releases.        |

Child wrappers MUST retain the parent owner state. Releasing a parent while
children are live MUST fail without consuming or destroying the parent.

`MapProjectionHandle` is the exception: after creation it owns a standalone
projection snapshot. It MUST remain valid after the source map closes and MUST
release with `mln_map_projection_destroy()`.

### Handle copying

Owned handle wrappers are affine: the safe public API MUST NOT create duplicate
owners. Reference-copy languages MUST make all references share one owner state.
Value-copy languages MUST make owned handles non-copyable or move-only. Public
code MUST NOT be able to fabricate live handles, ID-backed operation tokens, or
request tokens from raw integers, raw addresses, public fields, or ordinary
constructors. Any unsafe constructor for backend interop MUST produce a borrowed
value, not an owned handle.

---

## Status And Diagnostics

Every status-returning C call maps to the target language's normal error
mechanism.

Requirements:

- `MLN_STATUS_INVALID_ARGUMENT`, `MLN_STATUS_INVALID_STATE`,
  `MLN_STATUS_WRONG_THREAD`, `MLN_STATUS_UNSUPPORTED`, and
  `MLN_STATUS_NATIVE_ERROR` MUST map to stable public categories.
- Unknown future status values MUST map to a stable unknown-status category and
  preserve the raw native value.
- The binding MUST copy `mln_thread_last_error_message()` immediately after a
  non-OK status on the same thread, before any other diagnostic-writing C call.
- Public errors MUST expose the copied diagnostic string when the target
  language supports payloads.
- Binding-owned validation failures MUST use the same public error family. They
  MUST provide a fresh binding diagnostic and MUST NOT read stale native
  thread-local diagnostics.
- When public errors cannot carry payloads, the binding MUST expose a
  deterministic diagnostic store for the failing call and document how child
  handles inherit that store.
- If public errors expose raw native status, binding-owned validation failures
  MUST be distinguishable from native non-OK statuses.
- Native `MLN_STATUS_WRONG_THREAD` MUST surface as the binding's wrong-thread
  error. The binding MUST NOT silently dispatch the call to another thread.

Binding-owned validation covers:

- closed wrappers;
- active scoped borrows;
- one-shot resource request completion;
- invalid language string shape for C inputs;
- unsupported callback shapes;
- ABI version mismatch;
- host-owned buffer size mismatches that the binding can detect before C.

Native validation remains authoritative for MapLibre state, enum domains, native
handle validity, numeric ranges, and owner-thread identity.

---

## Type Mapping

### Descriptors and structs

C option structs become language-owned descriptors.

Requirements:

- Public callers MUST set semantic fields, not ABI `size` fields, masks, or raw
  nested C storage.
- Materializers MUST initialize every C struct to the C API defaults before
  setting public fields. When the C API provides a default constructor,
  materializers MUST call it.
- Field-mask descriptors MUST provide explicit present/absent behavior for each
  optional field.
- Descriptor storage passed to C MUST live for the complete C borrow window,
  including recursive descriptor trees and interior pointers.
- Structs that C fills and returns by value MUST become copied language values.

### Enums and masks

Requirements:

- Closed C enum domains MUST map through explicit raw conversions.
- Public code MUST NOT rely on language enum ordinal values matching C ABI
  values.
- Future-extensible output domains MUST preserve unknown raw values.
- Bit masks MUST use an idiomatic set, flags enum, option set, packed wrapper,
  or purpose-built mask type.
- C field masks MUST remain internal to descriptor materializers.

### Strings

Requirements:

- Public string inputs use UTF-8 at the C boundary.
- Null-terminated `const char*` inputs MUST reject embedded `NUL`.
- Explicit-length `mln_string_view` inputs MUST pass UTF-8 bytes and byte
  length. They allow embedded `NUL` exactly when the C contract allows it.
- Borrowed C strings and string views MUST be copied before their native borrow
  window ends.

### JSON and GeoJSON

Structured JSON and GeoJSON models MUST preserve MapLibre value semantics:
object member order, repeated member names, signed and unsigned integer width,
floating-point values, booleans, nulls, strings, arrays, and nested objects. Raw
JSON or GeoJSON text inputs MUST pass through as text without reparsing or
reformatting unless the public API is explicitly a structured-value API.

### Native pointers

`NativePointer` represents a borrowed opaque backend-native address.

Requirements:

- `NativePointer` MUST transfer no ownership.
- `NativePointer` MUST grant no general memory access.
- Public APIs MUST accept `NativePointer` only where the C API accepts
  host-owned opaque backend handles.
- Conversion from raw addresses or raw pointers MUST be internal or exposed only
  as an unsafe or borrowed backend-interop constructor. Public constructors MUST
  state the backend lifetime and synchronization requirements.
- Public fields or constructors MUST NOT make a `NativePointer` look like an
  owned handle or grant safe access to arbitrary memory.
- Backend pointers returned from acquired texture frames MUST perform
  active-frame checks before exposing the pointer.

---

## Data Ownership

### Temporary native storage

Bindings materialize most native input at the call boundary.

Requirements:

- Temporary C structs, strings, arrays, out parameters, and recursive pointer
  graphs MUST live until the C call returns or the full documented native borrow
  window ends.
- Temporary pointers MUST NOT be stored in public objects unless the object owns
  the native storage and releases it deterministically.
- Materializers MUST release temporary native allocations on every failure path.
- Out parameters and response structs passed into callbacks MUST be initialized
  to the C API's neutral value before user code runs.

### Copied output

Native snapshot, result, and list handles are internal implementation details.

Requirements:

- Public snapshot/list/result APIs MUST return copied language-owned data.
- Internal readers MUST release native snapshot/list/result handles exactly once
  after copying, including failure paths.
- Runtime event polling MUST return copied language values independent of the
  next native poll.
- Unknown event and payload domains MUST preserve raw values and copied payload
  bytes when the C API exposes those bytes.
- Map-originated events MUST identify a live source map when identity can be
  proven. If lookup misses, they MUST carry no public map handle or only copied
  source metadata; they MUST NOT expose a dangling borrowed native handle.

## Callbacks And Requests

### Callback lifetime

Callback state MUST be stored strongly for the native scope that can invoke it:

| Callback kind         | Native scope                                                                                           |
| --------------------- | ------------------------------------------------------------------------------------------------------ |
| Logging               | Process-global until replaced or cleared and in-flight invocations finish.                             |
| Resource transform    | Runtime until replaced, cleared, runtime close makes it unreachable, and in-flight invocations finish. |
| Resource provider     | Runtime until replaced, cleared, runtime close, and in-flight requests or invocations end.             |
| Custom geometry/style | Map/style/source scope until native unregistration and in-flight invocations finish.                   |

Requirements:

- Callback adapters MUST pass copied language-owned callback arguments to user
  code. A lexical view is allowed only when the public type prevents retention
  beyond the callback invocation.
- Callback adapters MUST catch host exceptions, panics, or errors and convert
  them to the C callback's documented behavior.
- Host failures MUST NOT unwind through native frames.
- Callback state that can be invoked concurrently MUST use explicit
  synchronization.
- Callback code MUST return quickly and hand owner-thread work back to the owner
  thread before calling runtime or map APIs.
- Replacing a callback MUST install the new native descriptor before releasing
  old callback state. If installation fails, the old callback state MUST remain
  active and the replacement state MUST be released.
- Unregistering callback state by clearing, replacing, or closing MUST prevent
  new upcalls, wait for in-flight upcalls, and release callback roots only after
  native can no longer invoke them.
- If a leaked native owner can still reach callback user data, non-deterministic
  cleanup MUST report the leak and keep callback memory reachable from native
  alive.
- Style-scoped callback retention and release MUST be driven by current native
  source ownership, not by stale event timing or source ID reuse alone.

### Resource transforms

Resource transform callbacks are synchronous.

Requirements:

- Request URL and metadata exposed to user code MUST be copied language-owned
  values.
- Replacement URLs MUST be copied into the native response shape before the
  callback returns.
- Returning no rewrite MUST map to the C API's pass-through behavior.
- Host failure, validation failure, or no rewrite MUST leave the native response
  shape in the neutral pass-through state.

### Resource providers

Resource providers decide whether a request passes through to the native
provider or is handled by the binding.

Requirements:

- Non-matching requests MUST return pass-through without retaining the native
  request handle.
- Matching handled requests MUST copy request fields before user code can retain
  them.
- A handled request MUST own the native request reference until completion,
  cancellation handling, or release.
- Inline completion during the provider callback finalizes the request as
  handled ownership, regardless of the callback's later return path.
- Completion MUST be one-shot.
- Completion that reaches the C API MUST be terminal, including non-OK native
  completion results.
- Release MUST be exactly once.
- Release MUST wait for in-flight completion or cancellation checks before
  releasing the native request reference.
- Late use after release MUST report a binding error before crossing into C.
- When the C API allows deferred or cross-thread completion, the binding MUST
  preserve that capability without changing one-shot or release behavior.
- Cancellation checks MUST preserve the request handle's one-shot and release
  state.
- Registry-backed request handles MUST include stale-handle or ABA protection so
  copied old handles cannot complete, cancel, or release a later request that
  reused the same slot or integer identity.

---

## Threading

The C API owner-thread model is visible at the binding layer.

Requirements:

- Runtime creation records the owner thread; map, projection, and render session
  operations follow the C API owner-thread rules.
- Ordinary public methods MUST call C synchronously on the calling native
  thread.
- The binding MUST NOT dispatch ordinary calls to another host scheduler,
  executor, event loop, worker queue, or UI thread.
- Scheduler or owner-thread execution adapters MUST NOT change ordinary binding
  call semantics.
- Runtime event draining MUST happen only when host code calls the binding's
  event polling/draining API.
- Signals or language events that mirror runtime events MUST be emitted only
  while the owner thread is explicitly draining the C runtime event queue.

Languages with static or runtime concurrency markers MUST represent owner-thread
handles as not freely transferable across threads unless the binding proves a
stronger invariant. Copied immutable values can be transferable when their
contents are independent of native owner-thread state. Unchecked or unsafe
concurrency conformance MUST name the synchronization invariant that makes it
sound and MUST be covered by binding tests.

---

## Rendering

### Render sessions

Requirements:

- `RenderSessionHandle` MUST be a distinct public handle for one attached render
  target on one map.
- Attach APIs MUST return `RenderSessionHandle`.
- Public render-target descriptors MUST expose backend-native handles as
  `NativePointer`.
- Borrowed backend handles MUST remain caller-owned and caller-synchronized.
  Passing them MUST NOT transfer ownership to the binding.
- The binding MUST surface `MLN_STATUS_UNSUPPORTED` when a requested native
  backend or render-target mode is unavailable.
- The binding MUST expose supported render-backend queries.

### Texture frames

Session-owned texture frames are scoped borrows.

Requirements:

- Acquiring a frame MUST produce an explicit frame handle.
- Frame metadata exposed publicly MUST be copied from the native frame.
- Backend handles exposed from the frame MUST be valid only while the frame is
  active.
- Access after frame release MUST fail before returning a backend handle.
- Frame release MUST no-op after successful release.
- Failed native frame release MUST leave the frame live for retry.
- The binding MUST reject nested frame acquisition and every exposed session
  operation whose C contract forbids execution while a frame is active,
  including render update, resize, readback, query, detach, release, or session
  destruction.
- If wrapper construction fails after native frame acquisition, the binding MUST
  release the acquired native frame.
- Copyable or registry-backed frame handles MUST include stale-handle protection
  so old frame copies cannot expose backend handles after release or after a
  later frame reuses the same storage.

### Readback

Requirements:

- CPU texture readback MUST use caller-owned reusable storage when the target
  language can express mutable external storage safely. Languages without a safe
  caller-owned buffer shape return an explicitly allocated language-owned buffer
  and document the allocation path.
- Readback MUST return copied `TextureImageInfo` metadata.
- Buffer-capacity failures MUST preserve the caller's buffer ownership and map
  to the binding's error mechanism.
- Public buffer reads MUST return copied or read-only views unless the binding
  proves exclusive mutable access. Reusable native buffers MUST reject use after
  release before crossing into C.

---

## Host Constraints

Host-language constraints refine the common contract; they do not create
alternate binding designs.

| Constraint                         | Binding requirement                                                                                                 |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| Package calls C headers directly   | Raw declarations stay below the safe public API; public wrappers own safety policy.                                 |
| Package uses a native bridge       | The bridge exposes the same low-level binding contract and keeps raw C details internal to the bridge.              |
| Cleanup can run arbitrarily        | Cleanup hooks report leaks for thread-affine handles; explicit release performs native destruction.                 |
| Scope cleanup or destructors exist | Explicit release remains observable for fallible native release; scope-bound cleanup does not hide fallible errors. |
| Public references are copyable     | Copied references share one owner state.                                                                            |
| Public values are copyable         | Owned handles are non-copyable or move-only.                                                                        |
| Logical execution moves threads    | Ordinary calls run on the current native thread; scheduler confinement belongs outside the core binding.            |
| Introspection metadata is public   | Metadata accurately records transfer, nullability, closure, array length, and errors.                               |

Owner-thread execution helpers belong above the core binding. If a binding
subproject ships one, it is documented as an adapter API and keeps the core
binding's owner-thread model visible.

---

## Documentation

Each binding subproject MUST document:

- supported platforms and native render backends;
- how the native library is found or linked;
- how to run that binding's tests;
- which C API domains are implemented;
- any owner-thread execution adapter shipped in the binding subproject;
- unsafe APIs and caller obligations;
- non-deterministic cleanup or leak-reporting behavior;
- callback threading behavior.

Language-specific convention docs remain the place for implementation details
such as generator tools, package manager choices, module names, and idiomatic
spelling.

---

## Test Cases

Binding tests prove both the language adaptation layer and the native behavior
that users reach through that binding. This matters on targets where the Zig
test suite does not run. Tests use the public binding API to cover high-value
native workflows plus every binding-owned ownership, copying, callback,
threading, and error-mapping invariant.

The matrix below is comprehensive for a binding's supported domains. Tests
marked "Applies when" are required when the binding exposes that capability or
matches that host constraint.

### Test execution strictness

Missing native libraries, native dependencies, configured render backends,
platform setup, or CI capabilities are test failures, not skips. Skips are
limited to tests that are inapplicable because the binding does not expose the
domain, the C API does not support the capability on the target, an "Applies
when" host constraint is absent, or the target is outside the binding's
documented supported platforms.

Skips MUST be declared explicitly in the test file or through a shared
capability check. Individual tests MUST NOT convert setup, loading, rendering,
or native-call failures to skips.

### Build and bindability

| ID      | Test                                                                                                                                                                                                         |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| BND-001 | The raw C layer compiles or generates from `maplibre_native_c.h` without public API edits.                                                                                                                   |
| BND-002 | Public binding tests load or link the real native library used by the subproject.                                                                                                                            |
| BND-003 | ABI-version mismatch returns the binding's ABI-version error before storing a public native handle.                                                                                                          |
| BND-004 | Public API surface tests prevent raw C structs, raw C handles, raw FFI carrier types, and generated declarations leaking through public types, methods, fields, properties, delegates, or generic arguments. |
| BND-005 | Applies when generated layouts exist: representative generated struct layouts match the C ABI.                                                                                                               |
| BND-006 | Applies when introspection metadata exists: metadata generation and a small consumer compile test pass.                                                                                                      |
| BND-007 | Missing native libraries, missing native dependencies, load failures, ABI mismatch, and unavailable configured render backends fail the test run rather than skip tests.                                     |

### Status and diagnostics

| ID      | Test                                                                                                                                                                             |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-020 | Each native status category maps to the expected public error category through one real failing C call.                                                                          |
| BND-021 | Unknown native status maps to the unknown-status category and preserves the raw status value, using an internal conversion hook when no real C call can produce a future status. |
| BND-022 | A native diagnostic is copied immediately and remains available after a later C call changes thread-local state.                                                                 |
| BND-023 | Binding-owned closed-handle validation returns the documented public error before crossing into C.                                                                               |
| BND-024 | Invalid string input containing embedded `NUL` is rejected for null-terminated C inputs.                                                                                         |
| BND-025 | Binding-owned validation produces a fresh binding diagnostic and does not expose stale native thread-local diagnostics.                                                          |
| BND-026 | A public failing call that performs binding cleanup or support work still reports the original native diagnostic, not a later diagnostic.                                        |

### Handle lifetime

| ID      | Test                                                                                                                                                                                        |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-040 | Runtime creation followed by explicit release destroys the native handle exactly once; a second release no-ops.                                                                             |
| BND-041 | A failed native destroy leaves the handle live; a later successful release destroys the native handle.                                                                                      |
| BND-042 | A child handle retains parent owner state, and parent release fails while child handles are live.                                                                                           |
| BND-043 | Runtime release fails while maps are live; after maps release, runtime release succeeds.                                                                                                    |
| BND-044 | `MapProjectionHandle` remains usable after the source map closes and then releases successfully.                                                                                            |
| BND-045 | Applies when non-deterministic cleanup hooks exist: leaked thread-affine handles report leaks rather than destroy.                                                                          |
| BND-046 | Applies when handles can be copied by the host language: copied references share owner state, and owned value handles are non-copyable or move-only.                                        |
| BND-047 | Applies when release can run concurrently: concurrent releases call native release at most once and public calls fail while release is in progress.                                         |
| BND-048 | Public APIs cannot fabricate live owned handles, ID-backed operation handles, or request tokens from raw integers, raw addresses, public fields, or ordinary constructors.                  |
| BND-049 | Applies when infallible cleanup calls best-effort release: best-effort failure is reported through the binding's documented leak or failure channel and explicit release remains retryable. |

### Descriptors, values, and copied data

| ID      | Test                                                                                                                             |
| ------- | -------------------------------------------------------------------------------------------------------------------------------- |
| BND-060 | Descriptor materialization initializes C defaults, `size` fields, field masks, and nested descriptors.                           |
| BND-061 | Optional field-mask descriptors distinguish absent values from present zero values.                                              |
| BND-062 | Closed enum inputs convert through explicit raw mapping.                                                                         |
| BND-063 | Unknown output enum values preserve the raw native value, using an internal conversion hook when no real C call can produce one. |
| BND-064 | Borrowed native strings and string views are copied before their native borrow window ends.                                      |
| BND-065 | JSON values round-trip supported scalar, array, object, null, integer, floating, boolean, and string data.                       |
| BND-066 | GeoJSON values copy nested geometry, feature, feature collection, properties, and identifiers.                                   |
| BND-067 | Native snapshot/list/result handles are released on success and on copy failure.                                                 |
| BND-068 | Structured JSON preserves object member order, repeated member names, and signed or unsigned integer width.                      |

### Runtime and events

| ID      | Test                                                                                                                                                        |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-080 | `run_once` drives native event processing through the public runtime API and event polling reports an empty queue after events drain.                       |
| BND-081 | Map style loading emits the expected copied map event and identifies the correct public map identity.                                                       |
| BND-082 | Event message and payload data remain valid after the next event poll.                                                                                      |
| BND-083 | Unknown event or payload domains preserve raw values and copied bytes when the C API exposes those bytes.                                                   |
| BND-084 | Offline operation completion returns copied result data and leaves failed take-result handles retryable.                                                    |
| BND-085 | Offline region observation emits copied status/error events through the public runtime event model.                                                         |
| BND-086 | Applies when event source identity uses a registry: a map-originated event with no live public map exposes no public map handle or borrowed native pointer. |

### Map, camera, projection, style, and query

| ID      | Test                                                                                                                                |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| BND-100 | Map creation applies public map options, extent, and mode, then releases through the runtime parent relationship.                   |
| BND-101 | Style URL and style JSON loading succeed through public map APIs and emit copied style-loaded events.                               |
| BND-102 | Camera set/get, animated camera commands, and transition cancellation produce the expected native camera state and statuses.        |
| BND-103 | Projection helpers round-trip screen, lat/lng, and projected-meter values through copied public values within documented tolerance. |
| BND-104 | Representative invalid map and projection inputs propagate native invalid-argument diagnostics through the public error shape.      |
| BND-105 | Style source, layer, image, and feature-state workflows add, update, query/list, and remove public descriptors and copied IDs.      |
| BND-106 | Query workflows return copied feature geometry, properties, feature state, source/layer identifiers, and unknown IDs.               |

### Logging and callbacks

| ID      | Test                                                                                                                                                        |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-120 | Log callback install invokes the registered callback, clear prevents later invocation, and replacement invokes only the replacement callback.               |
| BND-121 | Callback exceptions, panics, or thrown errors are caught and converted to documented C behavior.                                                            |
| BND-122 | Each exposed callback family preserves the previous callback and releases replacement state when replacement fails.                                         |
| BND-123 | Applies when callbacks can arrive concurrently: callback state remains synchronized under concurrency.                                                      |
| BND-124 | Applies when callbacks use bridge roots, global references, or delegates: clear, replacement, and owner release release the root and prevent later upcalls. |
| BND-125 | Custom geometry or style-scoped callback teardown handles style reload, source removal, source ID reuse, map close, and in-flight upcalls without late use. |

### Resources

| ID      | Test                                                                                                                                                     |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-140 | Resource transform can rewrite a URL and can be cleared after registration.                                                                              |
| BND-141 | Resource transform request data is copied into language-owned values before user code receives it.                                                       |
| BND-142 | Resource provider pass-through delegates to native loading without retaining a request handle.                                                           |
| BND-143 | Resource provider handled request can complete inline and load a style.                                                                                  |
| BND-144 | Resource provider handled request can complete later and load a style.                                                                                   |
| BND-145 | Applies when the C API allows it: handled request can complete from another thread.                                                                      |
| BND-146 | Completing a handled request twice reports the binding's already-completed error before crossing into C.                                                 |
| BND-147 | Releasing a handled request makes later completion and cancellation checks fail as closed.                                                               |
| BND-148 | Request cancellation is observable before a late completion, and late completion maps native status.                                                     |
| BND-149 | Resource error responses become copied runtime loading-failure or offline-error events.                                                                  |
| BND-150 | Inline completion during the provider callback finalizes handled ownership even when the callback's later return path would otherwise pass through.      |
| BND-151 | Applies when request handles use registries or integer identities: stale copied handles cannot complete, cancel, or release a later reused request slot. |
| BND-152 | Completion that reaches C is terminal even when native completion returns a non-OK status.                                                               |
| BND-153 | Releasing a request waits for in-flight completion or cancellation checks before native release.                                                         |

### Rendering

| ID      | Test                                                                                                                                                         |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| BND-160 | Supported render-backend queries agree with the loaded native library and gate unsupported render workflows.                                                 |
| BND-161 | Render-target descriptors materialize extents and `NativePointer` backend handles without taking ownership.                                                  |
| BND-162 | Attaching a render target returns one `RenderSessionHandle` and keeps the map parent valid.                                                                  |
| BND-163 | Attaching a second render session to the same map reports invalid state.                                                                                     |
| BND-164 | Unsupported backend or render-target mode reports unsupported status with diagnostics.                                                                       |
| BND-165 | `render_update` maps invalid state without closing the session.                                                                                              |
| BND-166 | Resize updates extent through the public render session API.                                                                                                 |
| BND-167 | CPU readback copies metadata; undersized buffers fail without losing ownership, and sufficiently sized reusable buffers receive image bytes.                 |
| BND-168 | Owned texture frame acquire returns an explicit frame handle with copied metadata and active-checked backend handles.                                        |
| BND-169 | Owned texture frame access after release fails before exposing backend handles.                                                                              |
| BND-170 | Failed frame release leaves the frame live and a later successful release closes it.                                                                         |
| BND-171 | Nested frame acquisition and every exposed session operation forbidden during an active frame fail while a frame is active.                                  |
| BND-172 | Borrowed texture descriptors do not release or mutate caller-owned backend handles during session close.                                                     |
| BND-173 | Wrapper construction failure after native frame acquisition releases the native frame.                                                                       |
| BND-174 | Applies when frame handles are copyable or registry-backed: stale frame copies cannot expose backend handles after release or reuse.                         |
| BND-175 | Applies when reusable native buffers are exposed: reads are copied or read-only, exclusive mutable access is enforced, and use after release fails before C. |

### Threading

| ID      | Test                                                                                                                                         |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-190 | Owner-thread-affine call from a different native thread reports the binding's wrong-thread error.                                            |
| BND-191 | The wrong-thread error includes the copied native diagnostic.                                                                                |
| BND-192 | Ordinary public calls do not internally dispatch to another thread or scheduler.                                                             |
| BND-193 | Applies when an owner-thread execution adapter ships in the subproject: adapter confines create, pump, event drain, and close to one thread. |
| BND-194 | Applies when concurrency markers exist: owner-thread handles are not freely transferable across threads.                                     |
| BND-195 | Applies when unchecked or unsafe concurrency conformance exists: tests cover the documented synchronization invariant.                       |

### Test quality

Requirements:

- Tests MUST exercise public binding APIs except raw-layer bindability,
  generated-layout, capability-gate, and internal lifetime-guard tests.
- Tests MUST use real C calls for public behavior that crosses the binding/C
  boundary.
- Tests use internal fake destroy callbacks only for failure paths that are hard
  to force through the public native library, such as destroy retry.
- Tests MUST cover every supported public domain with high-value native
  workflows and every binding-owned safety invariant. They MUST avoid trivial
  constant assertions and mechanically exhaustive invalid-input matrices unless
  the binding owns conversion, validation, lifetime, or error behavior for that
  case.
- Test skips MUST state the inapplicable domain, C API capability, host-language
  constraint, or unsupported target platform and link to the tracking issue or
  task when the inapplicability is temporary.
