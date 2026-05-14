# Zig binding implementation Ralph loop

Implement `bindings/zig/PLAN.md` from start to finish on the current branch.

## Goals

- Build a supported low-level Zig package for MapLibre Native FFI.
- Keep `@cImport` and raw C declarations private to the Zig package.
- Migrate Zig C API test coverage to public Zig binding APIs milestone by
  milestone.
- Keep Ralph artifacts under `.ralph/` tracked in git on this branch.
- At every phase or meaningful milestone: run a parallel review, apply all
  sensible findings, update this file with evidence, commit, and push.

## Required context before implementation

- Read `docs/src/content/docs/development/overview.md` for workflow commands.
- Read `docs/src/content/docs/concepts.md` before changing related scope,
  ownership, threading, events, rendering targets, or host integration behavior.
- Read `docs/src/content/docs/development/c-conventions.md` before changing C
  ABI behavior, callbacks, diagnostics, or render target contracts.
- Read `docs/src/content/docs/development/bindings.md` and
  `docs/src/content/docs/development/bindings-zig.md` before changing Zig
  binding APIs or tests.
- Use `mise run test` to build and test when practical; use narrower binding
  tasks during early phases, then root test once integrated.
- Use `mise run fix` before milestone commits unless the milestone is
  intentionally limited to setup and has no formatting-sensitive code.

## Milestone protocol

For each phase or smaller milestone:

1. Implement only the next coherent slice.
2. Update this Ralph file with decisions, changed files, and verification
   evidence.
3. Run applicable tests and format/lint commands.
4. Run a parallel review before committing:
   - Use the subagent workflow in parallel with at least two independent
     reviewers when available.
   - Ask reviewers to inspect the current diff against `bindings/zig/PLAN.md`,
     binding conventions, C API conventions, handle lifetimes, diagnostics,
     callback safety, tests, and build integration.
   - Record review commands/prompts and findings in this file.
5. Apply all sensible findings.
6. Document any findings intentionally deferred or rejected with reasons.
7. Re-run applicable verification after changes.
8. Commit the milestone, including `.ralph/zig-binding-implementation.md`, then
   push the branch.

## Checklist

### Phase 0: loop setup

- [x] Load Ralph Loop skill.
- [x] Create tracked Ralph task file under `.ralph/`.
- [x] Commit and push initial Ralph setup.

### Phase 1: package skeleton

- [x] Add `bindings/zig/build.zig` and `bindings/zig/build.zig.zon`.
- [x] Add `bindings/zig/src/maplibre_native.zig` as public root.
- [x] Add private `bindings/zig/src/c.zig` with the only public-package
      `@cImport` of `maplibre_native_c.h`.
- [x] Add build options for CMake artifact directory, render backend variant,
      platform include/library paths, and test rpath.
- [x] Add `zig build test` step linking `maplibre-native-c`.
- [x] Add or update mise tasks for `//bindings/zig:build`,
      `//bindings/zig:test`, and `//bindings/zig:ci`.
- [x] Verify empty binding tests compile and link.
- [x] Review, apply findings, commit, and push.

### Phase 2: status and diagnostics

- [x] Define native status error mapping.
- [x] Define public binding error set for status and binding-local validation
      errors.
- [x] Add owned diagnostic record and `DiagnosticStore`.
- [x] Implement `checkStatus(status, diagnostics)` with thread-local message
      copying before later C calls.
- [x] Add ABI version validation with diagnostics.
- [x] Add tests for invalid status mapping, copied diagnostics, and unknown
      status preservation.
- [x] Review, apply findings, commit, and push.

### Phase 3: runtime and map vertical slice

- [x] Implement `RuntimeHandle` with create/init, `runOnce`, `pollEvent`, and
      fallible close.
- [x] Implement `MapHandle` with creation, basic options, style setter, and
      fallible close.
- [x] Implement initial `MapProjectionHandle` ownership and close.
- [x] Add private live-handle checks and retryable close behavior.
- [x] Verify no public API leaks raw C handle types.
- [x] Add vertical-slice binding test.
- [x] Review, apply findings, commit, and push.

### Phase 4: foundational tests

- [x] Port `diagnostics.zig` assertions to binding tests.
- [x] Port `runtime.zig` assertions to binding tests.
- [x] Port `map_lifecycle.zig` assertions to binding tests.
- [x] Retire exact duplicate direct C API assertions only after binding coverage
      preserves intent.
- [x] Keep a private C import compile test if needed for early header
      bindability.
- [x] Review, apply findings, commit, and push.

### Phase 5: options, values, and copied results

- [ ] Add public Zig value types needed for camera, geometry, render extents,
      style images, query options, offline regions, events, and resource
      metadata.
- [ ] Represent C option structs as semantic Zig descriptors and materialize C
      ABI fields internally.
- [ ] Add private temporary storage helpers for strings, string views, arrays,
      nested descriptor graphs, and output pointers.
- [ ] Add native result guards for snapshot/list/query handles.
- [ ] Add owned output deinit paths and tests.
- [ ] Port matching value/descriptor tests: style, style values, camera,
      projection, map tuning, and geojson as supported.
- [ ] Review, apply findings, commit, and push.

### Phase 6: events and source identity

- [x] Add binding-assigned `MapId`.
- [x] Register maps with runtimes for source lookup while handles are live.
- [x] Copy runtime events into owned Zig values before next poll.
- [x] Preserve unknown event and payload raw values.
- [x] Apply binding-owned side effects required by other bindings where
      applicable.
- [x] Port `events.zig` coverage.
- [x] Review, apply findings, commit, and push.

### Phase 7: logging, resources, offline, and callbacks

- [x] Add network status and logging callback APIs.
- [x] Add runtime ambient cache operations.
- [x] Add offline region APIs with copied value types.
- [x] Add resource transform callbacks.
- [x] Add resource provider callbacks and `ResourceRequestHandle`.
- [x] Add custom geometry source callbacks and lifetime management.
- [x] Model callbacks as function pointers plus context pointers first.
- [x] Ensure C trampolines use `callconv(.c)`, copy borrowed data before
      returning, and never let failures escape through C frames.
- [x] Enforce one-shot request completion or release.
- [x] Port logging, resources, offline, and custom geometry tests.
- [x] Review, apply findings, commit, and push.

### Phase 8: render targets and readback

- [x] Add `NativePointer` borrowed opaque backend address value.
- [x] Add public render target descriptors for supported Metal and Vulkan
      surfaces, borrowed textures, and owned textures.
- [x] Add `RenderSessionHandle` with fallible close.
- [x] Add readback APIs for caller-owned buffers and allocator-backed owned
      images.
- [x] Add owned texture frame handles with scoped native pointer access.
- [x] Port render backend, surface, texture, feature-state, and query render
      coverage.
- [x] Review, apply findings, commit, and push.

### Phase 9: examples and documentation

- [x] Port `examples/zig-readback` to the binding.
- [x] Port `examples/zig-map` to the binding.
- [x] Update contributor docs and command lists for Zig binding tasks.
- [x] Add the Zig binding suite to root `mise run test` when supported by native
      variants.
- [x] Update `.github/config/variants.toml` and workflow task lists for
      supported platform/render-backend variants and explicit exclusions.
- [x] Keep examples small and focused on low-level binding usage.
- [ ] Review, apply findings, commit, and push.

### Final definition of done

- [ ] Public package exposes no raw C declarations.
- [ ] Relocated Zig binding tests pass through public binding APIs.
- [ ] Zig binding suite covers behavior previously covered by direct Zig C API
      tests.
- [ ] Handle close, diagnostic capture, copied output, callback state, and
      render frame lifetimes have tests.
- [ ] Examples use the binding package.
- [ ] `mise run test` includes the Zig binding suite in the normal project path
      where supported.
- [ ] Final parallel review has no blocking findings or all sensible findings
      are applied.
- [ ] Final full verification is recorded.
- [ ] Final commit is pushed.
- [ ] Output `<promise>COMPLETE</promise>` only after the final pushed state
      satisfies this definition.

## Verification log

- 2026-05-14: Loaded Ralph skill and created this task file from
  `bindings/zig/PLAN.md`.
- 2026-05-14: Confirmed `.ralph/zig-binding-implementation.md` is not ignored,
  committed it as `c500d79` (`Track Zig binding Ralph loop`), and pushed
  `zig-binding-implementation` to origin.
- 2026-05-14: Iteration 1 read required context docs:
  `docs/src/content/docs/development/overview.md`,
  `docs/src/content/docs/concepts.md`,
  `docs/src/content/docs/development/c-conventions.md`,
  `docs/src/content/docs/development/bindings.md`, and
  `docs/src/content/docs/development/bindings-zig.md`.
- 2026-05-14: Iteration 1 inspected existing Zig direct C tests in `tests/c/`,
  Zig examples in `examples/zig-*`, root `build.zig`, root `mise.toml`, and
  example mise tasks to plan Phase 1 build integration.
- 2026-05-14: Restored the full 212-line Ralph task file after `ralph_start`
  rewrote it with the shorter startup task content; keep the committed file as
  canonical and avoid passing abbreviated task content for this loop again.
- 2026-05-14: Iteration 2 added the initial Zig package skeleton:
  `bindings/zig/build.zig`, `bindings/zig/build.zig.zon`,
  `bindings/zig/src/maplibre_native.zig`, private `bindings/zig/src/c.zig`,
  `bindings/zig/tests/main.zig`, and `bindings/zig/mise.toml`.
- 2026-05-14: Iteration 2 verification passed: `mise run //bindings/zig:test`
  (2/2 tests passed; compiles private C import and links `maplibre-native-c`)
  and `mise run //bindings/zig:build`.
- 2026-05-14: Iteration 2 verified missing build options fail clearly with
  direct `zig build test` checks for omitted `-Drender-backend` and omitted
  `-Dcmake-artifact-dir`.
- 2026-05-14: Iteration 3 applied Phase 1 parallel review findings: made the
  default `zig build` compile the binding test artifact, changed
  `linkMaplibreNativeC` to require explicit consumer paths, mirrored root
  multi-config native library directory resolution in `bindings/zig/mise.toml`,
  added explicit target/backend validation, and removed the raw public render
  backend bitmask helper.
- 2026-05-14: Iteration 3 verification passed after review fixes:
  `mise run fix`, `mise run //bindings/zig:test`, and
  `cd bindings/zig && zig build --summary all -Dcmake-artifact-dir=../../build/macos-arm64-metal -Drender-backend=metal`
  showing the default build compiles the test artifact.
- 2026-05-14: Iteration 3 committed Phase 1 as `1fe500f`
  (`Add initial Zig binding package skeleton`) and pushed
  `zig-binding-implementation` to origin.
- 2026-05-14: Iteration 4 implemented Phase 2 status and diagnostics:
  `bindings/zig/src/status.zig`, `bindings/zig/src/diagnostics.zig`, public root
  exports for `DiagnosticStore`, error sets, and ABI validation, plus a private
  status-module test artifact wired into `bindings/zig/build.zig`.
- 2026-05-14: Iteration 4 verification passed: `mise run //bindings/zig:test`
  ran both public package tests and private status-module tests (7/7 tests
  passed).
- 2026-05-14: Iteration 5 applied Phase 2 review findings: `DiagnosticStore.get`
  now returns a read-only pointer to store-owned diagnostics, diagnostic message
  slices are read-only to callers, `Diagnostic` no longer exposes a public
  deinit footgun, and the copied-diagnostics test now performs a later real C
  API call before checking the stored copy.
- 2026-05-14: Iteration 5 verification passed after review fixes:
  `mise run
  //bindings/zig:test`, `mise run fix`, and
  `mise run //bindings/zig:test` again after formatting/lint fixes.
- 2026-05-14: Iteration 5 reflection: Phase 1 and Phase 2 are complete and
  pushed-ready; the milestone review loop is catching useful ownership/test
  gaps. Keep using private module tests for internals until public handle APIs
  can cover the same behavior through `maplibre_native`.
- 2026-05-14: Iteration 5 committed Phase 2 as `77e9f34`
  (`Add Zig status diagnostics layer`) and pushed `zig-binding-implementation`
  to origin.
- 2026-05-14: Iteration 6 reflection:
  - Accomplished so far: Phase 1 package skeleton and Phase 2 status/diagnostics
    are reviewed, committed, and pushed; Phase 3 now has the first runtime/map
    slice in progress.
  - Working well: small phase-sized commits, fresh parallel reviews, and narrow
    `//bindings/zig:test` verification catch issues quickly while root test
    integration waits for a supported binding path.
  - Not working/blocking: Zig has no field privacy for public structs, so raw
    native handle storage needs careful shaping. The current Phase 3 slice uses
    opaque handle storage to avoid public raw C handle fields, but it still
    needs review before committing.
  - Approach adjustment: keep milestone reviews before every commit, and add a
    focused public-boundary check for handle types before marking Phase 3 done.
  - Next priorities: add `MapProjectionHandle`, tighten live-handle/public
    boundary checks, then run the Phase 3 parallel review.
- 2026-05-14: Iteration 6 implemented the first Phase 3 vertical slice:
  `RuntimeHandle`, `MapHandle`, basic options, `runOnce`, `pollEvent`, style
  JSON/URL setters, fallible idempotent close, embedded-NUL string validation,
  and public tests for runtime/map creation, style JSON, closed-handle errors,
  and string validation.
- 2026-05-14: Iteration 6 verification passed: `mise run //bindings/zig:test`
  (10/10 tests passed).
- 2026-05-14: Iteration 7 completed the Phase 3 implementation slice before
  review: added `MapProjectionHandle`, removed public `nativeOpaque` methods in
  favor of non-root internal module helpers, added a public-boundary test that
  handle exported types do not contain raw `mln_` type names, and extended the
  vertical-slice test to create and close a projection snapshot.
- 2026-05-14: Iteration 7 verification passed: `mise run //bindings/zig:test`
  (10/10 tests passed).
- 2026-05-14: Iteration 8 ran Phase 3 parallel review (`22251894`) and follow-up
  review (`373e331f`). Applied findings: removed borrowed event message exposure
  from `RuntimeEvent`, added wrong-thread diagnostic coverage, added close-retry
  coverage, checked handle use before string materialization, strengthened
  public-boundary tests, and replaced public `?*anyopaque` native storage with
  private allocated state behind opaque state pointers.
- 2026-05-14: Iteration 8 verification passed after review fixes:
  `mise run //bindings/zig:test` (12/12 tests), `mise run fix`, and
  `mise run //bindings/zig:test` again after formatting/lint fixes.
- 2026-05-14: Iteration 8 committed Phase 3 as `3d3d826`
  (`Add Zig runtime map vertical slice`) and pushed `zig-binding-implementation`
  to origin.
- 2026-05-14: Iteration 9 ported foundational diagnostics/runtime/map lifecycle
  assertions into public binding tests: lifecycle diagnostics capture and copy
  behavior, second-runtime same-thread rejection, distinct-thread runtime
  creation, map option validation, wrong-thread runtime and map errors with
  diagnostics, and multiple-map runtime pumping.
- 2026-05-14: Iteration 9 verification passed: `mise run //bindings/zig:test`
  (17/17 tests passed).
- 2026-05-14: Iteration 10 reorganized foundational binding tests into
  `bindings/zig/tests/diagnostics.zig`, `runtime.zig`, `map_lifecycle.zig`, and
  `support.zig`, ran Phase 4 parallel review (`2fec76ed`), and applied findings
  by adding wrong-thread runtime `pollEvent`/`close` coverage, runtime pump
  after map close, repaint-to-render-update event coverage, and live-handle
  embedded-NUL validation.
- 2026-05-14: Iteration 10 verification passed: `mise run //bindings/zig:test`
  (20/20 tests passed). Direct C tests remain for C ABI-only assertions, raw
  stale-handle behavior, and exact duplicate retirement decisions in the next
  slice.
- 2026-05-14: Iteration 10 committed the foundational binding test port as
  `e3017d7` (`Port foundational Zig binding tests`) and pushed
  `zig-binding-implementation` to origin.
- 2026-05-14: Iteration 11 reflection:
  - Accomplished so far: Phases 1-3 are complete, reviewed, committed, and
    pushed. Phase 4 now has public binding tests for foundational diagnostics,
    runtime, and map lifecycle behavior.
  - Working well: the review-before-commit rhythm continues to catch concrete
    safety gaps, and the split binding test files make coverage easier to map
    back to direct C test areas.
  - Not working/blocking: exact duplicate retirement is necessarily granular;
    direct C tests still contain C ABI-only assertions mixed with native
    behavior assertions.
  - Approach adjustment: retire only assertions that public binding tests now
    preserve, while keeping direct C tests for null pointers, undersized
    structs, out-parameter initialization, raw enum/status domains, and stale
    raw handles.
  - Next priorities: finish Phase 4 commit, then start Phase 5 by adding the
    value/descriptor and memory helpers needed for style/camera/projection test
    ports.
- 2026-05-14: Iteration 11 retired exact duplicate direct C assertions from
  `tests/c/runtime.zig` and `tests/c/map_lifecycle.zig`: wrong-thread runtime
  destroy/poll, runtime pump before/after maps, distinct-thread runtime
  creation, map width/height/scale validation, multiple maps, and wrong-thread
  map repaint. Kept C ABI-specific invalid argument, raw stale-handle,
  out-pointer, raw enum, and still-image invalid-state coverage.
- 2026-05-14: Iteration 11 verification passed: `mise run test` (117 passed, 12
  skipped), `mise run //bindings/zig:test` (20/20 tests passed), `mise run fix`,
  then `mise run test` and `mise run //bindings/zig:test` again. The binding
  private `status.zig` test artifact and remaining direct C suite continue to
  compile the private C import/header path.
- 2026-05-14: Iteration 11 committed Phase 4 duplicate-retirement work as
  `5deffc3` (`Retire duplicated foundational C assertions`) and pushed
  `zig-binding-implementation` to origin.
- 2026-05-14: Iteration 12 started Phase 5 with the camera/projection value
  slice: added public semantic values for geographic coordinates, screen points,
  edge insets, bounds, projected meters, camera options, animation options,
  camera-fit options, and projection modes in `bindings/zig/src/values.zig`.
- 2026-05-14: Iteration 12 added internal descriptor materialization and temp
  storage: C option structs are now created inside the binding from semantic Zig
  descriptors, and `bindings/zig/src/native_temp.zig` owns temporary arrays for
  coordinate and screen-point calls.
- 2026-05-14: Iteration 12 ported the first Phase 5 public binding coverage for
  camera and projection behavior in `bindings/zig/tests/camera.zig` and
  `bindings/zig/tests/projection.zig`, covering camera snapshots/commands,
  camera fitting, projection mode snapshots, map/projection coordinate
  conversion, and projected-meter helpers.
- 2026-05-14: Iteration 12 verification passed: `mise run //bindings/zig:test`
  (28/28 tests passed). This slice still needs the Phase 5 milestone review,
  formatting/fix pass, broader value/descriptor coverage, and commit/push.
- 2026-05-14: Iteration 13 expanded Phase 5 value coverage to map tuning: added
  public semantic debug options, viewport option enums/descriptors, and tile
  option enums/descriptors, with internal raw enum and field-mask
  materialization in `bindings/zig/src/values.zig`.
- 2026-05-14: Iteration 13 added public `MapHandle` tuning APIs for debug
  options, rendering stats, loaded-state queries, debug-log dump, viewport
  options, and tile options without exposing raw C bitmasks or option structs.
- 2026-05-14: Iteration 13 ported map tuning coverage to
  `bindings/zig/tests/map_tuning.zig`, covering debug option round trips,
  rendering stats toggles, viewport/tile selected-field updates, and invalid
  descriptor values surfaced as public binding errors.
- 2026-05-14: Iteration 13 verification passed: `mise run //bindings/zig:test`
  (32/32 tests passed; `dumpDebugLogs` emitted native debug log lines during the
  run). Phase 5 still needs style/style-value/geojson descriptors, copied-result
  guards/deinit paths, review, fix, commit, and push.
- 2026-05-14: Iteration 14 added semantic JSON descriptors and owned copied JSON
  outputs: public `JsonValue`/`JsonMember` inputs materialize recursive C
  descriptor graphs through arena-backed temporary storage, while
  `OwnedJsonValue` copies snapshot data before destroying native snapshot
  handles and provides explicit recursive `deinit`.
- 2026-05-14: Iteration 14 added copied style ID list output with `StringList`
  and public `MapHandle` style-value APIs for layer properties, layer filters,
  style source ID lists, and style layer ID lists. Native snapshot/list handles
  are guarded as private temporaries and always destroyed after copying.
- 2026-05-14: Iteration 14 expanded the binding test style fixture to include a
  GeoJSON source and circle layer, then ported style-value coverage in
  `bindings/zig/tests/style_values.zig` for owned ID lists, layer property JSON
  values, nested filter arrays, copied snapshot deinit, and invalid JSON
  descriptor/string handling.
- 2026-05-14: Iteration 14 verification passed: `mise run //bindings/zig:test`
  (36/36 tests passed; native debug log lines still appear from the map tuning
  `dumpDebugLogs` test). Phase 5 still needs geometry/GeoJSON descriptors,
  broader style/source coverage as supported, review, fix, commit, and push.
- 2026-05-14: Iteration 15 added public semantic geometry and GeoJSON input
  descriptors: `Geometry`, `FeatureIdentifier`, `Feature`, and `GeoJson` cover
  points, lines, polygons, multi-geometries, geometry collections, feature
  properties, feature IDs, and feature collections without exposing raw C
  descriptor structs.
- 2026-05-14: Iteration 15 extended private temporary storage to materialize
  nested geometry, feature, GeoJSON, coordinate-span, polygon, and JSON member
  descriptor graphs into arena-owned C ABI memory borrowed only for the native
  call.
- 2026-05-14: Iteration 15 added public GeoJSON source APIs on `MapHandle` for
  source existence checks, adding/updating inline GeoJSON data, and adding/
  updating GeoJSON URLs, then ported `geojson.zig` coverage through public
  binding APIs for source add/update, nested geometry collections, invalid
  native geometry values, and embedded-NUL descriptor strings.
- 2026-05-14: Iteration 15 verification passed: `mise run //bindings/zig:test`
  (39/39 tests passed; native debug log lines still appear from the map tuning
  `dumpDebugLogs` test). Phase 5 still needs broader style/source coverage as
  supported, duplicate direct-C retirement decisions, review, fix, commit, and
  push.
- 2026-05-14: Iteration 16 reflection:
  - Accomplished so far: Phases 1-4 are complete, reviewed, committed, and
    pushed. Phase 5 now has public value descriptors for camera/projection, map
    tuning, JSON values, copied JSON snapshots/lists, geometry, GeoJSON, and a
    growing set of style/source APIs.
  - Working well: semantic descriptors plus private arena materialization keep
    raw C structs out of the public API while preserving the C ABI's
    borrowed-for-call lifetime model. The binding tests now cover progressively
    richer behavior through `maplibre_native` instead of direct `@cImport`.
  - Not working/blocking: Phase 5 has become broad enough that review and
    duplicate-retirement should happen soon. `values.zig` and `native_temp.zig`
    are growing into large mixed-responsibility files and may need splitting
    after the current milestone review.
  - Approach adjustment: finish one more coherent style/source coverage slice,
    then run formatting, parallel review, and targeted duplicate retirement
    rather than continuing to expand Phase 5 indefinitely.
  - Next priorities: add style source metadata/attribution coverage, run the
    Phase 5 review loop, retire direct C duplicates that public binding tests
    now preserve, then commit and push the reviewed milestone.
- 2026-05-14: Iteration 16 added the style source metadata slice: public
  `StyleSourceType`, `StyleSourceInfo`, and `OwnedString` copied-attribution
  values; `MapHandle` APIs for adding/removing style source JSON, source type
  and info lookup, and attribution copying; and `style_sources.zig` tests for
  source JSON descriptors, metadata, copied attribution deinit, missing-source
  results, removal, and invalid source descriptors.
- 2026-05-14: Iteration 16 verification passed: `mise run //bindings/zig:test`
  (42/42 tests passed; native debug log lines still appear from the map tuning
  `dumpDebugLogs` test). Next iteration should run formatting, Phase 5 review,
  and direct-C duplicate retirement rather than expanding the milestone much
  further.
- 2026-05-14: Iteration 17 ran `mise run fix`; dprint formatted the Ralph JSON,
  Ralph Markdown, and touched Zig files. Verification after formatting passed:
  `mise run //bindings/zig:test` (42/42 tests passed).
- 2026-05-14: Iteration 17 ran Phase 5 parallel review `9679e64a` with two
  reviewers. Applied blocking findings: removed public raw-C leakage from
  `OwnedJsonValue.copyFromNative` by moving native JSON copying to an internal
  module-level helper, changed geometry coordinate spans to arena-stable arrays
  so nested descriptor graphs cannot dangle after later appends, fixed empty
  native JSON array/object copying, and fixed copied attribution deinit by
  returning an exact allocation length.
- 2026-05-14: Iteration 17 added regressions for the applied review findings:
  public-boundary coverage that `OwnedJsonValue` has no `copyFromNative` raw-C
  helper, an internal empty native JSON array/object copy test, and a public
  multi-line GeoJSON source test with enough coordinates to exercise stable
  nested coordinate span materialization. Verification passed after fixes:
  `mise run //bindings/zig:test` (43/43 tests passed).
- 2026-05-14: Iteration 18 retired exact duplicate direct C assertions now
  covered by Phase 5 binding tests: camera jump snapshots, projection mode
  snapshots, map coordinate conversion, projected-meter conversion, map debug
  and tuning option round trips, layer property/filter values, and invalid
  layer-property descriptor/conversion behavior. Kept direct C coverage for C
  ABI-only defaults, undersized structs, raw unknown enum/field masks, null
  pointers/out-pointers, source/layer/image/light APIs not yet ported, and
  callback/resource/render coverage not yet in the binding.
- 2026-05-14: Iteration 18 verification before and after formatting passed:
  `mise run test` (107 passed, 12 skipped), `mise run //bindings/zig:test`
  (43/43 tests passed), `mise run fix`, then `mise run test` (107 passed, 12
  skipped) and `mise run //bindings/zig:test` (43/43 tests passed).
- 2026-05-14: Iteration 18 committed the reviewed Phase 5 value/descriptor
  milestone as `db14aa0` (`Add Zig binding value descriptors`) and pushed
  `zig-binding-implementation` to origin.
- 2026-05-14: Iteration 18 added follow-up commit `d9f1037`
  (`Record Zig value
  descriptor milestone`) with the Ralph record of the
  pushed milestone.
- 2026-05-14: Iteration 19 started Phase 6 event/source identity work: added a
  public binding-assigned `MapId`, assigned IDs when maps are created, and
  registered live maps with their runtime for native event source lookup.
- 2026-05-14: Iteration 19 added copied runtime event polling with
  `RuntimeHandle.pollEventOwned(allocator)`, returning an `OwnedRuntimeEvent`
  that owns copied message bytes and resolves map-originated events back to a
  `MapId` while preserving the existing lightweight `pollEvent()` API.
- 2026-05-14: Iteration 19 added binding coverage for unsupported-style failure
  events: the test verifies owned event message persistence after a later
  runtime call and verifies the event source resolves to the creating map's
  `MapId`. Verification passed: `mise run //bindings/zig:test` (44/44 tests
  passed; expected native unsupported-URL log line appears during the new test).
- 2026-05-14: Iteration 21 reflection:
  - Accomplished so far: Phases 1-5 are reviewed, committed, and pushed; Phase 6
    now has map identity, live runtime registration, owned event messages, and
    the first public event-source coverage in progress.
  - Working well: keeping `pollEvent()` lightweight while adding
    `pollEventOwned(allocator)` gives callers a clear choice between borrowed
    native event metadata and allocator-backed copies.
  - Not working/blocking: event payload coverage is broad and some payloads need
    callback/render/offline setup before they can be observed through purely
    public binding tests.
  - Approach adjustment: copy and model all payload shapes now, use private
    payload-copy tests for hard-to-trigger raw domains, then add public tests as
    later render/offline/callback APIs make those events observable.
  - Next priorities: finish event payload modeling, port the remaining feasible
    `events.zig` public coverage, run Phase 6 review, apply findings, verify,
    commit, and push.
- 2026-05-14: Iteration 21 expanded `OwnedRuntimeEvent` with owned payload
  copies for render-frame, render-map, style-image-missing, tile-action,
  offline-status, offline-response-error, tile-count-limit, and unknown payload
  bytes. Unknown event, source, payload, render mode, tile operation, offline
  state, and resource-error raw values are preserved as public `unknown` union
  cases instead of collapsing to generic errors.
- 2026-05-14: Iteration 21 added private runtime-module tests for raw-domain
  preservation, borrowed tile-action source ID copying, unknown payload byte
  copying, and malformed native payload rejection, then wired `src/runtime.zig`
  into the private Zig binding test step. Public runtime coverage now also
  asserts no payload for the unsupported-style loading failure and covers
  map-close event draining. Verification passed: `mise run //bindings/zig:test`
  (53/53 tests passed; expected native log lines appear during event/style
  tests).
- 2026-05-14: Iteration 22 ported and retired the remaining duplicate public
  event behavior from `tests/c/events.zig`: empty queue polling now runs through
  both `RuntimeHandle.pollEvent()` and `pollEventOwned()`, unsupported-style
  loading failure message/source/payload coverage is in the public binding test,
  and map-close event draining is covered through `MapHandle.close()`. Remaining
  direct C event tests are C ABI-specific invalid-output and raw payload-layout
  checks.
- 2026-05-14: Iteration 22 ran `mise run //bindings/zig:test` (54/54 tests
  passed), `mise run test` (104 passed, 12 skipped), `mise run fix`, and
  `mise run //bindings/zig:test` again (54/54 tests passed) before review fixes.
- 2026-05-14: Iteration 22 ran Phase 6 parallel review `29c7eea9` with two
  reviewers. Blocking findings applied: the public copied-message lifetime test
  now performs a later event poll before asserting the copied bytes remain
  stable; private payload-copy tests now mutate borrowed tile-action and unknown
  payload backing storage after copying; `payloadAs` validates byte size,
  pointer alignment, and the payload struct's leading `size` field before
  casting/reading; malformed payload tests now cover null, undersized, and
  misaligned payload pointers. Also changed the public source-ID assertion to
  return `error.MissingSourceId` instead of force-unwrapping.
- 2026-05-14: Iteration 22 verification after review fixes passed:
  `mise run //bindings/zig:test` (54/54 tests passed), `mise run fix`, then
  `mise run //bindings/zig:test` again (54/54 tests passed; expected native log
  lines appear during event/style tests). Deferred public payload-bearing event
  lifetime coverage until render/offline/callback APIs make such events
  observable through public binding APIs.
- 2026-05-14: Iteration 23 final Phase 6 root verification passed before the
  milestone commit: `mise run test` (104 passed, 12 skipped).
- 2026-05-14: Iteration 23 committed the reviewed Phase 6 event/source identity
  milestone as `97db8ec` (`Add Zig runtime event bindings`) and pushed
  `zig-binding-implementation` to origin.
- 2026-05-14: Iteration 24 started Phase 7 with process-global network status,
  runtime ambient cache operations, and logging callbacks. Added semantic
  `NetworkStatus`, `AmbientCacheOperation`, `LogSeverity`, `LogEvent`,
  `LogSeverityMask`, `LogRecord`, and callback registration APIs without
  exposing raw C declarations; the logging C trampoline copies only borrowed
  callback metadata into a Zig `LogRecord` view and calls a stored function
  pointer plus context pointer.
- 2026-05-14: Iteration 24 added public binding tests for network status get/set
  and invalid status diagnostics, ambient cache pack on a runtime without
  explicit cache path, log callback receive/consume behavior, severity masks,
  and callback clearing. Added private logging-module coverage for unknown raw
  log severity/event preservation and wired the module into the Zig binding
  private test step.
- 2026-05-14: Iteration 24 verification passed: `mise run //bindings/zig:test`
  (60/60 tests passed), `mise run fix`, then `mise run //bindings/zig:test`
  again (60/60 tests passed; expected native log lines appear during event/style
  tests). Direct C duplicate retirement and Phase 7 review remain for a later
  coherent callback/resource milestone.
- 2026-05-14: Iteration 25 added runtime-scoped resource transform callbacks:
  public `ResourceKind`, `ResourceTransformRequest`,
  `ResourceTransformResponse`, and `ResourceTransform` model the callback as a
  function pointer plus context pointer, store it in private runtime state, and
  route native calls through a `callconv(.c)` trampoline that returns a status
  instead of letting Zig errors cross the C frame.
- 2026-05-14: Iteration 25 added public transform coverage that registers a
  transform before map creation, rejects re-registration after the runtime owns
  a live map, observes the original HTTP style URL and style resource kind, and
  rewrites the URL to a replacement copied by the native C API before the
  callback returns.
- 2026-05-14: Iteration 25 verification passed: `mise run //bindings/zig:test`
  (61/61 tests passed), `mise run fix`, then `mise run //bindings/zig:test`
  again (61/61 tests passed; expected native log lines appear during
  event/style/resource tests). Resource provider callbacks,
  `ResourceRequestHandle`, offline APIs, duplicate direct-C retirement, and
  Phase 7 review remain.
- 2026-05-14: Iteration 26 reflection:
  - Accomplished so far: Phases 1-6 are reviewed, committed, and pushed. Phase 7
    now has network status, ambient cache operations, logging callbacks,
    resource transforms, and the first resource-provider/request-handle slice in
    progress.
  - Working well: callback APIs are staying low-level and explicit: function
    pointer plus context pointer, private `callconv(.c)` trampolines, borrowed
    native inputs projected into semantic Zig views, and C status/decision
    returns instead of Zig errors crossing C frames.
  - Not working/blocking: provider and offline coverage is inherently broad;
    exact direct-C duplicate retirement should wait until the provider handle
    lifecycle and offline copied outputs have both been reviewed.
  - Approach adjustment: keep Phase 7 as a larger reviewed milestone, but still
    add it in coherent slices with narrow binding verification after each slice.
  - Next priorities: finish provider handle lifecycle coverage, add offline
    region copied value APIs, run the Phase 7 review, retire covered direct-C
    duplicates, then commit and push.
- 2026-05-14: Iteration 26 added public resource provider callbacks and
  `ResourceRequestHandle`: provider callbacks receive borrowed `ResourceRequest`
  views plus an optional handle, return semantic pass-through/handle decisions,
  can complete with a semantic `ResourceResponse`, check cancellation, and
  release the native request handle through a private opaque state wrapper.
- 2026-05-14: Iteration 26 added binding coverage for a custom URL style served
  through the resource provider, including observed request kind/loading/
  priority/usage/storage/range metadata, inline completion, handle release, and
  post-map provider registration rejection. Verification passed:
  `mise run //bindings/zig:test` (62/62 tests passed), `mise run fix`, then
  `mise run //bindings/zig:test` again (62/62 tests passed; expected native log
  lines appear during event/style/resource tests).
- 2026-05-14: Iteration 27 tightened `ResourceRequestHandle` lifecycle:
  successful completion marks the handle completed, second completion returns
  `error.AlreadyCompleted`, release is idempotent and closes the handle for
  later operations, pass-through destroys only the Zig wrapper without releasing
  the native handle, and copied handle values avoid dangling private state.
- 2026-05-14: Iteration 27 expanded resource-provider coverage for cancellation
  checks, duplicate completion rejection, duplicate release no-op behavior,
  after-release `error.ClosedHandle`, and public-boundary checks for resource
  request/response/handle types. Verification passed:
  `mise run
  //bindings/zig:test` (62/62 tests passed), `mise run fix`, then
  `mise run
  //bindings/zig:test` again (62/62 tests passed; expected native
  log lines appear during event/style/resource tests).
- 2026-05-14: Iteration 28 added copied offline region value APIs: public tile-
  pyramid and geometry definitions, owned offline region/list outputs, recursive
  `OwnedGeometry` copies, metadata copying, create/get/list/merge/
  metadata/status/observe/download/invalidate/delete runtime methods, and
  private materialization of offline C definitions through `TempStorage`.
- 2026-05-14: Iteration 28 ported public offline coverage for tile-pyramid
  region persistence, copied metadata lifecycle, list/get/delete behavior,
  inactive status snapshots, geometry region copied coordinate views, and
  public-boundary checks for offline types. Verification passed:
  `mise run
  //bindings/zig:test` (64/64 tests passed), `mise run fix`, then
  `mise run
  //bindings/zig:test` again (64/64 tests passed; expected native
  log lines appear during event/style/resource tests).
- 2026-05-14: Iteration 29 added custom geometry source binding APIs with
  semantic `CanonicalTileId`, nullable option fields, function-pointer/context
  callbacks, private `callconv(.c)` fetch/cancel trampolines, and map-owned
  callback state retained until map close so native worker callbacks cannot
  reach freed binding trampoline state.
- 2026-05-14: Iteration 29 ported custom geometry source coverage for adding a
  custom-vector source, setting tile GeoJSON, tile and region invalidation,
  duplicate source rejection, invalid zoom and tile validation, public-boundary
  checks, and private trampoline routing/cancel no-op behavior. Verification
  passed: `mise run //bindings/zig:test` (65/65 tests passed), `mise run fix`,
  then `mise run //bindings/zig:test` again (66/66 tests passed after adding
  private map-module tests; expected native log lines appear during event/style/
  resource tests).
- 2026-05-14: Iteration 30 expanded resource-provider test ports beyond inline
  completion: delayed request completion stores the semantic handle outside the
  callback, verifies observed prior metadata absence and request fields, rejects
  duplicate completion, and completes successfully after the callback returns.
- 2026-05-14: Iteration 30 added cross-thread public binding coverage for
  `ResourceRequestHandle.complete`, plus an error-response provider test that
  produces a public `map_loading_failed` event through `pollEventOwned`.
  Verification passed: `mise run //bindings/zig:test` (69/69 tests passed),
  `mise run fix`, then `mise run //bindings/zig:test` again (69/69 tests passed;
  expected native log lines appear during event/style/resource tests).
- 2026-05-14: Iteration 31 reflection:
  - Accomplished so far: Phases 1-6 are reviewed, committed, and pushed. Phase 7
    now has public APIs and tests for logging, network status, ambient cache
    operations, resource transforms, resource providers, delayed request
    handles, offline region copied values/events, and custom geometry source
    callbacks.
  - Working well: the callback boundary remains explicit and low-level: function
    pointers plus context pointers in public APIs, private `callconv(.c)`
    trampolines, and semantic borrowed/copying rules backed by public tests plus
    private module tests for trampoline internals.
  - Not working/blocking: Phase 7 is now broad and uncommitted, so the next
    substantial step should be review and duplicate-retirement rather than more
    API expansion. Some direct C resource tests still cover native transport
    paths and C ABI invalid-input behavior that the binding should not duplicate
    one-for-one.
  - Approach adjustment: finish only the remaining high-value public behavior
    ports, then run Phase 7 parallel review, apply safety findings, run root
    verification, retire exact duplicates, and commit/push the milestone.
  - Next priorities: finish provider cancellation/offline event coverage, run
    the Phase 7 review, then decide which direct C logging/resource/offline/
    custom-geometry assertions are now duplicate binding behavior.
- 2026-05-14: Iteration 32 ran Phase 7 parallel review `58e3504e` with two
  reviewers. Blocking findings included failed callback replacement mutating
  active Zig state, racy logging callback globals, borrowed callback input
  slices, `ResourceRequestHandle` wrapper lifetime/thread-safety gaps, and
  custom geometry active-upcall lifetime tracking.
- 2026-05-14: Iteration 32 applied the first Phase 7 review fixes: resource
  transform/provider state now updates only after native installation succeeds;
  failed post-map replacement keeps the previous callback active; logging now
  passes one callback state through native `user_data` instead of separate
  handler/context atomics and frees old state only after successful replacement
  or clear; logging, transform, and provider trampolines copy native borrowed
  strings/bytes into Zig-owned callback-duration storage before invoking user
  code.
- 2026-05-14: Iteration 32 added regressions that failed resource transform and
  provider replacement after map creation does not dispatch to the replacement
  state. Verification passed: `mise run //bindings/zig:test` (71/71 tests
  passed), `mise run fix`, then `mise run //bindings/zig:test` again (71/71
  tests passed; expected native log lines appear during event/style/resource
  tests). Remaining review blockers for the next iteration: request-handle state
  reclamation/thread-safety and custom-geometry active-upcall teardown.
- 2026-05-14: Iteration 33 applied more Phase 7 review fixes:
  `ResourceRequestHandle` operations now serialize with a binding-owned atomic
  lock, duplicate completion is marked before crossing into native code, release
  is still idempotent, and pass-through request wrappers are destroyed
  immediately because they are never exposed to public callers.
- 2026-05-14: Iteration 33 added custom-geometry active-upcall tracking:
  binding-owned source state carries retired and active callback counters,
  trampolines avoid invoking retired states, and map close retires source states
  then waits for active upcalls to finish before freeing the callback state.
  Private map-module tests now cover retired trampoline suppression.
  Verification passed: `mise run //bindings/zig:test` (71/71 tests passed),
  `mise run fix`, then `mise run //bindings/zig:test` again (71/71 tests passed;
  expected native log lines appear during event/style/resource tests). Remaining
  review follow-up: decide whether handled request wrapper reclamation can
  improve without breaking copied-handle `error.ClosedHandle` semantics, then
  re-review.
- 2026-05-14: Iteration 34 ran Phase 7 follow-up parallel review `6e729b64`.
  Reviewers confirmed the prior callback replacement, logging state, borrowed
  callback input copying, and custom-geometry active-upcall findings were
  resolved, but found a remaining blocking ABA hazard in the request-handle
  registry: released handle slots could be reused, allowing stale copied handles
  to alias later requests.
- 2026-05-14: Iteration 34 fixed the remaining request-handle reclamation
  blocker by making the registry tombstone released slots instead of reusing
  them. Released/completed request state is reclaimed, stale copied handles keep
  resolving to `error.ClosedHandle`, and later requests receive fresh registry
  IDs.
- 2026-05-14: Iteration 34 added a regression that releases a delayed provider
  handle, creates a later request, and verifies the stale copied handle cannot
  complete/cancel/release the later request. Verification passed after
  formatting: `mise run //bindings/zig:test` (72/72 tests passed; expected
  native log lines appear during event/style/resource tests).
- 2026-05-14: Iteration 35 retired exact duplicate direct C assertions now
  covered by Phase 7 binding tests: logging callback receive/mask/clear
  behavior, custom resource provider style load, cross-thread completion,
  cancellation-before-late-completion, error-response style failure, offline
  tile-pyramid and geometry copied-value lifecycle coverage, offline status
  event coverage, custom-geometry source helper behavior, and provider
  replacement rejection after map creation. Kept direct C coverage for file,
  asset, HTTP, PMTiles range metadata, ambient cache with real cache, offline
  invalid descriptor/input, offline error and merge behavior, native transport
  pass-through, and C ABI invalid-input behavior.
- 2026-05-14: Iteration 35 verification passed after duplicate retirement and
  formatting: `mise run fix`, `mise run //bindings/zig:test` (72/72 tests
  passed), and `mise run test` (92 passed, 12 skipped; 104 total).
- 2026-05-14: Iteration 35 committed the reviewed Phase 7 logging/resources/
  offline/callback milestone as `a726553`
  (`Add Zig resource callback
  bindings`) and pushed
  `zig-binding-implementation` to origin.
- 2026-05-14: Iteration 36 reflection:
  - Accomplished so far: Phases 1-7 are implemented, reviewed, verified,
    committed, and pushed. The binding now covers package setup,
    diagnostics/status, runtime/map lifecycle, semantic descriptors and copied
    outputs, owned runtime events, logging, resources, offline regions, and
    callback lifetimes through public Zig APIs.
  - Working well: milestone-sized review loops continue to catch concrete safety
    defects before commits. The semantic-descriptor/private-materializer pattern
    has scaled from map/style values into offline and render setup, and direct C
    duplicate retirement is staying conservative.
  - Not working/blocking: Phase 8 introduces platform/backend objects and frame
    lifetimes, so tests need to separate portable owned-texture/readback
    behavior from Metal/Vulkan platform scaffolding. Feature-state and query
    ports must wait until the render session slice has a stable public API.
  - Approach adjustment: start Phase 8 with portable owned-texture sessions,
    readback, and render-session lifetime coverage, then add backend-specific
    borrowed surface/texture attach APIs and owned frame handles in smaller
    reviewed slices.
  - Next priorities: add owned texture frame handles, port remaining render
    session maintenance coverage, then run the Phase 8 review before retiring
    matching direct C render assertions.
- 2026-05-14: Iteration 36 started Phase 8 with `bindings/zig/src/render.zig`:
  public `NativePointer`, semantic render-target extent and Metal/Vulkan
  surface/texture descriptor types, `RenderSessionHandle` with resize/render/
  detach/maintenance/close operations, owned texture attachment, caller-buffer
  readback, and allocator-backed `OwnedImage` readback.
- 2026-05-14: Iteration 36 added public render tests covering owned texture
  attachment validation, render update, readback metadata and owned bytes,
  resize, detach, idempotent close, and public-boundary checks for render types.
  Verification passed: `mise run //bindings/zig:test` (74/74 tests passed;
  expected native log lines appear during event/style/resource tests).
- 2026-05-14: Iteration 37 added backend-specific render attach APIs for Metal
  and Vulkan owned textures, borrowed textures, and surfaces. Public descriptors
  materialize C `size` fields and borrowed backend pointers internally while
  keeping raw native declarations private.
- 2026-05-14: Iteration 37 added scoped owned texture frame handles for Metal
  and Vulkan. Frame access returns semantic info values with borrowed
  `NativePointer`s valid until explicit frame release; release is routed through
  the owning render session and repeated release is a no-op.
- 2026-05-14: Iteration 37 extended public render coverage with renderer
  maintenance calls after render creation and a Metal owned-frame test covering
  acquire, native-pointer access, acquired-frame resize/close rejection,
  release, and closed-frame access. Verification passed:
  `mise run //bindings/zig:test` (75/75 tests passed; expected native log lines
  appear during event/style/ resource/render tests).
- 2026-05-14: Iteration 38 added semantic render-backend support reporting so
  callers can inspect Metal/Vulkan support without raw C backend bitmasks.
- 2026-05-14: Iteration 38 added render-session feature-state APIs:
  `FeatureStateSelector`, `RenderSessionHandle.setFeatureState`,
  `getFeatureState`, and `removeFeatureState`. Selectors and JSON state are
  materialized through private temporary C descriptors, and JSON snapshots are
  copied into `OwnedJsonValue` before native handles are destroyed.
- 2026-05-14: Iteration 38 ported public feature-state coverage for invalid
  pre-render state, set/get of object state, key removal, repaint/render update
  after removal, selector validation, and public-boundary checks. Verification
  passed: `mise run //bindings/zig:test` (77/77 tests passed; expected native
  log lines appear during event/style/resource/render tests).
- 2026-05-14: Iteration 39 added copied feature-query APIs for rendered and
  source features: semantic point/box/line query geometry, rendered/source query
  options, copied `FeatureQueryResult`, and copied `QueriedFeature` properties,
  geometry, optional source IDs, and feature state. Native query result handles
  are destroyed before returning.
- 2026-05-14: Iteration 39 ported rendered/source feature query coverage through
  public render sessions, including pre-render `error.InvalidState`, layer and
  filter options, copied source IDs/properties, source-feature filtering,
  invalid source IDs, and public-boundary checks. The shared binding style
  fixture now gives the point feature a stable ID and `kind` property used by
  feature-state/query tests. Verification passed: `mise run //bindings/zig:test`
  (78/78 tests passed; expected native log lines appear during event/style/
  resource/render tests).
- 2026-05-14: Iteration 40 added feature-extension query support:
  `OwnedFeature`, `OwnedFeatureCollection`, `FeatureExtensionResult`, and
  `RenderSessionHandle.queryFeatureExtension`. Feature-extension values and
  feature collections are copied before native extension result handles are
  destroyed.
- 2026-05-14: Iteration 40 ported cluster feature-extension coverage through
  public render sessions: rendered cluster query, borrowed conversion of the
  copied queried feature into an extension input, `children`, `expansion-zoom`,
  and limited `leaves` extension queries. Verification passed:
  `mise run //bindings/zig:test` (79/79 tests passed; expected native log lines
  appear during event/style/resource/render tests).
- 2026-05-14: Iteration 41 reflection:
  - Accomplished so far: Phases 1-7 are reviewed, verified, committed, and
    pushed. Phase 8 now covers render sessions, readback, backend descriptors,
    owned texture frames, feature state, rendered/source queries, and cluster
    feature-extension queries through public binding APIs.
  - Working well: semantic public descriptors plus private C materialization are
    still keeping raw `mln_*` types out of the package root, and the review loop
    is catching the exact safety edges this phase is meant to harden.
  - Not working/blocking: Phase 8 remains uncommitted until review findings,
    root verification, and direct-C render duplicate-retirement decisions are
    complete. Backend-specific tests need build-option gates so non-Metal
    variants stay portable.
  - Approach adjustment: finish review fixes before adding new Phase 8 API
    surface, then run root verification and retire only exact duplicate direct C
    render/query/feature-state assertions.
  - Next priorities: complete Phase 8 duplicate-retirement/root verification,
    commit and push the render milestone, then start Phase 9 examples/docs.
- 2026-05-14: Iteration 41 ran the Phase 8 parallel review retry `38ebc2f9`
  after the first retry `670862f2` failed with no usable output. Applied review
  findings: owned texture frame handles now store active frame state on the
  render session instead of allocating one heap object per acquire/release,
  copied feature properties free the copied key if JSON value copying fails,
  binding tests receive backend build options, the Metal owned-frame test skips
  on non-Metal variants, and backend support coverage no longer assumes Metal
  and Vulkan support are mutually exclusive.
- 2026-05-14: Iteration 41 verification after review fixes passed:
  `mise run
  fix` and `mise run //bindings/zig:test` (79/79 tests passed;
  expected native log lines appear during event/style/resource/render tests).
- 2026-05-14: Iteration 42 root verification before direct-C duplicate
  retirement passed: `mise run test` (92 passed, 12 skipped; 104 total).
- 2026-05-14: Iteration 42 retired exact duplicate direct C render assertions
  now covered by Phase 8 binding tests: owned-texture extent validation,
  lifecycle/render-update/resize/detach/readback behavior, feature-state
  set/get/remove behavior, rendered/source feature queries, and cluster feature
  extension queries. Kept direct C coverage for C ABI struct defaults/imports,
  null/out-param/undersized raw descriptors, raw query validation, backend-
  specific Metal/Vulkan surface and borrowed-texture scaffolding, raw stale
  native handle behavior, wrong-thread render calls, observer/still-image paths,
  and platform transport details not yet usefully portable through the public
  binding tests.
- 2026-05-14: Iteration 42 final Phase 8 verification after duplicate retirement
  passed: `mise run fix`, `mise run //bindings/zig:test` (79/79 tests passed),
  and `mise run test` (87 passed, 12 skipped; 99 total). Phase 8 was committed
  and pushed as the reviewed render-target/readback milestone.
- 2026-05-14: Iteration 43 started Phase 9 examples by porting
  `examples/zig-readback` from direct `@cImport`/raw C calls to the public
  `maplibre_native` Zig binding. The example now depends on `bindings/zig` as a
  local package, creates runtime/map/render-session handles through public APIs,
  uses semantic camera/render/readback values, writes the copied owned image to
  PPM, and removed its example-local `c.zig` MapLibre C import.
- 2026-05-14: Iteration 43 made the binding package's build `RenderBackend` enum
  public so downstream build scripts can use the `linkMaplibreNativeC` helper or
  dependency options with a stable type. Verification passed: `mise run fix`,
  `mise run //bindings/zig:test` (79/79 tests passed),
  `mise run //examples/zig-readback:build`, and a bounded
  `mise run //examples/zig-readback:run` completed in about 3.5s and wrote
  `examples/zig-readback/zig-out/macos-arm64-metal/zig-readback.ppm`.
- 2026-05-14: Iteration 44 ported `examples/zig-map` from raw MapLibre C calls
  to the public `maplibre_native` binding. The example now depends on the local
  binding package, uses public runtime/map/render-session handles in its app
  state, drives camera commands through semantic binding methods, drains binding
  runtime events, and attaches Metal/Vulkan render targets through public
  descriptor APIs. The example-local `c.zig` now imports SDL/Metal/Vulkan only;
  `rg "mln_|logAbiError" examples/zig-map` returns no MapLibre raw-C usage.
- 2026-05-14: Iteration 44 verification passed after the Zig map port:
  `mise run fix`, `mise run //bindings/zig:test` (79/79 tests passed),
  `mise run //examples/zig-readback:build`, and
  `mise run //examples/zig-map:build`. A bounded
  `mise run //examples/zig-map:run:owned-texture` launched the GUI example,
  printed controls/backend information, processed resize events, and was stopped
  after 8s as expected for a GUI app.
- 2026-05-14: Iteration 45 updated contributor documentation for the Zig binding
  workflow: `docs/src/content/docs/development/overview.md` now lists focused
  binding/example commands and clarifies root test coverage, and
  `docs/src/content/docs/development/bindings-zig.md` now includes a build/test
  guide for the binding package and examples. Audience/category: contributors,
  how-to guide.
- 2026-05-14: Iteration 45 integrated the Zig binding suite into root
  `mise run test` by running the binding package tests after the direct Zig C
  API suite with the same native artifact directory and render-backend variant.
- 2026-05-14: Iteration 45 updated `.github/config/variants.toml` so CI runs
  `//bindings/zig:ci` on Linux/macOS Metal/Vulkan-supported variants and runs a
  Windows Zig binding build-only check while native runtime tests remain
  excluded on Windows. Matrix previews passed for
  `mise run ci:matrix bindings
  --pretty` and
  `mise run ci:matrix examples --pretty`.
- 2026-05-14: Iteration 45 verification passed: `mise run test` now runs the
  direct C suite (87 passed, 12 skipped; 99 total) and then the Zig binding
  suite (79/79 passed), followed by `mise run fix` with all checks passing and a
  second `mise run test` with the same counts after formatting.
- 2026-05-14: Iteration 46 reflection:
  - Accomplished so far: Phases 1-8 are complete, reviewed, verified, committed,
    and pushed. Phase 9 has ported both Zig examples to the public binding,
    added contributor/build documentation, integrated the binding suite into
    root `mise run test`, and added Zig binding CI matrix entries with an
    explicit Windows build-only path.
  - Working well: the public binding is now the normal Zig consumption path for
    tests, examples, and root verification. Parallel reviews continue to find
    real integration issues that local host builds can miss, especially package
    consumption from outside the repository.
  - Not working/blocking: the Phase 9 diff is reviewed but not committed yet.
    One reviewer found that forwarding relative `cmake-artifact-dir` strings to
    the binding dependency resolved them relative to `bindings/zig`, which would
    break external Zig consumers with relative artifact paths.
  - Approach adjustment: finish Phase 9 by tightening package-consumer path
    handling and recording the external-consumer smoke test, then rerun the
    root/example verification and commit/push the milestone before the final
    definition-of-done audit.
  - Next priorities: finish applying Phase 9 review findings, run final Phase 9
    verification, commit and push the example/docs/integration milestone, then
    perform a final public-boundary/full-verification pass.
- 2026-05-14: Iteration 46 ran Phase 9 parallel review `ff87536b` with two
  reviewers. Reviewer 0 found no blockers and confirmed the examples use public
  MapLibre binding APIs while keeping `@cImport` to SDL/Metal/Vulkan host
  integration. Reviewer 1 found a package-consumption bug: relative
  `cmake-artifact-dir` values forwarded to the `maplibre_native` dependency were
  interpreted relative to `bindings/zig` instead of the consuming build.
- 2026-05-14: Iteration 46 applied the package-consumption review finding by
  changing `cmake-artifact-dir` handling in `bindings/zig/build.zig`,
  `examples/zig-readback/build.zig`, and `examples/zig-map/build.zig` from raw
  string paths to `std.Build.LazyPath` options. Example build scripts now pass a
  LazyPath to the binding dependency so relative paths stay anchored to the
  consumer build root; the binding still resolves a runtime path string for
  Windows test `PATH` setup.
- 2026-05-14: Iteration 46 kept the examples small/focused after review:
  `rg "mln_|maplibre_native_c|logAbiError" examples/zig-readback examples/zig-map`
  finds no raw MapLibre C usage, while the remaining example-local `@cImport`
  imports only host integration headers in `examples/zig-map/c.zig`. Line-count
  spot check: `zig-readback/main.zig` is 136 lines, and the larger `zig-map`
  files stay focused on SDL input/windowing plus Metal/Vulkan host render setup.
- 2026-05-14: Iteration 46 verification after review fixes passed:
  `mise run //bindings/zig:test` (79/79 passed),
  `mise run //examples/zig-readback:build`, `mise run //examples/zig-map:build`,
  `mise run fix`, and `mise run test` (direct C suite 87 passed, 12 skipped; Zig
  binding suite 79/79 passed). A temporary external Zig consumer at
  `/tmp/mln-zig-consumer.cQxEls` also built successfully while passing
  `b.path("relative-artifacts")` as the binding dependency's
  `cmake-artifact-dir`, covering the relative-path review finding. One parallel
  `ensure-native-library` run failed due a concurrent CMake/Ninja regeneration
  race; rerunning the Zig map build sequentially passed.
- 2026-05-14: Iteration 31 added public resource-provider cancellation coverage:
  a delayed request survives callback return, the map closes before completion,
  `cancelled()` becomes true, and late completion reports `error.InvalidState`.
- 2026-05-14: Iteration 31 added public offline download-control event coverage:
  invalid observe/download commands surface `error.InvalidArgument`, observing a
  region and activating download produces an owned
  `offline_region_status_changed` event with copied status payload data. Narrow
  verification passed: `mise run //bindings/zig:test` (71/71 tests passed;
  expected native log lines appear during event/style/resource tests).

## Review log

- Setup milestone: no code implementation diff to review. First parallel
  implementation review is required before the Phase 1 commit.
- Phase 1 parallel review run `5e4ffacb` completed with two fresh-context
  reviewers. Findings applied: default `zig build` no-op, consumer-unsafe link
  helper paths, multi-config mise native library dir handling, unsupported
  target/backend clarity, and raw public render-backend bitmask exposure.
  Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/5e4ffacb_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/5e4ffacb_reviewer_1_output.md`.
- Phase 2 parallel review run `f0c39965` completed with one completed reviewer
  and one transport-failed reviewer that still returned usable output. Findings
  applied: avoid `DiagnosticStore.get()` returning an owned-looking value with a
  public deinit path, and strengthen copied-diagnostics coverage with a later
  real C API call. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/f0c39965_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/f0c39965_reviewer_1_output.md`.
- Phase 3 parallel review run `22251894` completed with two reviewers. Findings
  applied or re-reviewed: borrowed event message exposure, public mutable
  `?*anyopaque` handle storage, wrong-thread diagnostic coverage, close-retry
  coverage, and closed-handle/string-validation ordering. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/22251894_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/22251894_reviewer_1_output.md`.
- Phase 3 follow-up review run `373e331f` completed with two reviewers. It
  confirmed event memory safety, wrong-thread coverage, and close-retry
  behavior; the remaining public `?*anyopaque` handle-storage blocker was fixed
  afterwards by moving native handles into private allocated state behind opaque
  state pointers. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/373e331f_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/373e331f_reviewer_1_output.md`.
- Phase 4 parallel review run `2fec76ed` completed with two reviewers. Findings
  applied: add wrong-thread runtime destroy/poll coverage, add runtime pump
  after map destruction, add repaint render-update event coverage, and clarify
  that direct C tests remain justified for C ABI-only invalid-input and raw
  stale-handle assertions. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/2fec76ed_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/2fec76ed_reviewer_1_output.md`.
- Phase 5 parallel review run `9679e64a` completed with two reviewers. Findings
  applied: raw C leakage through `OwnedJsonValue.copyFromNative`, dangling
  nested geometry coordinate spans from `ArrayList` reallocation, empty native
  JSON array/object copy safety, and copied-attribution exact allocation
  ownership. Findings deferred for a later slice: diagnostic-aware projection
  free-helper overloads and preserving unknown raw `StyleSourceType` values;
  those are not blockers for the current copied-result/descriptor milestone and
  will be revisited with broader output-domain/API polish. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/9679e64a_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/9679e64a_reviewer_1_output.md`.
- Phase 6 parallel review run `29c7eea9` completed with two reviewers. Findings
  applied: exercise event-copy invalidation with a later poll, mutate borrowed
  payload backing storage after private copies to prove ownership, validate
  native payload alignment and leading size before casting, add malformed
  payload tests for null/undersized/misaligned inputs, and replace a public
  source-ID force unwrap with a clear test error. Deferred: public
  payload-bearing event lifetime coverage waits for later render/offline/
  callback APIs that can produce those events through public binding APIs.
  Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/29c7eea9_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/29c7eea9_reviewer_1_output.md`.
- Phase 7 parallel review run `58e3504e` completed with two reviewers. Findings
  applied across iterations 32-33: failed resource callback replacement no
  longer mutates active Zig state, logging no longer uses racy split globals,
  borrowed native callback strings/bytes are copied before invoking Zig user
  callbacks, request-handle operations are serialized, pass-through request
  wrappers are destroyed immediately, and custom-geometry callback state waits
  for active upcalls before freeing. Remaining follow-up: decide whether handled
  request wrapper reclamation can improve without breaking copied-handle
  `error.ClosedHandle` semantics. Direct-C duplicate retirement is deferred
  until that follow-up is settled. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/58e3504e_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/58e3504e_reviewer_1_output.md`.
- Phase 7 follow-up review run `6e729b64` completed with two reviewers. Findings
  applied: request-handle registry slot reuse created an ABA hazard for stale
  copied handles after release. The registry now tombstones released slots
  instead of reusing them, reclaims request state on release/completion wrapper
  destruction, and has a public regression proving stale handles stay closed
  after a later request is created. Non-blocking follow-up from reviewers: a
  stronger custom-geometry in-flight close stress test could add confidence, but
  the structural active-upcall lifetime blocker is resolved. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/6e729b64_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/6e729b64_reviewer_1_output.md`.
- Phase 8 first review attempt `670862f2` failed with no usable reviewer output,
  so it was not used for milestone decisions. Retry review run `38ebc2f9`
  completed with two reviewers. Findings applied: avoid per-frame heap leaks in
  owned texture frame handles, free copied feature property keys if JSON value
  copying fails, gate Metal-only frame tests with binding build options, and
  remove the brittle assumption that Metal and Vulkan support bits are mutually
  exclusive. Duplicate direct-C render retirement remains deferred until root
  verification and exact-coverage comparison are complete. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/38ebc2f9_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/38ebc2f9_reviewer_1_output.md`.
- Phase 9 parallel review run `ff87536b` completed with two reviewers. Findings
  applied: `cmake-artifact-dir` forwarding to the `maplibre_native` Zig
  dependency now uses `std.Build.LazyPath` so external consumers can pass
  relative artifact paths anchored to their build roots. Findings deferred:
  examples could thread `DiagnosticStore` through more operations for richer
  native messages, but the current examples intentionally keep diagnostics thin
  and focused while demonstrating binding API usage; revisit if examples gain
  error-handling guidance. Artifacts:
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/ff87536b_reviewer_0_output.md`
  and
  `/Users/sargunv/.pi/agent/sessions/--Users-sargunv-Code-maplibre-native-ffi--/subagent-artifacts/ff87536b_reviewer_1_output.md`.

## Notes and decisions

- Keep direct Zig C tests until equivalent public-binding assertions land.
- Prefer narrow binding test commands early; integrate root `mise run test`
  after the binding suite has a supported path.
- Commit and push every completed phase or meaningful milestone so the branch
  always carries the latest Ralph artifact and implementation state.
- Phase 1 uses a tiny public root (`cAbiVersion`) to prove native linking while
  keeping the raw `@cImport` private in `src/c.zig` and out of the package root.
- Phase 2 keeps raw C status handling in private `status.zig`; the public root
  exposes stable Zig error sets, diagnostics, and ABI validation without
  exposing raw C declarations.
- Phase 3 handle structs store only pointers to private opaque state handles in
  the public type. Native `mln_*` pointers and diagnostic store references live
  in private implementation structs inside the module.
- Phase 3 keeps cast-to-C helpers in internal modules and does not re-export
  those modules from the package root; public methods operate through Zig handle
  types and stable binding errors.
- Phase 3 currently keeps private handle-state allocations alive after close so
  copied handle values can make repeated `close()` calls a no-op and later
  method calls return `error.ClosedHandle`. Revisit state reclamation later if a
  better Zig ownership shape preserves those semantics without leaks.
- Phase 4 foundational binding tests cover public binding behavior now exposed
  by Phases 2 and 3. Direct C tests remain for C ABI-only validation and raw
  stale-handle coverage after duplicate native-behavior assertions are retired.
- Phase 5 uses nullable fields on public option structs to represent C field
  masks semantically. The binding materializes `size` and `fields` internally
  before C calls so callers never construct raw C option structs.
- Public Zig enums represent known C enum domains for map tuning. Snapshot
  conversion treats unknown native enum values as `error.UnknownStatus` so the
  binding fails explicitly rather than preserving a raw C value in public data.
- Phase 5 copies native snapshot/list handles into Zig-owned values immediately
  and destroys native result handles before returning. Callers deinitialize the
  copied outputs with `OwnedJsonValue.deinit()` and `StringList.deinit()`.
- Geometry and GeoJSON inputs use public recursive descriptors. The binding
  materializes the corresponding nested C graph in temporary arena storage for
  each call, preserving the C API's borrowed-for-call ownership model.
- Phase 6 keeps `pollEvent()` as the no-allocation event API and adds
  `pollEventOwned(allocator)` for callers that need copied borrowed data such as
  native event messages. `MapId` is binding-assigned and stable only while a map
  is live with its runtime.
- Phase 7 callback APIs start with function pointers plus context pointers. The
  binding-owned trampoline remains private and translates native borrowed inputs
  into Zig semantic request/record values for the callback duration.
- Resource provider callbacks allocate a private request-handle wrapper only
  when native supplies a handle. Returning pass-through destroys the wrapper
  without releasing the native handle; returning handle transfers release
  responsibility to the caller-facing `ResourceRequestHandle`.
- Resource request handle state is reclaimed on release or internal wrapper
  destruction. The registry tombstones released slots instead of reusing them so
  stale copied handle values keep reporting `error.ClosedHandle` and cannot
  alias later native requests.
- Offline region snapshot/list handles are copied immediately into Zig-owned
  `OwnedOfflineRegion` values and destroyed before returning. Region style URLs,
  geometry coordinates, recursive geometry collections, and metadata are caller-
  owned and released through `deinit` methods.
- Custom geometry source callbacks use a binding-owned per-source state object
  as native `user_data`; map close frees those state objects after native map
  destruction succeeds. Source removal and style replacement may retire the
  native source earlier, but the binding keeps callback state alive until map
  close to prefer safety over eager reclamation.
- Resource provider tests now cover both inline completion and delayed handle
  ownership. A provider may retain the semantic `ResourceRequestHandle` when it
  returns `.handle`; the caller remains responsible for completing or releasing
  that handle exactly once from any valid thread. Request handle operations use
  a binding-owned atomic lock so complete/cancel/release do not race with each
  other.
- Callback replacement follows the binding convention that native installation
  succeeds before active Zig state changes. On failed replacement, the previous
  resource/logging callback remains the active dispatch target.
- Phase 8 render target descriptors expose borrowed backend addresses as
  `NativePointer` values. The binding materializes native descriptor sizes and
  option graphs internally so Metal/Vulkan handles remain opaque at the public
  API boundary.
- Owned texture frame handles scope borrowed backend texture/image pointers to
  the acquired frame lifetime. Release goes through the owning render session;
  after release, public frame access reports `error.ClosedHandle`.
- Render-session feature-state snapshots are copied into `OwnedJsonValue` before
  native JSON snapshot handles are destroyed. Feature-state selectors use
  nullable public fields for optional source-layer, feature, and state-key
  fields instead of exposing C field masks.
- Rendered/source feature query results are copied before native result handles
  are destroyed. Query options expose semantic optional fields and JSON filters;
  the binding materializes C string-view arrays and filters in temporary
  borrowed storage for each call.
- Feature-extension query results use copied `OwnedJsonValue` or
  `OwnedFeatureCollection` values before native extension result handles are
  destroyed. Extension inputs currently take semantic `Feature` descriptors; a
  caller can build one from a copied query result when chaining rendered cluster
  queries into extension queries.
- Owned texture frame handles use render-session-owned active frame slots with
  binding generations instead of per-acquire heap state. Release clears the
  active slot, repeated release is a no-op, and stale copied frame handles see
  `error.ClosedHandle` after the slot generation changes or is cleared.
- The Zig examples should use the public binding for MapLibre APIs. They may
  still import host-platform libraries such as SDL, Metal, Objective-C, and
  Vulkan directly because those are application integration dependencies rather
  than MapLibre C ABI declarations.
