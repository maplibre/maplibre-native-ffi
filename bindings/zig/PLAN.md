# Zig binding completion plan

## Audience and purpose

This plan is for maintainers and contributors finishing the initial Zig binding
before merge. It records what already landed, then lists the remaining
milestones that make the binding safe, maintainable, and clear enough for broad
contributor work.

Long-term rules live in the binding conventions documents. This plan tracks the
branch-specific work that remains.

## References

- [Binding conventions](../../docs/src/content/docs/development/bindings.md)
- [Zig binding conventions](../../docs/src/content/docs/development/bindings-zig.md)
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md)
- [Concepts](../../docs/src/content/docs/concepts.md)
- Comparable bindings in `bindings/rust/` and `bindings/java-ffm/`
- Retained direct Zig C ABI tests in `src/c_api/tests/zig/`
- Zig binding tests in `bindings/zig/tests/`

## Current status

The branch has the initial Zig package, private C import, public root exports,
runtime/map/projection/render APIs, values, callbacks, resources, offline
regions, examples, docs, root test integration, and CI variants.

The public package keeps MapLibre C symbols private. Binding tests exercise the
public Zig API, and the direct C suite remains for C ABI-specific coverage that
is still useful below the binding layer.

## Completed history

The initial implementation completed these areas:

1. Created `bindings/zig` with `build.zig`, `build.zig.zon`, a public
   `maplibre_native` module, and private `src/c.zig`.
2. Added `linkMaplibreNativeC` so tests, examples, and consumers share the same
   native artifact configuration.
3. Implemented status mapping, diagnostic copying, ABI validation, and package
   boundary tests that keep raw C declarations out of the public root.
4. Added runtime, map, projection, camera, style, source, value, resource,
   offline, event, logging, render, query, feature-state, and readback coverage.
5. Ported the Zig examples to the binding package.
6. Wired the binding suite into `mise run test` and CI variants.

## Remaining milestones

### Milestone 1: settle handle ownership

Clarify and fix wrapper-owned state for `RuntimeHandle`, `MapHandle`,
`MapProjectionHandle`, and `RenderSessionHandle`.

Current handles allocate Zig state so copied handles can observe `ClosedHandle`,
but `close()` only clears the native pointer. Before merge, choose one model:

- reclaim wrapper state with a non-copyable, borrowed, generational, or
  reference-counted handle design; or
- document process-lifetime wrapper cells as an explicit ownership tradeoff.

Acceptance:

- The public handle contract states who owns native state and wrapper state.
- Tests cover close, double-close, copied closed handles, and use-after-close.
- Long-lived programs have a documented or bounded allocation story.

### Milestone 2: bound resource-request storage

Make handled resource-request storage reusable without allowing stale handles to
complete a later request.

Recommended shape:

- change `ResourceRequestHandle` to carry an index and generation;
- reuse released registry slots through a free list;
- reject stale generations;
- add `errdefer` cleanup when request-state allocation succeeds but registration
  fails.

Acceptance:

- Released request slots can be reused safely.
- Stale handles still fail after release or callback completion.
- Tests cover many handled requests, stale handles, inline completion,
  cross-thread completion, cancellation, and late completion.

### Milestone 3: finish cimport test migration parity

Port assertions, not just files. The binding tests should preserve behavior that
legacy direct Zig C tests covered while routing MapLibre behavior through the
public Zig binding.

Known gaps:

- Feature state: assert copied values, including `hover == true` and
  `radius == 20`, before and after removal.
- Logging: cover invalid or reserved async severity-mask bits and diagnostic
  capture.
- Offline geometry regions: cover create, list, runtime reopen, reload by id,
  geometry and metadata checks, and delete.

Acceptance:

- Each legacy scenario has equivalent public-binding coverage or a documented
  reason to keep it in the direct C suite.
- The direct C suite contains C ABI-level tests, not duplicate binding behavior.
- `mise run test` passes with the binding suite and the retained direct C suite.

### Milestone 4: decide API parity scope for the initial release

Status: completed. The initial Zig release keeps the current tested low-level
surface and defers the reviewed parity gaps below. These gaps are mechanical API
coverage over existing C entry points, not ownership or safety blockers.
Required pre-merge work remains focused on lifetime, callback, copied-output,
and C ABI migration correctness.

Deferred follow-up list with comparable APIs:

- Camera and navigation helpers: still-image requests, animated gesture
  variants, fitting for coordinate lists and geometry, unwrapped bounds, bounds
  constraints, and free-camera options. Comparable APIs: Rust
  `bindings/rust/crates/maplibre-native/src/map.rs` methods
  `request_still_image`, `move_by_animated`, `scale_by_animated`,
  `rotate_by_animated`, `pitch_by_animated`, `camera_for_lat_lngs`,
  `camera_for_geometry`, `lat_lng_bounds_for_camera_unwrapped`, `bounds`,
  `set_bounds`, `free_camera_options`, and `set_free_camera_options`; Java
  `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/map/MapHandle.java`
  methods `requestStillImage`, `moveByAnimated`, `scaleByAnimated`,
  `rotateByAnimated`, `pitchByAnimated`, `cameraForLatLngs`,
  `cameraForGeometry`, `latLngBoundsForCameraUnwrapped`, `bounds`, `setBounds`,
  `freeCameraOptions`, and `setFreeCameraOptions`.
- Style layer lifecycle and JSON helpers: add, remove, exists, move, and fetch
  layer JSON. Comparable APIs: Rust `map/style.rs` methods
  `add_style_layer_json`, `style_layer_json`, and style layer ID/type helpers;
  Java `MapHandle.java` methods `addStyleLayerJson`, `removeStyleLayer`,
  `styleLayerExists`, `moveStyleLayer`, and `styleLayerJson`.
- Projection `setVisibleGeometry`. Comparable APIs: Rust
  `bindings/rust/crates/maplibre-native/src/projection.rs` method
  `set_visible_geometry`; Java `MapProjectionHandle.java` method
  `setVisibleGeometry`.
- Resource-transform clearing. Comparable implementation point: Java
  `RuntimeHandle.java` resource-transform lifecycle; Zig currently supports
  replacement only while native state accepts it.
- Logging restore-default helper. Comparable API: Rust
  `bindings/rust/crates/maplibre-native/src/logging.rs`
  `restore_default_async_log_severity_mask`; Zig callers use
  `setAsyncLogSeverityMask(.default, ...)`.
- Ergonomic overloads or optional placement for style-layer insertion.
  Comparable APIs: Java `MapHandle.java` overloads for `addStyleLayerJson` and
  concrete layer helpers with optional `beforeLayerId`; Zig can add optional
  placement wrappers without changing current explicit methods.

Acceptance:

- Required gaps have implementations and tests.
- Deferred gaps appear in a short follow-up list with matching Rust/Java API
  references.
- Public names and semantics stay consistent with the Zig binding conventions.

### Milestone 5: harden correctness edges

Address the correctness issues that can pass the current test suite.

Work items:

- destroy a native render session if Zig render-session state allocation fails
  after `attach()` succeeds;
- add binding-level active-frame preflight checks around render-session
  operations that conflict with active owned frames;
- return an error for unknown native JSON value tags and feature-identifier tags
  instead of silently converting them to `null`;
- align growable native output domains, such as style source types, with the Zig
  convention to preserve unknown raw values.

Acceptance:

- Allocation-failure paths release native resources.
- Render-session operations report binding errors before crossing into C when an
  active frame makes the operation invalid.
- Unknown closed-domain tags fail clearly.
- Unknown growable-domain values preserve raw data.

### Milestone 6: relocate retained C ABI tests

Status: completed. Retained raw C ABI tests now live in `src/c_api/tests/zig/`,
next to the C API implementation. The root `build.zig` test entrypoint and
Objective-C support-file path use that location, and the root test step
describes the suite as retained Zig C ABI tests.

Acceptance:

- `mise run test` still runs the retained C ABI suite before the Zig binding
  suite.
- The top-level `tests/` directory no longer contains the raw Zig C API suite.
- Binding behavior lives in `bindings/zig/tests/`; raw C ABI behavior lives with
  the C API implementation.

### Milestone 7: clean the branch for review

Keep merge artifacts focused on source, tests, docs, and examples.

Work items:

- keep Ralph execution logs out of the merge branch;
- remove stale one-time implementation notes from this plan;
- remove or use dead state such as `ResourceRequestState.released`;
- factor repeated Windows test-runner path setup in `bindings/zig/build.zig` if
  nearby build logic changes;
- defer splitting large modules until ownership or testability pressure
  justifies the churn.

Acceptance:

- `git diff main...HEAD` contains intentional source, test, example, and
  documentation changes.
- Planning documents describe current remaining work.
- Cleanup changes reduce maintenance burden without broad rewrites.

### Milestone 8: validate the merge candidate

Run focused checks after the milestones above land.

Required checks:

```sh
mise run //bindings/zig:test
mise run test
mise run fix
```

Acceptance:

- Binding tests pass through public Zig APIs.
- Retained direct C tests pass and cover C ABI behavior.
- Formatters and linters pass.
- Any skipped platform/backend checks have a short explanation in the merge
  notes.

## Definition of done before merge

The branch is ready to merge when:

- the public package exposes no raw MapLibre C declarations;
- handle, callback, diagnostic, copied-output, resource-request, and
  render-frame lifetimes have clear ownership and tests;
- binding tests preserve the intended legacy cimport coverage;
- binding-level behavior lives in `bindings/zig/tests/`, including normal
  lifecycle flows, public error mapping, copied outputs, callback behavior,
  resources, offline regions, style/source helpers, camera/projection, queries,
  feature state, render sessions, and readback;
- tests outside `bindings/zig/tests/` cover only behavior that needs raw C or
  host integration access, such as descriptor size/version contracts,
  null-pointer validation, raw invalid enum values, stale native handles,
  undersized C output buffers, backend host scaffolding, and C ABI smoke tests;
- required Rust/Java parity gaps are implemented or explicitly deferred;
- examples use the binding package;
- root test and CI paths include the Zig binding suite;
- branch-local execution artifacts are absent from the diff.
