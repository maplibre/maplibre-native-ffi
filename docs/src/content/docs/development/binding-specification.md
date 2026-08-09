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

## Mise Tasks

Every binding exposes the same task contract in its `mise.toml`, so a
contributor moves between bindings without relearning commands.

Requirements:

- A binding MUST define `build`, `test`, and `api` tasks. `build` compiles the
  binding against a native install prefix, `test` runs the binding's suite, and
  `api` generates the HTML reference that `//docs:api` collects.
- `build` and `test` MUST take an optional `[preset]` argument that defaults to
  `{{vars.host_native_preset}}`, and MUST depend on `//:build` for that preset.
  Both come from the root `ffi:preset` task template (`extends = "ffi:preset"`)
  rather than from a restated copy.
- The preset selects the platform, so `test` MUST select its runner from the
  preset rather than exposing a task per platform. Host presets run the suite in
  process, runnable Android and OpenHarmony presets cross-compile and push to an
  emulator through the shared runners in `scripts/`
  (`run-android-emulator-test.sh`, `run-ohos-emulator-test.sh`, which boot the
  emulator on demand), iOS simulator presets build a test bundle and spawn it on
  a simulator, and Emscripten presets run in headless Chromium.
- A preset that a binding cannot build or run MUST fail with a message that
  names what the binding supports. A device preset with no runner, such as
  `ios-arm64-metal`, fails the same way and points at a simulator preset.
- Colon-suffixed tasks cover the axes that a preset does not encode: one
  `test:<runtime>` task per runtime when a platform maps to more than one
  (`test:jvm` and `test:native` for Kotlin; a JavaScript binding adds its
  engines, such as `test:node` and `test:browser`), and acquisition paths such
  as `test:download`. The plain `test` task runs every runtime relevant to the
  preset, so it stays the one command that tests everything, and each runtime
  task applies the same preset dispatch on its own.
- Cross-compilation environment that more than one task needs MUST come from a
  shared script, as `scripts/rust-cross-env.sh` and `scripts/go-cross-env.sh`
  provide, rather than from a copy in each task.
- Task bodies stay small. Logic beyond a few commands belongs in a script under
  `scripts/` or a file task under `.mise/tasks/`, where it reads as a program
  rather than as a TOML string.

---

## Native Artifact Acquisition

A binding gets its C declarations from the checkout and its native library from
somewhere else. Bindings whose ecosystem runs binding-supplied code during a
consumer's build MAY acquire that library themselves from the published snapshot
release. Bindings whose ecosystem does not MUST document the manual install
instead; Go has no such hook by design, and SwiftPM sandboxes plugins away from
the network.

Rust implements this in `bindings/rust/crates/maplibre-native-ffi-sys/build.rs`
and Dart in `bindings/dart/hook/build.dart`.

Requirements for a binding that acquires the library:

- The binding's local-development pointer at an install prefix MUST take
  precedence and MUST skip all network access when set. One mechanism serves
  local development and consumer opt-out; a binding MUST NOT add a second
  discovery mechanism beside it. Rust reads `MAPLIBRE_NATIVE_C_INSTALL_DIR`;
  Dart reads a file, because build hooks run in a semi-hermetic environment that
  strips arbitrary environment variables.
- The target platform and the selected render backend together MUST resolve to
  exactly one published preset. A target with no published artifact MUST fail
  with a message naming the local-development pointer. MapLibre Native compiles
  one renderer per build, so the backend selector MUST reject a request for more
  than one backend rather than choosing between them.
- The backend selector MUST use the target language's own configuration idiom,
  and MUST default to the same backend `host_native_preset` selects for that
  platform.
- `SHA256SUMS` MUST be fetched before any archive, and its digest MUST be the
  cache key. Snapshot asset URLs are stable while their contents are not, so a
  cache keyed on the URL serves stale bytes indefinitely.
- A downloaded archive MUST be verified against its `SHA256SUMS` entry before it
  is installed into the cache, and MUST be extracted somewhere else and moved
  into place afterward, so that a failed download leaves no usable cache entry
  and concurrent builds cannot observe a partial one.
- An unreachable release MUST reuse a cached prefix when one exists, warning
  that it may be out of date, and MUST otherwise fail naming the
  local-development pointer. Offline builds stay possible either way.
- A checksum mismatch MAY be retried once against a freshly fetched
  `SHA256SUMS`, because a publish replaces the checksum file and the archives
  separately and a build spanning that moment can pair one generation with the
  other. A binding that retries MUST stop reusing a cached prefix for the rest
  of that acquisition — neither as an offline fallback nor as a cache hit on the
  retried digest. Once bytes have failed verification, answering from disk hides
  an unverified download behind an older artifact.
- The extracted prefix's descriptor MUST be checked against the requested
  preset. A binding whose local-development pointer is written by tooling rather
  than named by the consumer MUST check that prefix against the build's target
  too, because the pointer then names whichever preset was built last. Dart's is
  written by a mise task and checked; Rust's environment variable is named by
  whoever set it and taken as given.
- The binding MUST warn when the checkout's public headers differ from the
  artifact's. The snapshot release publishes on its own schedule and its commit
  lags the checkout by design, so the commits themselves are not the signal; the
  headers are. `artifact.json` records `gitSha` for the warning to cite, and a
  binding MUST tolerate its absence.
- A binding that links the native library at build time SHOULD link the complete
  static archive rather than the shared library, so its artifact carries no
  runtime search path and needs no repackaging step. The archive merges in every
  dependency it can; what stays external is the platform and render backend's
  system libraries, which `artifact.json` records as `staticLinkLibraries` and
  `staticLinkFrameworks`. A binding MUST tolerate their absence. Bindings that
  load the library at run time, such as Kotlin and Dart, keep using the shared
  library.

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

- the private native handle id;
- live/releasing/closed state;
- the native destroy function or bridge release path;
- parent references or lifetime evidence required for native validity;
- callback state owned by that handle's native scope;
- leak-reporting context when the binding has non-deterministic cleanup hooks.

The native handle id stays private. A binding publishes native identity only as
a copied identity value that supports equality and hashing and carries no
operations. Public code MUST NOT be able to obtain an operable handle from an
identity value, a raw integer, a public field, or an ordinary constructor.

Public release follows this operation:

1. If the wrapper is already closed, return success without calling native code.
2. If another release is in progress, wait for it or return the binding's
   in-progress error before calling native code.
3. Mark the wrapper as releasing so public methods fail before calling native
   code.
4. Keep owner-scoped support state live, including parent references, callback
   state, and request registries.
5. Wait for uses that already passed their liveness check to return.
6. Invoke the matching native release path.
7. If native release succeeds, mark the wrapper closed and make later release
   calls no-op.
8. If native release fails, restore the live state and return the native error
   with diagnostics. Consuming or move-based release APIs return the live owner
   state so callers can retry.

Step 5 is what orders a use against a release for handles that have no
owner-thread rule to do it. A handle whose release is confined to one thread
satisfies it with no mechanism, because that thread cannot also be inside a use.
A handle the host may use and release from different threads holds release off
for the duration of a use, so a release that begins mid-call reports the
binding's own closed-handle error rather than surfacing the C API's rejection of
an id retired underneath the call. The mechanism belongs to the shared handle
state, so every handle of that kind gets the same ordering.

Deterministic cleanup hooks follow the same release operation when they can
report release failure through the target language's normal error path.
Non-deterministic cleanup hooks report leaks for thread-affine handles. They
MUST NOT destroy runtime, map, projection, or render-session handles from
cleanup hooks. Infallible language destructors that attempt best-effort release
MUST preserve the explicit release contract and MUST NOT mask native errors from
the explicit release path.

### Stale and mismatched handles

The C API validates every handle id it receives. It reports
`MLN_STATUS_INVALID_ARGUMENT` with a distinguishing diagnostic for an id that
names a released handle, an id whose handle type does not match the operation,
and a value it never issued.

Bindings surface that status through their ordinary invalid-argument error with
the native diagnostic attached, and keep binding-owned close-once state so a
released wrapper reports its own closed-handle error before crossing into C.
Bindings rely on the C API for id validity, generation, and handle-type
checking.

A binding whose public handle values can be copied or moved between threads or
host isolates documents that a copy of a released handle reports invalid
argument rather than reaching a later native handle.

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

### Value semantics

Public option types, copied result values, and values wrapping copied byte
buffers compare by field value using the target language's ordinary equality
operation. Option types additionally expose a copy operation that produces an
independent instance. Callers diff successive snapshots and derive modified
values without reaching for reference identity.

Equality covers every semantic field the value carries, including private copied
storage, list-valued fields, and nested value trees, and distinguishes an absent
optional field from a present empty or zero value. Languages whose default
equality compares collection, array, or pointer members by identity supply the
comparison that inspects the contents instead, at every level of nesting.

Values that hold callbacks, delegates, or native handles keep identity
semantics, because behavior rather than field values determines whether two such
values are interchangeable.

Where the target language hashes values, the hash covers the same fields as
equality. Values that stay mutable document that an instance in a hash-based
collection stays unmodified while it is a key.

### Enums and masks

Public enum values expose named cases for known C values and keep the
represented raw value available for conversion to C.

When C returns an enum value that the binding does not know yet, the binding
preserves the raw value instead of collapsing it to a known case. Public input
paths pass represented raw enum values through to C unless the binding owns an
additional state, lifetime, or type-safety invariant that must fail before C.
Bindings do not duplicate enum validation performed by the C API.

Public bit masks use a named public type that supports combining, testing, and
empty values. C field masks stay internal to C struct materializers.

### Strings

Public string inputs use UTF-8 at the C boundary. Null-terminated `const char*`
inputs reject embedded `NUL`. The C ABI represents other borrowed strings with
`mln_buffer_view`; each parameter documents its text encoding and accepted
contents. Borrowed strings are copied before their native borrow window ends.

### JSON and GeoJSON

Bindings expose arbitrary JSON and GeoJSON transit as language-owned byte arrays
containing UTF-8. They MUST NOT expose the removed recursive JSON, geometry,
feature, or GeoJSON descriptor models. A document API accepts one complete
document, a property API accepts one JSON value, and a geometry API accepts one
GeoJSON Geometry.

Input bytes stay alive through the C call and may be released or mutated after
it returns. Output buffers are copied once into a new language-owned byte array
and destroyed on success and failure. Bindings do not parse or reformat these
payloads. Loaded style documents therefore round-trip byte-for-byte; values
serialized from native state have no stable formatting or member-order contract.

The low-level API exposes no parallel string overload. Callers that start with
text encode it as UTF-8. Callers that start with a file, response, database
blob, or serializer output can pass its bytes without an intermediate host
string.

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

Native buffer, snapshot, result, and list handles are internal implementation
details. Plain value outputs with no interior borrowed pointers are copied by
value.

Outputs backed by native storage follow this operation:

1. Acquire the native buffer, snapshot, result, list, or event.
2. Copy public data into language-owned values before the native borrow window
   ends.
3. Preserve unknown event and payload domains as raw values with copied payload
   bytes when the C API exposes those bytes.
4. Release native buffer, snapshot, result, and list handles exactly once after
   copying, including failure paths.

Runtime event polling returns values independent of the next native poll.
Map-originated events identify a live source map when identity can be proven. If
lookup misses, they carry no public map handle or only copied source metadata.

### Style source metadata

Style source inspection returns one copied, language-owned source information
value. It combines the fixed fields from `mln_style_source_info` with copied
attribution, source URL, and inline TileJSON tile URLs. The native string-list
handle is an internal copy mechanism and MUST NOT appear in the public API.

The public value represents these concepts:

- source type, volatility, and optional attribution;
- an optional retained source URL;
- optional inline TileJSON containing the complete copied tile URL list, minimum
  and maximum zoom, scheme, and optional bounds;
- optional tile size, vector encoding, and DEM raster encoding.

A binding MAY expose the inline TileJSON fields as a nested value or flatten
them into its source information value. Either shape MUST preserve the same
presence boundary: no inline TileJSON is different from inline TileJSON with an
empty tile list or zero-valued fields. Optional tile size, bounds, and encodings
likewise distinguish absence from a present default or zero value.

The value describes the source's reconstructible constructor state. A URL-backed
tile source exposes its source URL and leaves inline TileJSON absent, including
after its remote description loads. An inline tile source exposes its retained
TileJSON fields and has no source URL. GeoJSON and image sources expose a
retained URL when one is present. The value does not expose the resolved
TileJSON of a URL-backed source.

Bindings copy every string before returning and destroy the native tile URL list
on success and on copy failure. They preserve unknown source, scheme, vector
encoding, and raster encoding values through the binding's ordinary unknown-enum
representation. A returned value remains valid after source removal, style
replacement, and map release.

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

### HTTP header transforms

Direct-callback bindings copy the resource kind and transformed URL into a
language-owned request and accept a language-owned collection of header names
and values. They reject duplicate names case-insensitively and contain every
host exception, panic, or callback error so that a failed invocation returns no
transformed headers. Header field-name, field-value, and transport-managed-name
validation produces the C API's invalid-argument behavior without exposing a
header value in diagnostics. Header values are valid UTF-8 and contain only the
horizontal tab and non-control characters accepted by the C API.

The callback runs after resource URL transformation and before an HTTP attempt
is dispatched. The URL presented to the callback is the transformed URL.
Cache-only loads, non-HTTP resources, and requests handled by a resource
provider produce no header-transform invocation.

Registration remains replaceable for the lifetime of a runtime, including while
maps are live. A binding installs a replacement before releasing the previous
callback state. Replacement, clear, and runtime close release the old state only
after the native registration call has retired every in-flight callback.

Bindings that cannot answer a synchronous callback expose native rule tables. An
exact rule matches the complete transformed URL, a glob rule matches it against
the pattern language the C API reference defines, and the first matching rule
supplies its complete header collection. Unknown flags, null match operands, and
requests outside the matched URL family pass through without transformed
headers.

Every transformed header is redirect-sensitive. A same-origin redirect, with the
same scheme, host, and effective port, preserves transformed headers. A redirect
that changes any origin component removes every transformed header before
dispatch and does not invoke a destination transform. Native range and
conditional headers retain their platform transport behavior independently of
the transformed collection.

#### OpenHarmony and the browser

HTTP header transform registration reports unsupported on OpenHarmony, whose
platform HTTP client lacks a redirect-decision hook, and in the browser, whose
fetch transport follows redirects itself. This keeps transformed credentials out
of cross-origin redirects rather than enabling a transport that cannot satisfy
the redirect contract. A resource provider serves those requests instead.

### Resource providers

Resource providers decide whether a request passes through to the native
provider or is handled by the binding.

A request carries two URLs, and a binding exposes both under names that keep
them distinguishable. The requested URL preserves configured URI-scheme aliases
and custom schemes, and is the request's logical, cache-facing identity. The
resolved URL is what a provider fetches, and equals the requested URL when no
configured alias applies.

Resource provider invocation follows this operation:

1. For pass-through requests, return pass-through without retaining the native
   request handle.
2. For handled requests, copy request fields before user code can retain them.
3. Retain the native request reference until completion, cancellation handling,
   or release.
4. Treat inline completion during the provider callback as handled ownership,
   even if the callback return path would otherwise pass through.
5. Defer inline request release until the callback returns handled ownership.
6. Allow deferred or cross-thread completion when the C API allows it, without
   changing one-shot or release behavior.

Provider registration is replaceable for a runtime's whole life. A binding keeps
the registered callback state reachable until the C call that replaces or clears
the provider returns, and releases it after that call returns.

Handled request completion is terminal. A request can complete once; a
completion that reaches C consumes the completion path even when native returns
non-OK. Release runs once, waits for in-flight completion or cancellation
checks, and makes later completion or cancellation checks fail before crossing
into C. Stale public request handles cannot affect later native requests.

A binding whose host runtime moves a handled request between execution contexts
passes the request handle id itself, and exposes the C API's wait-until-retired
operation for teardown. A released or completed request id reports invalid
argument, so one request handle type covers both the owning and the moved use.

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

A helper that parks its owner thread between iterations acquires a wake source
and signals it from submission and close, so a submitted operation runs at
submission time.

### Parking and wake

The pump is one method taking a timeout. Bindings expose it alongside a wake
source handle.

The pump wrapper follows this design:

1. It takes the host language's duration or timeout type, maps zero to a
   non-blocking drain, and maps the language's "no timeout" spelling to an
   unbounded park.
2. It releases the host runtime's blocking-call machinery for the duration of
   the call, including any interpreter lock, so other host threads run while the
   owner thread parks.
3. Its documentation states that a wake signal sets a flag the pump clears, and
   that callers drain events after every return.

The wake source follows this design:

1. It is a distinct owned handle that the host releases explicitly, and
   releasing it is independent of the runtime's lifetime in both orders.
2. It is transferable and callable from any thread, and the binding declares
   that where the host language can express it.
3. Signalling after the runtime is closed succeeds and does nothing.

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
6. Copy the event's source id, resolve any public map wrapper for that id
   through binding-owned runtime state, and expose the copied id as the event's
   source identity. Constructing a public handle from the source id stays
   outside the safe public API.
7. Apply binding-owned state updates triggered by the event before returning the
   copied event.

### Attaching a render session

A render session's owner thread is the thread that attached it, fixed for the
session's lifetime, and it need not be the map's owner thread.

1. Attach requires the map to be live, not to be owned by the calling thread.
2. The session a binding returns is affine to the attaching thread. Every
   session operation from another thread reports the binding's wrong-thread
   error, including close.
3. A binding MUST NOT retain the map in binding-owned state that cannot reach
   the attaching thread. Where a binding drops that retention, it documents that
   the C API keeps the map alive instead, by rejecting map destroy while a
   session is attached, and that releasing a map before its session reports
   through the binding's leak channel rather than destroying the map.
4. A binding whose map handle cannot cross threads MUST expose the map to the
   attaching thread through a transferable attach reference whose only operation
   is attach. Bindings whose map handle is already safe to use from another
   thread expose attach on the map handle directly and MUST NOT add a redundant
   reference type. A binding whose owner identity is a host construct rather
   than the native thread, such as a Dart isolate, states that the two must
   coincide for its handles to stay usable.

### Transferability

When the language can declare or enforce cross-thread transferability, ordinary
owner-thread handles MUST be non-transferable. A transferable owner-thread
helper handle is allowed only when every operation either is submitted back to
the bound native owner thread or is serviced entirely under native
synchronization. Two handles are the second kind. A map attach reference reaches
no thread-affine map state: attach claims the map's render-session slot under
the C API's map registry lock and posts the new size to the map's own owner
thread. A wake source handle reaches native wake state that carries its own
synchronization and holds no owner-thread state. Both are transferable and MUST
NOT be shareable. Copied immutable values can be transferable when their
contents are independent of native owner-thread state. Unchecked or unsafe
concurrency conformance MUST name the synchronization invariant that makes it
sound.

---

## Rendering

Rendering bindings expose render sessions, frame lifetimes, and readback without
taking ownership of caller-owned backend resources.

Render session calls may run on a thread other than the one that pumps the
runtime. Events produced by rendering are still delivered by runtime event
polling on the runtime owner thread.

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
3. Call the matching C attach function on the thread that will drive the
   session, which for a host graphics API with a thread-current context is the
   thread where that context is current.
4. Return a distinct `RenderSessionHandle`, bound to the calling thread, for the
   map's one live render session.
5. Surface unsupported backend, unsupported render-target mode,
   existing-session, and native errors through the binding's status mapping.

For host-owned backend resources, the binding does not release or synchronize
those resources. The caller keeps them valid for the C API's documented borrow
window.

The public handle exposes:

- `resize` for session kinds that support resize;
- `set_target` for session kinds whose target the host owns, which is surface
  sessions and caller-owned texture sessions;
- `render_update` for the latest available map render update, reporting whether
  an update was rendered;
- `detach`, which keeps the public handle live after backend resources detach;
- `close` or `destroy`, using the owned-handle release operation, on the thread
  that attached the session.

`set_target` takes the same public descriptor type its attach function takes, so
a host builds a replacement target the way it built the first one. It is exposed
per backend and target kind, matching the C API's `mln_*_surface_set_target()`
and `mln_*_borrowed_texture_set_target()` functions, and it is bound to the
thread that attached the session like every other session operation. The C API
rejects a descriptor whose graphics context differs from the session's, so
bindings pass the descriptor through.

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

| ID      | Test                                                                                                                                                                                                      |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-040 | Runtime creation followed by explicit release destroys the native handle exactly once; every public alias observes release state, and a second release no-ops.                                            |
| BND-041 | A failed native destroy leaves the handle live; a later successful release destroys the native handle.                                                                                                    |
| BND-042 | A child handle retains parent owner state, and parent release fails while child handles are live.                                                                                                         |
| BND-043 | `MapProjectionHandle` remains usable after the source map closes and then releases successfully.                                                                                                          |
| BND-045 | A released handle's id, replayed through an internal seam after a new handle of the same kind is created, reports the binding's invalid-argument error naming it stale, and the new handle keeps working. |
| BND-047 | A handle id of one kind passed to another kind's operation through an internal seam reports the binding's invalid-argument error, and the safe public API has no expression of that call.                 |
| BND-049 | A handle id moved to a different native thread and called there reports the binding's wrong-thread error rather than a stale-handle or closed-handle error.                                               |

BND-049 applies where the host language can reach a second native thread while
the handle stays live. Dart is excluded: an isolate may resume on a different
native thread after an await, so a handle must be closed before the test awaits
another isolate, which leaves its id stale rather than live.

BND-049 applies where the host language can reach a second native thread while
the handle stays live. Dart is excluded: an isolate may resume on a different
native thread after an await, so a handle must be closed before the test awaits
the other isolate, which makes its id stale rather than live.

### Input Structs, Values, and Copied Data

| ID      | Test                                                                                                                                                                                                       |
| ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-060 | Each public API family that accepts input structs has at least one test that initializes C defaults, `size` fields, field masks, and nested inputs.                                                        |
| BND-061 | Optional field-mask inputs distinguish absent values from present zero values.                                                                                                                             |
| BND-062 | Unknown output enum values preserve the raw native value, using an internal conversion hook when no real C call can produce one.                                                                           |
| BND-063 | Borrowed native strings and buffer views are copied before their native borrow window ends.                                                                                                                |
| BND-064 | JSON byte inputs accept scalar and nested values and reach native parsing and validation without a binding-owned parse.                                                                                    |
| BND-065 | GeoJSON byte inputs cover geometry, feature, and feature-collection shapes, including nested geometry and properties.                                                                                      |
| BND-066 | Native buffer, snapshot, list, and result handles, including style tile-URL lists, are released on success and on copy failure, using fault injection for copy failure.                                    |
| BND-067 | Byte transit preserves the complete input length, including non-null-terminated storage, and loaded style documents round-trip byte-for-byte.                                                              |
| BND-068 | Unknown enum values preserve their raw value, and public input APIs report the C API's status and diagnostic unless the binding owns a stricter pre-C invariant.                                           |
| BND-069 | Public values and descriptors that accept caller-owned mutable storage remain unchanged after later caller mutation, and accessors do not expose mutable storage that can mutate the stored value.         |
| BND-070 | Option types compare and hash by field value, separate absent optional fields from present empty or zero values, and copy to an independent instance; one case per option type mutates each field in turn. |
| BND-071 | Copied result values and values wrapping copied buffers compare by content when built from distinct list or array instances holding equal contents.                                                        |

### Runtime and events

| ID      | Test                                                                                                                                                                |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-080 | `pump` drives native event processing through the public runtime API, and repeated event polling reaches an empty queue.                                            |
| BND-081 | Map style loading returns the expected copied map event through polling and identifies the correct public map identity.                                             |
| BND-082 | Event message and payload data remain valid after the next event poll.                                                                                              |
| BND-083 | Unknown event or payload domains preserve raw values and copied bytes when the C API exposes those bytes.                                                           |
| BND-084 | Offline operation completion returns copied result data. Native take-result status failures before result ownership transfers leave the operation handle retryable. |
| BND-085 | Offline region observation returns copied status/error events through the public runtime event model.                                                               |
| BND-086 | A map-originated event with no provable live public map exposes no public map handle.                                                                               |
| BND-087 | Known typed event payloads validate native payload size before reading payload fields.                                                                              |
| BND-088 | A parked owner thread is released by native work and by a wake source signalled from another thread, and reports a wake rather than a timeout.                      |
| BND-089 | A pump clears the wake flag it returned on, and a wake source stays signalable and releasable after its runtime closes.                                             |

### Map, camera, projection, style, and query

| ID      | Test                                                                                                                                                                                                    |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-100 | Map creation applies public map options, extent, and mode, then releases through the runtime parent relationship.                                                                                       |
| BND-101 | Style URL and style JSON loading succeed through public map APIs and return copied style-loaded events through polling.                                                                                 |
| BND-102 | Camera set/get, animated camera commands, transition cancellation, and gesture-in-progress bracketing produce the expected native camera state and statuses.                                            |
| BND-103 | Projection helpers round-trip screen, lat/lng, and projected-meter values through copied public values within documented tolerance.                                                                     |
| BND-104 | Representative invalid map and projection inputs propagate native invalid-argument diagnostics through the public error shape.                                                                          |
| BND-105 | Style source, layer, image, and feature-state workflows add, update, query/list, and remove public input values and copied IDs.                                                                         |
| BND-106 | Query workflows return one copied UTF-8 JSON envelope containing feature geometry, properties, identifiers, state, and optional source/layer identifiers.                                               |
| BND-108 | The loaded style document reads back byte-for-byte through public map APIs, the style URL reads back the last requested URL, and both report empty when absent.                                         |
| BND-109 | Source inspection copies a URL-backed source URL and inline tile-source metadata, including multiple tile URLs and absent fields, and the result remains valid after the map no longer owns the source. |

### Logging and callbacks

| ID      | Test                                                                                                                                                        |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-120 | Log callback install invokes the registered callback, clear prevents later invocation, and replacement invokes only the replacement callback.               |
| BND-121 | Host-language failures do not unwind or escape across the C callback boundary, and recoverable callback failures are converted to documented C behavior.    |
| BND-122 | Each exposed callback family preserves the previous callback and releases replacement state when replacement fails.                                         |
| BND-123 | Callback state remains synchronized for callback families whose C contract allows concurrent invocation.                                                    |
| BND-124 | Custom geometry or style-scoped callback teardown handles style reload, source removal, source ID reuse, map close, and in-flight upcalls without late use. |

### Resources

| ID      | Test                                                                                                                                                                                                  |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-140 | Resource transform can rewrite a URL and can be cleared after registration.                                                                                                                           |
| BND-141 | Resource transform request data is copied into language-owned values before user code receives it.                                                                                                    |
| BND-142 | Resource provider pass-through delegates to native loading without retaining a request handle.                                                                                                        |
| BND-143 | Resource provider handled request can complete inline and load a style.                                                                                                                               |
| BND-144 | Resource provider handled request can complete later and load a style.                                                                                                                                |
| BND-145 | Handled request can complete from another thread.                                                                                                                                                     |
| BND-146 | Completing a handled request twice reports the binding's already-completed error before crossing into C.                                                                                              |
| BND-147 | Releasing a handled request makes later completion and cancellation checks fail as closed.                                                                                                            |
| BND-148 | Request cancellation is observable before a late completion, and late completion maps native status.                                                                                                  |
| BND-149 | Resource error responses become copied runtime loading-failure or offline-error events.                                                                                                               |
| BND-150 | Inline completion during the provider callback finalizes handled ownership even when the callback's later return path would otherwise pass through.                                                   |
| BND-151 | Stale request handles cannot complete, cancel, or release later native requests.                                                                                                                      |
| BND-152 | Completion that reaches C is terminal even when native completion returns a non-OK status.                                                                                                            |
| BND-153 | Releasing a request waits for in-flight completion or cancellation checks before native release.                                                                                                      |
| BND-154 | Resource provider can be replaced while maps are live and can be cleared, and a cleared provider stops receiving requests.                                                                            |
| BND-155 | A request for a configured URI-scheme alias exposes the alias as the requested URL and the tile-server-normalized URL as the resolved URL.                                                            |
| BND-158 | HTTP header transform requests and returned headers cross the callback boundary as copied language-owned values, reject duplicate field names case-insensitively, and contain host-language failures. |
| BND-159 | HTTP header transforms can be installed, replaced, and cleared while maps are live; transformed headers reach matching requests and no request after clear.                                           |

#### Queued provider routes

When the binding routes provider requests through
`mln_adapter_queued_resource_provider`, include:

| ID      | Test                                                                                                                                                                                       |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| BND-156 | A glob route claims every request URL its pattern matches, and a request URL the pattern leaves unmatched passes through to native loading.                                                |
| BND-157 | A route comparing the requested URL claims a request for a configured URI-scheme alias, and a route comparing the resolved URL claims that same request by its tile-server-normalized URL. |

### Rendering

| ID      | Test                                                                                                                                                      |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-160 | Supported render-backend queries gate configured workflows and unsupported backend/mode errors.                                                           |
| BND-161 | Render-target descriptors materialize extents and `NativePointer` backend handles without taking ownership.                                               |
| BND-162 | Surface, session-owned texture, and caller-owned texture attach paths call the matching C session family and report the same public session handle shape. |
| BND-163 | Attaching a second render session to the same map reports invalid state.                                                                                  |
| BND-164 | `render_update` reports no-update-available as a false result without closing the session.                                                                |
| BND-165 | Resize updates extent through the public render session API.                                                                                              |
| BND-175 | `set_target` replaces a host-owned render target through the public render session API and updates the session's extent.                                  |
| BND-176 | `set_target` reports unsupported for a target kind the session does not have, covering a session-owned texture and a mismatched surface/texture pairing.  |
| BND-166 | CPU readback copies metadata; undersized buffers fail without losing ownership, and sufficiently sized reusable buffers receive image bytes.              |
| BND-167 | Owned texture frame acquire returns an explicit frame handle with copied metadata and active-checked backend handles.                                     |
| BND-168 | Owned texture frame access after release fails before exposing backend handles.                                                                           |
| BND-169 | Failed frame release leaves the frame live and a later successful release closes it.                                                                      |
| BND-170 | Nested frame acquisition and every exposed session operation forbidden during an active frame fail while a frame is active.                               |
| BND-171 | Caller-owned texture descriptors do not release or mutate caller-owned backend handles during session close.                                              |
| BND-172 | Bindings with fallible owned-frame wrapper construction release the native frame when construction fails after native frame acquisition.                  |
| BND-173 | Stale frame handles cannot expose backend handles after release or reuse.                                                                                 |
| BND-174 | Closing a map whose render session was attached on another thread reports the C API's invalid-state error and leaves both handles live.                   |

### Conditional tests

The following tests apply only when a binding has the named host-language
mechanic, helper, or test fixture.

#### Host cleanup hooks

When the host language can run cleanup outside explicit release, include:

| ID      | Test                                                                                                                             |
| ------- | -------------------------------------------------------------------------------------------------------------------------------- |
| BND-044 | Non-deterministic cleanup hooks report leaked thread-affine handles rather than destroying them.                                 |
| BND-048 | Best-effort cleanup failure is reported through the binding's documented leak or failure channel and explicit release can retry. |

#### Cross-thread public handle use

When safe public code can call owner-thread-affine APIs from the wrong native
thread or race release on the same owner-thread handle, include:

| ID      | Test                                                                                                                                                          |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-046 | Concurrent releases call native release at most once and public calls fail while release is in progress.                                                      |
| BND-190 | Owner-thread-affine calls from a different native thread report the binding's wrong-thread error.                                                             |
| BND-191 | Runtime wrong-thread errors include the copied native diagnostic.                                                                                             |
| BND-197 | A release racing a use of the same handle waits for the in-flight use, and a use starting after the release begins reports the binding's closed-handle error. |

BND-197 applies to handles the host can use and release from different threads,
which today means the wake source and the resource request. A binding that
orders the two by holding one lock across the native call satisfies it by
construction and has nothing beyond that lock to assert; a binding that counts
in-flight uses and drains them exercises the counter directly.

#### Render sessions on a second thread

When the binding's test suite attaches a render session on a configured render
backend and the host language can start a native thread, include:

| ID      | Test                                                                                                                                                  |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-193 | A native thread that does not own the map attaches its own render session against it and renders while the map is pumped on its own owner thread.     |
| BND-194 | Every render-session operation reports the binding's wrong-thread error on a thread other than the one that attached the session, leaving it usable.  |
| BND-195 | A session attached and closed on a second native thread destroys the native handle exactly once, after which the map closes successfully.             |
| BND-196 | Attaching through a reference to a released map reports the binding's invalid-argument error naming the map stale, including once a later map exists. |

#### Live render session queries

When the binding's test suite attaches a render session on a configured render
backend, include:

| ID      | Test                                                                                                                                                                                    |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BND-107 | A queried cluster feature passed back to a feature-extension query resolves its unsigned `cluster_id`, and unsigned `limit` and `offset` arguments bound and shift the returned leaves. |

Bindings without live render-session fixtures cover byte ownership and copying
through BND-066 and BND-067.

#### Owner-thread execution adapters

When the subproject ships an owner-thread execution adapter, include:

| ID      | Test                                                                       |
| ------- | -------------------------------------------------------------------------- |
| BND-192 | The adapter confines create, pump, event polling, and close to one thread. |
