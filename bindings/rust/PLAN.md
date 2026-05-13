# Rust binding architecture plan

## Audience and purpose

This plan is for maintainers and contributors who shape the Rust binding and the
native-extension bridge bindings built from it. It describes the ideal end state
for the Rust crates after this branch lands. Backward compatibility with the
branch's current Rust API is not a constraint.

Rust bindings unblock Python, Node.js, and Java JNI because PyO3, napi-rs, and
jni-rs can share Rust code that adapts the public C ABI. They should share that
ABI adaptation without inheriting Rust's public handle, lifetime, callback, or
finalizer model.

## Design conclusion

The bridge bindings should build on a shared internal Rust adaptation layer, not
on the public `maplibre-native` crate and not directly on `maplibre-native-sys`
except inside small host-runtime trampolines.

The public Rust crate is safe because it encodes Rust-specific guarantees:
`!Send` thread-affine handles, `Rc` parent retention, `Drop` on the owner
thread, Rust lifetimes for texture frames, and closure traits for callbacks.
Python, Node.js, and Java JNI need different public safety models: garbage
collection, runtime-specific finalizers, environment ownership, exception
translation, callback queues, global references, and thread attachment. Reusing
the Rust public crate would force those bridges to work around Rust's safety
model.

The shared layer should instead own facts about the C ABI: status codes,
diagnostics, copied values, descriptor materialization, native result copying,
callback request copying, and exactly-once native-resource release. Each public
binding should own its language policy above that layer.

## Ideal crate layout

```text
maplibre-native-sys
  Generated raw declarations for the public C ABI.
  Unsafe extern functions, constants, C layouts, and opaque handle types.

maplibre-native-core
  Shared internal ABI adaptation.
  Safe or narrowly unsafe Rust building blocks that are independent of any
  public language binding's object model.

maplibre-native
  Public safe Rust binding.
  Rust-owned handles, Rust lifetimes, Rust errors, Rust callbacks, and re-exports
  of shared copied values where that improves the public Rust API.

maplibre-native-python
  PyO3 extension crate.
  Python classes, exceptions, buffers, queues, GIL/free-threaded synchronization,
  context managers, and leak-reporting finalizers.

maplibre-native-node
  napi-rs add-on crate.
  N-API classes, TypeScript declarations, environment cleanup hooks,
  thread-safe functions, `Symbol.dispose`, and leak-reporting finalizers.

maplibre-native-jni
  jni-rs bridge crate.
  JNI method registration, Java exceptions, global references, JVM thread
  attach/detach, Android packaging hooks, and leak-reporting finalizers.
```

Rename `maplibre-native-support` to `maplibre-native-core`. Keep low-level
helper modules inside that crate instead of preserving a separate support crate.
Bridge crates depend on this shared internal adaptation layer, and the public
Rust crate becomes one consumer of it.

## Layer responsibilities

### `maplibre-native-sys`

`sys` remains generated and raw. It owns no safety policy beyond bindgen layout
correctness.

Keep in `sys`:

- generated `extern "C"` functions;
- generated constants and C structs;
- generated opaque native handle declarations;
- generated layout tests and link/loading glue that belongs at the ABI
  declaration boundary.

### `maplibre-native-core`

`core` owns reusable C ABI adaptation. Its API can expose Rust types and
`unsafe` functions when the caller must prove native pointer validity, but those
unsafe functions should be small, named after the invariant they require, and
covered by tests.

Move or create in `core`:

- ABI version validation;
- status-code mapping and thread-local diagnostic capture;
- native string and string-view helpers;
- out-pointer helpers and null checks;
- short-lived native result/list/snapshot guards;
- pure copied value types and raw conversions;
- descriptor materializers that fill `size`, field masks, string storage, and
  nested native storage;
- native result readers that copy borrowed data and release native handles;
- event payload copying that preserves raw source identity;
- resource request copying, resource response materialization, and exactly-once
  request-handle release;
- common callback trampoline primitives that copy C arguments, catch panics, and
  return documented C callback outcomes;
- bridge-friendly explicit native handle state that does not assume Rust `Drop`
  can destroy thread-affine handles.

`core` should not own public language policy. It should avoid Python, Node.js,
Java, and public Rust concepts such as Python exceptions, N-API environments,
Java global refs, Rust `Rc` parent retention, or Rust frame lifetimes.

### `maplibre-native`

The public Rust crate owns Rust ergonomics and Rust safety policy.

Keep in `maplibre-native`:

- `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`, and `RenderSessionHandle`
  public wrappers;
- `Rc`/`Cell`/`RefCell` owner-thread state for Rust-only `!Send` handles;
- Rust parent retention and close semantics;
- Rust `Drop` behavior for thread-affine handles when safe Rust proves
  owner-thread destruction;
- `HandleOperationError<T>` and other Rust-specific error ergonomics;
- Rust callback closure APIs;
- Rust lifetime-encoded texture frame handles and scoped backend pointers;
- Rust-specific `NativePointer` constructors and `Send`/`Sync` decisions;
- Rust map registry policy, such as mapping native map pointers to Rust `MapId`
  values.

The crate should consume `core` for ABI adaptation and re-export shared copied
values where that keeps the public API natural.

### Bridge crates

Each bridge crate owns its host runtime's public safety model.

Python owns:

- PyO3 classes and exceptions;
- CPython 3.14 and free-threaded synchronization;
- Python buffers and `bytes` conversions;
- context-manager helpers;
- Python callback queues;
- leak reporting through `__del__`, weak finalizers, or GC hooks.

Node.js owns:

- napi-rs classes and TypeScript declarations;
- N-API environment ownership;
- `ThreadsafeFunction` callback routing;
- environment cleanup hooks;
- `Symbol.dispose` and finalizer leak reporting.

Java JNI owns:

- `JNI_OnLoad` and native method registration;
- Java exceptions and status translation;
- `JavaVM`, `JNIEnv`, global references, and local-reference scopes;
- native thread attach/detach;
- Android/JVM loading policy.

The bridge crates should touch `sys` directly only in host-ABI trampoline code
or when a C ABI feature has no `core` abstraction yet. Any repeated direct `sys`
use is a signal to move an adapter into `core`.

## What moves down from the current public Rust crate

### Pure values and enum domains

Move shared copied values and raw enum mapping into `core`. The public Rust
crate can re-export them.

Candidates:

- `LatLng`, `LatLngBounds`, `ProjectedMeters`, `ScreenPoint`, `ScreenBox`,
  `EdgeInsets`, `Vec3`, `Quaternion`, `UnitBezier`;
- camera, map option, render option, resource, logging, event, and offline enum
  raw mappings;
- `NetworkStatus` raw mapping;
- render backend mask raw mapping;
- copied image metadata and texture image info values.

These types represent the shared MapLibre Native FFI model, not Rust handle
policy.

### JSON, geometry, and GeoJSON materialization

Move the owned value trees and their native materializers/readers into `core`:

- `JsonValue`, `JsonMember`, `NativeJsonValue`, `NativeJsonMembers`, and JSON
  snapshot copying;
- `Geometry`, `NativeGeometry`, and geometry copying;
- `GeoJson`, `Feature`, `FeatureIdentifier`, `NativeGeoJson`, `NativeFeature`,
  and feature copying.

Bridge bindings convert from host-native values into these core values, then
rely on one materialization path at the C boundary.

### Descriptor materializers

Move materializers for C option and descriptor structs into `core`:

- camera descriptors;
- map options, viewport options, and tile options;
- runtime options;
- offline region definitions;
- render target descriptors;
- feature-state selectors;
- rendered/source feature query options;
- tile source options;
- style image options;
- resource responses.

These materializers own ABI bookkeeping: `size` fields, field masks, temporary
strings, string views, nested arrays, and pointer lifetimes.

### Native result and snapshot readers

Move readers that copy native result handles into owned values:

- style ID lists;
- offline region snapshots and lists;
- feature query results;
- feature extension results;
- JSON snapshots;
- style image info and copied style image buffers;
- runtime event payloads.

Readers should release native handles through RAII guards even when copying
fails.

### Resource request bridge primitives

Move the common resource-provider machinery into `core`:

- `ResourceRequest` copying from `mln_resource_request`;
- `ResourceResponse` materialization into `mln_resource_response`;
- request cancellation checks;
- exactly-once completion;
- exactly-once release;
- inline-completion decision finalization;
- panic/error-to-C-status helpers.

The public Rust crate can wrap this with a closure API returning
`ResourceProviderDecision`. Python, Node.js, and JNI can wrap it with their own
request objects and callback queues.

### Event copying

Move raw event copying into `core`, but keep wrapper lookup above it.

`core` should produce a copied event with:

- raw event type;
- raw source type;
- raw source address;
- copied message;
- copied payload;
- unknown raw payload bytes.

The public Rust crate can map the raw source address to `MapId`. Python,
Node.js, and JNI can map it to a live wrapper or copied source metadata.

### Logging record copying

Move severity/event raw mapping and `LogRecord` copying into `core`. Keep
callback installation and delivery policy above it.

Rust can call a closure directly. Python can enqueue work for Python. Node can
use a thread-safe function. JNI can attach the thread and call Java or enqueue
into Java-owned state.

### Native handle state

Add a `core` handle primitive that centralizes raw pointer ownership and
close-once state without assuming Rust public handle semantics.

Desired shape:

```text
NativeHandleState<T>
  owns a non-null native pointer while live
  supports explicit close_once(destroy_fn)
  supports status-returning and infallible destroy functions
  reports closed/live state
  can intentionally leak-report instead of destroying from finalizers
  does not use Rc to enforce !Send
  does not dispatch across threads
  does not assume Drop may call thread-affine destroy
```

The public Rust crate should layer `!Send`, parent retention, and owner-thread
`Drop` on top. Bridge crates should layer locks, finalizer leak reporting, and
host exceptions on top.

## What stays above `core`

Keep binding-specific policy out of `core`:

- public Rust handle types and method names;
- public Python, Node.js, and Java classes;
- finalizer behavior for GC runtimes;
- internal dispatch, event-loop routing, or scheduler policy;
- JavaScript `Promise` choices;
- Python GIL or free-threaded synchronization choices;
- JVM thread attach/detach choices;
- language exception hierarchies;
- callback delivery queues;
- public callback object lifetimes;
- wrapper registries that map native pointers to language objects;
- public unsafe backend-pointer APIs.

`core` should make those policies easier to implement, not choose them.

## Milestones

### Milestone 1: Establish the crate boundary

Rename the shared crate and make the intended boundary explicit.

Deliverables:

- Rename `maplibre-native-support` to `maplibre-native-core`.
- Move the current support helpers into modules inside `core`.
- Document the crate roles in each crate root.
- Make `maplibre-native` depend on `core` for all existing support helpers.
- Keep `sys` generated-only.

Acceptance criteria:

- A contributor can tell from crate names and module docs where new ABI
  adaptation code belongs.
- The public Rust crate contains little or no direct raw pointer utility code.

### Milestone 2: Move shared copied values and raw mappings

Extract pure values and enum mappings from the public Rust crate.

Deliverables:

- Move scalar value structs into `core`.
- Move closed enum raw conversions and unknown-value preservation into `core`.
- Re-export public Rust value types from `maplibre-native`.
- Add unit tests in `core` for raw mapping, unknown preservation, and round
  trips.

Acceptance criteria:

- Bridge crates can depend on one shared value vocabulary.
- The public Rust API remains natural through re-exports or thin wrappers.

### Milestone 3: Move JSON, geometry, and GeoJSON adapters

Extract the nested value trees and native storage builders.

Deliverables:

- Move JSON value/member types, materializers, and readers.
- Move geometry value types, materializers, and readers.
- Move GeoJSON feature and feature-collection materializers and readers.
- Preserve depth checks, integer width, object member order, duplicate keys, and
  finite-number validation.

Acceptance criteria:

- Native JSON/geometry/GeoJSON descriptors materialize through one shared code
  path.
- Tests cover copied output survival after backing storage changes.

### Milestone 4: Move descriptor materializers

Extract all per-call C descriptor builders.

Deliverables:

- Move camera, map, viewport, tile, runtime, offline-region, render-target,
  style, and query descriptor materializers.
- Keep public Rust builder methods in `maplibre-native`, but make them call
  `core` materializers.
- Add tests that inspect `size` fields, masks, null pointers, string views,
  nested arrays, and backing storage lifetimes.

Acceptance criteria:

- Python, Node.js, and JNI can build host-language descriptors and then call one
  core materialization path.
- Field-mask and `size` logic appears in one crate.

### Milestone 5: Move native result readers

Extract result-handle copying and release logic.

Deliverables:

- Move snapshot, list, query result, extension result, style ID, JSON snapshot,
  and offline region readers.
- Keep native guards in `core`.
- Make readers release native handles on success and failure.
- Add tests for copy failure paths.

Acceptance criteria:

- Public binding crates receive owned copied results from `core`.
- Native result handles never escape into public binding code.

### Milestone 6: Move event and logging copying

Separate copying from delivery policy.

Deliverables:

- Move runtime event raw copying into `core`.
- Represent raw source type and raw source address in copied events.
- Move log record copying and raw severity/event mapping into `core`.
- Keep Rust `MapId` lookup and log callback invocation in `maplibre-native`.

Acceptance criteria:

- A bridge crate can copy events and logs without adopting Rust wrapper
  registries or callback policy.
- Unknown event payloads preserve raw bytes.

### Milestone 7: Move resource-provider primitives

Extract request/response conversion and exactly-once request ownership.

Deliverables:

- Move request copying and response materialization.
- Move the internal request-handle state machine.
- Provide a bridge-neutral API for complete, cancelled, release, and
  provider-decision finalization.
- Move `ResourceProviderDecision` into `core` as the shared provider decision
  enum.
- Keep Rust closure types and public callback ergonomics in `maplibre-native`.
- Add tests for inline completion, pass-through, deferred completion,
  cancellation, double completion, close before decision finalization, and
  panic/error fallback.

Acceptance criteria:

- Python, Node.js, and JNI can wrap the same exactly-once request state.
- Direct resource-provider `sys` calls outside `core` are limited to
  host-runtime trampoline entry points.

### Milestone 8: Add bridge-friendly native handle state

Centralize native pointer ownership without imposing Rust public handle policy.

Deliverables:

- Add explicit close-once native handle state in `core`.
- Support status-returning destroy functions for thread-affine handles.
- Support infallible destroy functions for short-lived result handles.
- Support leak-report-only finalizer paths.
- Update public Rust handles to wrap this state while preserving `!Send`, parent
  retention, and owner-thread `Drop`.

Acceptance criteria:

- Bridge crates can store handle state behind locks or runtime objects without
  unsafe pointer ownership logic.
- Public Rust remains `!Send` for thread-affine handles.
- GC finalizers can report leaks without accidentally destroying thread-affine
  handles on arbitrary threads.

### Milestone 9: Reshape the public Rust crate around `core`

Make `maplibre-native` a clean Rust adapter over the shared layer.

Deliverables:

- Remove duplicated conversion logic from the public crate.
- Keep public Rust names, builders, and methods focused on Rust ergonomics.
- Audit all direct `sys` calls in `maplibre-native`; keep only native operation
  calls that belong in Rust handle methods.
- Add tests that assert Rust-specific invariants: `!Send`/`!Sync`, parent
  retention, owner-thread close behavior, frame lifetimes, and callback
  replacement.

Acceptance criteria:

- Public Rust code reads as handle policy plus method calls, not as broad C ABI
  adaptation.
- Shared adaptation tests live in `core`; Rust policy tests live in
  `maplibre-native`.

### Milestone 10: Prove the bridge boundary with skeleton crates

Create thin vertical slices for Python, Node.js, and JNI before filling broad
coverage.

Deliverables:

- Add minimal PyO3, napi-rs, and jni-rs crates.
- Implement a small shared feature slice in each bridge, such as ABI version,
  network status, runtime create/close, map create/close, and one copied event
  or value conversion.
- Keep bridge-specific exceptions/finalizers/loading local to each crate.
- Record any direct `sys` calls needed by the slices and either justify them or
  move missing adapters into `core`.

Acceptance criteria:

- The shared layer supports all three host-runtime models without wrapping the
  public Rust crate.
- Early bridge code validates the architecture before broad binding generation
  begins.

## Ongoing review rules

Use these rules when reviewing future Rust binding changes:

- Code that initializes C `size` fields, field masks, nested native arrays,
  string backing storage, or result-handle copying belongs in `core`.
- If code chooses public method names, exceptions, finalizers, callback queues,
  wrapper retention, or scheduler behavior, it belongs in the public binding
  crate.
- If two bridge crates need the same direct `sys` sequence, move that sequence
  into `core`.
- If an abstraction requires Python, Node.js, Java, or public Rust concepts to
  explain its invariants, keep it above `core`.
- If a finalizer might run on an arbitrary thread, make it report a leak unless
  the native release function is documented as thread-independent and
  infallible.
- Keep examples small and binding-focused; full SDK behavior belongs above these
  low-level bindings.
