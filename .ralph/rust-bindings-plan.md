# Rust binding architecture plan

Work through `bindings/rust/PLAN.md`, starting with Milestone 1 and progressing
as far as practical up to but not including Milestone 10 while keeping the
workspace buildable.

Conduct a parallel review round at each milestone, and apply any findings that
make sense. Commit and push after each review round, and proceed to the next
milestone until complete (through 9).

## Goals

- Establish the Rust crate boundary described in the plan.
- Move shared ABI adaptation from the public Rust crate into the shared core
  crate incrementally.
- Keep public Rust ergonomics intact through re-exports and thin wrappers.
- Verify each completed slice with Rust build/test commands when practical.

## Checklist

- [x] Milestone 1: Rename `maplibre-native-support` to `maplibre-native-core`.
- [x] Milestone 1: Move current support helpers into modules inside `core` and
      update dependents.
- [x] Milestone 1: Document crate roles in crate roots.
- [x] Milestone 1: Make `maplibre-native` depend on `core` for existing support
      helpers.
- [x] Milestone 1: Keep `sys` generated-only and document that boundary.
- [x] Milestone 2: Move scalar copied values into `core` and re-export them from
      `maplibre-native`.
- [x] Milestone 2: Move closed enum raw conversions and unknown preservation
      into `core` where practical. Moved `NetworkStatus`, `LogSeverity`,
      `LogEvent`, resource request/response enums, runtime event enums, map
      option enums, style source/tile enums, `AmbientCacheOperation`, and raw
      bitmask domains (`RenderBackendMask`, `LogSeverityMask`,
      `MapDebugOptions`).
- [x] Milestone 2: Add core unit tests for raw mapping, unknown preservation,
      and round trips.
- [x] Milestone 3: Move JSON value/member types, materializers, and readers into
      `core`. Moved `JsonValue`, `JsonMember`, `NativeJsonValue`,
      `NativeJsonMembers`, and JSON copy helpers.
- [x] Milestone 3: Move geometry value types, materializers, and readers.
- [x] Milestone 3: Move GeoJSON feature and feature-collection materializers and
      readers.
- [x] Milestone 3: Preserve depth checks, integer width, object member order,
      duplicate keys, copied-output ownership, and finite-number validation.
- [x] Milestone 4: Move map creation/options descriptor materializer into
      `core`.
- [x] Milestone 4: Move map viewport descriptor materializer/reader into `core`.
- [x] Milestone 4: Move map tile descriptor materializer/reader into `core`.
- [x] Milestone 4: Move camera, animation, and camera-fit descriptor
      materializers into `core`.
- [x] Milestone 4: Move camera bounds, free-camera, and projection descriptor
      materializers/readers into `core`.
- [x] Milestone 4: Move runtime options descriptor materializer into `core`.
- [x] Milestone 4: Move offline-region definition descriptor materializers into
      `core`.
- [x] Milestone 5: Start offline-region native reader extraction by moving
      `OfflineRegionInfo` and definition copying into `core`.
- [x] Milestone 5: Move JSON snapshot reader/guard ownership into `core`.
- [x] Milestone 5: Move style ID list reader/guard ownership into `core`.
- [x] Milestone 5: Move offline-region snapshot/list reader guard ownership into
      `core`.
- [x] Milestone 5: Move feature query result reader/guard ownership into `core`.
- [x] Milestone 5: Move feature-extension result reader/guard ownership into
      `core`.
- [x] Milestone 5: Add copy failure-path coverage for feature-extension
      collection pointer validation.
- [x] Milestone 5: Mark owned native-result reader helpers unsafe and document
      handle ownership/liveness preconditions.
- [x] Milestone 5: Add offline-region copy failure-path coverage for nonempty
      null metadata and null geometry pointers.
- [x] Milestone 5: Complete parallel review and apply relevant findings.
- [x] Milestone 4: Move render-target descriptor materializers into `core`.
- [x] Milestone 4: Move feature-state selector descriptor materializer into
      `core`.
- [x] Milestone 4: Move rendered-query geometry descriptor materializer into
      `core`.
- [x] Milestone 4: Move rendered/source feature query options descriptor
      materializers into `core`.
- [x] Milestone 4: Move tile-source options descriptor materializer into `core`.
- [x] Milestone 4: Move style-image options descriptor materializer into `core`.
- [x] Milestone 4: Finish remaining style descriptor/materializer audit.
- [x] Milestone 4: Complete parallel review and apply relevant findings.
- [x] Milestone 6: Move runtime event raw copying into `core` with raw source
      type/address preservation.
- [x] Milestone 6: Keep Rust `MapId` source lookup policy in `maplibre-native`
      while re-exporting copied payload types.
- [x] Milestone 6: Move log record copying and raw severity/event mapping into
      `core` while keeping callback invocation policy in `maplibre-native`.
- [x] Milestone 6: Complete parallel review and apply relevant findings.
- [x] Milestone 7: Move resource request copying into `core`.
- [x] Milestone 7: Move resource response materialization into `core`.
- [x] Milestone 7: Move resource transform request copying into `core`.
- [x] Milestone 7: Move internal request-handle state machine into `core`.
- [x] Milestone 7: Provide bridge-neutral complete/cancelled/release/provider
      decision finalization primitives in `core`.
- [x] Milestone 7: Keep Rust callback ergonomics, panic handling, replacement
      URL retention, and `!Sync` handle wrapper in `maplibre-native`.
- [x] Milestone 7: Complete parallel review and apply relevant findings.
- [x] Milestone 8: Add explicit close-once native handle state in `core`.
- [x] Milestone 8: Support status-returning and infallible destroy functions in
      core native handle state.
- [x] Milestone 8: Support leak-report-only finalizer paths in core native
      handle state.
- [x] Milestone 8: Update public Rust thread-affine handles to wrap core handle
      state while preserving `!Send` policy and owner-thread `Drop` behavior.
- [x] Milestone 8: Complete parallel review and apply relevant findings.
- [ ] Milestone 9: Reshape the public Rust crate around `core` in small
      buildable slices.

## Verification

- `mise run -C bindings/rust test` — passed after Milestone 1 (93
  `maplibre-native`, 14 `maplibre-native-core`, 0 `maplibre-native-sys`, doc
  tests passed).
- `mise run -C bindings/rust test` — passed after moving scalar copied values
  and fixing review findings (93 `maplibre-native`, 22 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed.
- `mise run -C bindings/rust test` — passed after moving `NetworkStatus` raw
  mapping into core (93 `maplibre-native`, 24 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed.
- `mise run -C bindings/rust test` — passed after moving log severity/event raw
  mapping into core (93 `maplibre-native`, 26 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed.
- `mise run -C bindings/rust test` — passed after moving resource request enum
  raw mapping into core (93 `maplibre-native`, 27 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed.
- `mise run -C bindings/rust test` — passed after moving runtime event enum raw
  mapping into core (93 `maplibre-native`, 30 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed.
- `mise run -C bindings/rust test` — passed after moving map/style/ambient enum
  domains and bitmask domains into core (93 `maplibre-native`, 34
  `maplibre-native-core`, 0 `maplibre-native-sys`, doc tests passed).
- `git commit -m "Refactor Rust binding core boundary"` — created `2f1b7ee`.
- `git push` — pushed branch `rust-refactor` to `origin`.
- `cargo fmt --all --manifest-path Cargo.toml` — passed.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed.
- `mise run -C bindings/rust test` — passed after moving copied image metadata
  values into core (93 `maplibre-native`, 37 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `git commit -m "Move Rust image value types into core"` — created `5e55ab8`.
- `git push` — pushed `5e55ab8` to `origin/rust-refactor`.
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving JSON into
  core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving JSON into core.
- `mise run -C bindings/rust test` — passed after moving JSON into core (90
  `maplibre-native`, 40 `maplibre-native-core`, 0 `maplibre-native-sys`, doc
  tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving geometry
  into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving geometry into core.
- `mise run -C bindings/rust test` — passed after moving geometry into core (88
  `maplibre-native`, 42 `maplibre-native-core`, 0 `maplibre-native-sys`, doc
  tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after applying Milestone
  3 review findings.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after applying Milestone 3 review findings.
- `mise run -C bindings/rust test` — passed after applying Milestone 3 review
  findings (85 `maplibre-native`, 49 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `git commit -m "Address Rust descriptor core review findings"` — created
  `354e26b`.
- `git push` — pushed `354e26b` to `origin/rust-refactor`.
- `git commit -m "Update Rust bindings plan progress"` — created `a9ca868`.
- `git push` — pushed `a9ca868` to `origin/rust-refactor`.
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving map option
  descriptors into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving map option descriptors into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving map option descriptors into core.
- `mise run -C bindings/rust test` — passed after moving map option descriptors
  into core (85 `maplibre-native`, 52 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving camera
  descriptors into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving camera descriptors into core.
- `mise run -C bindings/rust test` — passed after moving camera descriptors into
  core (85 `maplibre-native`, 55 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving runtime
  options into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving runtime options into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving runtime options into core.
- `mise run -C bindings/rust test` — passed after moving runtime options into
  core (85 `maplibre-native`, 57 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving
  offline-region descriptors/readers into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving offline-region descriptors/readers into core.
- `mise run -C bindings/rust test` — passed after moving offline-region
  descriptors/readers into core (85 `maplibre-native`, 60
  `maplibre-native-core`, 0 `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving
  render-target descriptors into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving render-target descriptors into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving render-target descriptors into core.
- `mise run -C bindings/rust test` — passed after moving render-target
  descriptors into core (85 `maplibre-native`, 62 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving query
  descriptors into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving query descriptors into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving query descriptors into core.
- `mise run -C bindings/rust test` — passed after moving query descriptors into
  core (85 `maplibre-native`, 65 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving style
  option descriptors into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving style option descriptors into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving style option descriptors into core.
- `mise run -C bindings/rust test` — passed after moving style option
  descriptors into core (85 `maplibre-native`, 67 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- Parallel Milestone 4 review — no blockers or required fixes found. Reviewers
  confirmed public re-exports/private raw traits, core ownership of descriptor
  materializers, backing-storage lifetimes, and validation coverage. The only
  remaining public descriptor builder noted was custom geometry source options,
  which stays above `core` because it owns callback trampolines and Rust
  `user_data` policy.
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving JSON
  snapshot, style ID list, and offline-region snapshot/list readers into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving the first Milestone 5 result readers into core.
- `mise run -C bindings/rust test` — passed after moving the first Milestone 5
  result readers into core (85 `maplibre-native`, 67 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving feature
  query and feature-extension result readers into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving feature query and feature-extension result readers into
  core.
- `mise run -C bindings/rust test` — passed after moving feature query and
  feature-extension result readers into core (85 `maplibre-native`, 68
  `maplibre-native-core`, 0 `maplibre-native-sys`, doc tests passed).
- Parallel Milestone 5 review — reviewers confirmed result-reader ownership was
  centralized in core and public APIs still return copied Rust values. Applied
  the blocker/medium finding by making owned native-result reader helpers
  `unsafe fn` with safety docs and explicit unsafe call-site comments. Applied
  coverage suggestions for offline-region metadata/geometry copy failures.
- `cargo fmt --all --manifest-path Cargo.toml` — passed after Milestone 5 review
  fixes.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after Milestone 5 review fixes.
- `mise run -C bindings/rust test` — passed after Milestone 5 review fixes (85
  `maplibre-native`, 70 `maplibre-native-core`, 0 `maplibre-native-sys`, doc
  tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving runtime
  event and log record copying into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving runtime event and log record copying into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving runtime event and log record copying into core.
- `mise run -C bindings/rust test` — passed after moving runtime event and log
  record copying into core (83 `maplibre-native`, 76 `maplibre-native-core`, 0
  `maplibre-native-sys`, doc tests passed).
- Parallel Milestone 6 review — reviewers confirmed event/log copying moved to
  core, raw event source data is preserved, and Rust `MapId` plus logging
  callback policy remains in `maplibre-native`. Applied findings by making
  `runtime_event_from_native` unsafe with safety docs, removing the public
  inherent raw `OfflineRegionStatus::from_native` method, adding non-null raw
  source address coverage, and adding invalid UTF-8 log callback fallback
  coverage.
- `cargo fmt --all --manifest-path Cargo.toml` — passed after Milestone 6 review
  fixes.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after Milestone 6 review fixes.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after Milestone 6 review fixes.
- `mise run -C bindings/rust test` — passed after Milestone 6 review fixes (84
  `maplibre-native`, 76 `maplibre-native-core`, 0 `maplibre-native-sys`, doc
  tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving resource
  request/response/transform primitives into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving resource request/response/transform primitives into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving resource request/response/transform primitives into
  core.
- `mise run -C bindings/rust test` — passed after moving resource
  request/response/transform primitives into core (84 `maplibre-native`, 80
  `maplibre-native-core`, 0 `maplibre-native-sys`, doc tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after moving the
  resource request-handle state machine into core.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after moving the resource request-handle state machine into core.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after moving the resource request-handle state machine into core.
- `mise run -C bindings/rust test` — passed after moving the resource
  request-handle state machine into core (84 `maplibre-native`, 80
  `maplibre-native-core`, 0 `maplibre-native-sys`, doc tests passed).
- Parallel Milestone 7 review — reviewers confirmed request/response/transform
  primitives and the request-handle state machine moved cleanly into core while
  Rust callback/user-data/panic policy remains in `maplibre-native`. Applied
  findings by making provider-decision finalization idempotent for exposed core
  users, tying `NativeResourceResponse` to the borrowed `ResourceResponse`
  lifetime, and adding tests for idempotent finalization, double successful
  completion rejection, and nonempty response bytes.
- `cargo fmt --all --manifest-path Cargo.toml` — passed after Milestone 7 review
  fixes.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after Milestone 7 review fixes.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after Milestone 7 review fixes.
- `mise run -C bindings/rust test` — passed after Milestone 7 review fixes (84
  `maplibre-native`, 83 `maplibre-native-core`, 0 `maplibre-native-sys`, doc
  tests passed).
- `cargo fmt --all --manifest-path Cargo.toml` — passed after adding core native
  handle state and wrapping Rust thread-affine handles around it.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after adding core native handle state and wrapping Rust thread-affine handles
  around it.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after adding core native handle state and wrapping Rust thread-affine
  handles around it.
- `mise run -C bindings/rust test` — passed after adding core native handle
  state and wrapping Rust thread-affine handles around it (84 `maplibre-native`,
  85 `maplibre-native-core`, 0 `maplibre-native-sys`, doc tests passed).
- Parallel Milestone 8 review — reviewers confirmed core centralizes close-once
  handle mechanics and public Rust keeps `!Send` policy, then flagged unsafe
  destroy-function preconditions and bridge-storage auto-traits. Applied fixes
  by making close methods unsafe with safety docs, storing handle addresses as
  integers so core state is `Send`, documenting leak-report ownership
  consumption, and adding compile-time/direct close-once tests.
- `cargo fmt --all --manifest-path Cargo.toml` — passed after Milestone 8 review
  fixes.
- `cargo check --manifest-path Cargo.toml -p maplibre-native --tests` — passed
  after Milestone 8 review fixes.
- `cargo clippy --manifest-path Cargo.toml -p maplibre-native-core -p
  maplibre-native --all-targets -- -D warnings`
  — passed after Milestone 8 review fixes.
- `mise run -C bindings/rust test` — passed after Milestone 8 review fixes (84
  `maplibre-native`, 87 `maplibre-native-core`, 0 `maplibre-native-sys`, doc
  tests passed).

## Reflection checkpoint 2026-05-13

1. Accomplished: Milestones 1-3 are complete and pushed through the review-fix
   commit. Milestone 4 is underway with map options, camera descriptors, and
   runtime options moved into `core` while public Rust names stay re-exported.
2. Working well: small buildable slices plus private extension traits keep the
   public API stable and keep raw `sys` conversions out of public inherent
   methods. Core tests are catching size fields, masks, round trips, and backing
   storage lifetimes close to the shared materializers.
3. Friction: Milestone 4 spans many descriptor families. Some areas, especially
   offline regions and query/style descriptors, mix materialization with result
   reading or Rust handle policy, so extraction needs careful seams rather than
   bulk moves.
4. Approach adjustment: keep moving one descriptor family at a time, with core
   free functions and native wrapper types first. Defer milestone review and
   commits until the remaining Milestone 4 descriptor families are moved and the
   diff is coherent.
5. Next priorities: move offline-region definition materializers/readers next,
   then render-target descriptors, then query/style descriptors. Preserve tests
   for C `size` fields, masks, null pointers, strings, nested arrays, and owned
   backing storage.

## Reflection checkpoint 2026-05-13, Milestone 6

1. Accomplished: Milestones 1-5 are complete, reviewed, committed, and pushed.
   Milestone 6 is mostly implemented: runtime event payload copying, raw source
   preservation, log record copying, and raw severity/event mapping now live in
   `core`; `maplibre-native` keeps `MapId` lookup and callback policy.
2. Working well: moving copied value/payload types wholesale to core reduces
   shim code quickly while preserving public Rust names via re-exports. Keeping
   source lookup and callback invocation above core continues to make the
   policy/mechanism boundary clear.
3. Friction: public Rust tests that used internal event constructors shifted
   into core, so test counts moved between crates. This is expected, but review
   should confirm there are no accidental public API removals or lost coverage.
4. Approach adjustment: finish Milestone 6 with an immediate parallel review
   before committing. Ask reviewers to focus on source-policy separation,
   callback safety, and event/log validation rather than broader refactoring.
5. Next priorities: complete Milestone 6 review/fixes/commit, then start
   Milestone 7 by moving resource request copying and response materialization
   before touching the exactly-once request-handle state machine.

## Notes

- Followed project binding docs: `docs/src/content/docs/development/bindings.md`
  and `docs/src/content/docs/development/bindings-rust.md`.
- Documentation audience/category for the Rust convention update: contributors
  maintaining the binding; Reference documentation. Loaded Diátaxis Reference
  guidance and kept the edits factual and concise.
- The plan itself is in `bindings/rust/PLAN.md` and supersedes the older
  `support` naming in current Rust conventions.
- Iteration 1 completed Milestone 1: renamed the crate and workspace package,
  updated mise tasks and Rust docs, added crate-role docs, and fixed a core
  unit-test race around shared destroy-count state.
- Iteration 2 moved scalar copied value structs (`LatLng`, `ScreenPoint`,
  `EdgeInsets`, `Vec3`, etc.) into `maplibre-native-core::values`, re-exported
  them through `maplibre-native`, and added raw field mapping tests in core.
- Parallel review after iteration 2 found one blocker: public inherent raw
  conversion methods on re-exported value types leaked `sys` types through the
  public Rust API. Fixed by replacing public inherent raw conversions with core
  free functions plus a private `maplibre-native` extension trait for internal
  call sites. Also tightened value tests and stale support/core docs.
- Iteration 3 started enum raw mapping migration by moving `NetworkStatus` to
  `maplibre-native-core::enums`, re-exporting it from `maplibre-native`, and
  adding known/unknown raw mapping tests in core.
- Iteration 4 moved `LogSeverity` and `LogEvent` into core, re-exported them
  from `maplibre-native`, kept `LogSeverityMask` and callback policy in the
  public Rust crate, and added known/unknown preservation tests in core.
- Iteration 5 moved resource request enum domains (`ResourceKind`,
  `ResourceLoadingMethod`, `ResourcePriority`, `ResourceUsage`, and
  `ResourceStoragePolicy`) into core, re-exported them from `maplibre-native`,
  and added core raw mapping/unknown preservation tests.
- Iteration 6 moved runtime/event enum domains (`RuntimeEventType`,
  `RenderMode`, `TileOperation`, `OfflineRegionDownloadState`, and
  `ResourceErrorReason`) into core, re-exported them from `maplibre-native`,
  kept internal module compatibility with `pub(crate)` re-exports from `events`,
  and added core raw mapping/unknown preservation tests.
- Iteration 7 moved map option enums (`MapMode`, `NorthOrientation`,
  `ConstrainMode`, `ViewportMode`, `TileLodMode`), style/tile enums
  (`SourceType`, `TileScheme`, `VectorTileEncoding`, `RasterDemEncoding`,
  `LocationIndicatorImageKind`), `AmbientCacheOperation`,
  `ResourceResponseStatus`, and review-flagged bitmask domains
  (`RenderBackendMask`, `LogSeverityMask`, `MapDebugOptions`) into core with raw
  mapping tests.
- Parallel Milestone 2 review found no blockers. Applied the medium-severity
  bitmask finding. Remaining copied image metadata/value structs are still a
  follow-up before marking Milestone 2 fully complete.
- Committed and pushed completed Milestone 1 plus current Milestone 2 progress
  as `2f1b7ee` on `rust-refactor`.
- Iteration 8 moved copied image metadata/value structs (`TextureImageInfo`,
  `PremultipliedRgba8Image`, and `StyleImageInfo`) into core with constructors
  for cross-crate construction and raw materialization tests.
- Parallel Milestone 2 final review found no blockers and confirmed Milestone 2
  acceptance criteria are satisfied.
- Committed and pushed Milestone 2 completion as `5e55ab8` on `rust-refactor`.
- Iteration 9 started Milestone 3 by moving JSON owned value trees and native
  materialization/copy helpers into `maplibre-native-core::json`. The public
  crate now re-exports `JsonValue` and `JsonMember`, uses a private extension
  trait for internal native conversion calls, and keeps raw `sys` conversion
  helpers out of the public Rust type surface.
- Iteration 10 moved geometry owned value trees and native materialization/copy
  helpers into `maplibre-native-core::geometry`. The public crate now re-exports
  `Geometry`, uses a private extension trait for internal native conversion
  calls, and core owns the geometry depth/materialization tests.
- Iteration 11 completed Milestone 3 by moving GeoJSON feature and collection
  materializers/readers into `maplibre-native-core::geojson`, preserving public
  crate re-exports and private extension traits.
- Parallel Milestone 3 review found no migration blockers and flagged clippy API
  polish plus copied-output ownership coverage. Fixed by using `AsRef` trait
  implementations for native wrappers, adding `NativeJsonMembers::is_empty`,
  documenting `copy_json_members` safety, and adding JSON/geometry/GeoJSON copy
  survival tests. Committed and pushed review fixes as `354e26b`.
- Iteration 12 started Milestone 4 by moving `MapOptions`, `MapViewportOptions`,
  and `MapTileOptions` plus their native materializers and readers into
  `maplibre-native-core::options`. The public Rust crate re-exports the option
  types and keeps raw conversion calls behind private extension traits. Added
  core tests for size fields, field masks, and round trips.
- Iteration 13 moved camera descriptor adaptation into
  `maplibre-native-core::camera`: `CameraOptions`, `AnimationOptions`,
  `CameraFitOptions`, `BoundOptions`, `FreeCameraOptions`, and `ProjectionMode`.
  The public Rust crate re-exports those types and keeps raw conversions/readers
  behind private extension traits. Added core tests for size fields, field
  masks, and round trips.
- Iteration 14 reflection confirmed the slice-by-slice approach is still sound
  for Milestone 4. Moved `RuntimeOptions` and `NativeRuntimeOptions` into
  `maplibre-native-core::runtime`, kept public Rust ergonomics via a private
  extension trait, and added core tests for flags, null pointers, and retained C
  string backing storage.
- Iteration 15 moved offline-region definition descriptors into
  `maplibre-native-core::runtime`, including tile-pyramid and geometry-region
  materializers with retained C string and geometry backing storage. Also moved
  `OfflineRegionInfo` and offline definition copying into core as the first
  Milestone 5 reader extraction slice. Public Rust now re-exports the types and
  uses private extension traits/free functions for raw conversion calls.
- Iteration 16 moved render-target descriptor materializers into
  `maplibre-native-core::render` for generic owned textures, Metal surface and
  texture descriptors, and Vulkan surface and texture descriptors. The public
  Rust descriptors keep `NativePointer` ergonomics and delegate raw size/pointer
  field initialization to core helpers. Added core tests for size fields,
  backend pointer fields, and Vulkan layout/format fields.
- Iteration 17 moved query descriptor materializers into
  `maplibre-native-core::query`: `FeatureStateSelector`,
  `RenderedQueryGeometry`, `RenderedFeatureQueryOptions`, and
  `SourceFeatureQueryOptions` plus their native wrappers. The public render
  module re-exports the query types and uses private extension traits for raw
  materialization. Added core tests for selector masks/views, rendered query
  geometry variants, layer/source-layer arrays, JSON filter pointers, and
  retained backing storage.
- Iteration 18 moved style option descriptor materializers into
  `maplibre-native-core::style`: `TileSourceOptions`, `NativeTileSourceOptions`,
  and `StyleImageOptions`. The public map style module re-exports those types
  and uses private extension traits for raw calls. Added core tests for tile
  source masks, attribution string views, bounds, encoding fields, and style
  image option defaults/masks.
- Iteration 19 completed Milestone 4 review. Parallel reviewers found no
  blockers and confirmed the descriptor extraction is coherent. Custom geometry
  source options remain in the public crate because they include callback
  trampolines and Rust callback/user-data policy rather than bridge-neutral C
  descriptor adaptation.
- Iteration 20 started Milestone 5 result-reader extraction by moving native
  JSON snapshot copying, style ID list copying, and offline-region snapshot/list
  copying into `maplibre-native-core`. The public crate now passes owned
  non-null handles into core free functions, so guard construction and release
  on success/error stay inside the shared ABI adaptation layer for these result
  families.
- Iteration 21 moved `QueriedFeature`, `FeatureExtensionResult`, feature query
  result copying, and feature-extension result copying into
  `maplibre-native-core::query`. The public render module re-exports the copied
  result types and hands owned native result handles to core, keeping handle
  release and copy failure paths out of public binding code. Added core coverage
  for invalid nonempty feature-extension collection pointers.
- Iteration 22 completed the Milestone 5 review round. Reviewers found no public
  API boundary regressions and flagged that core reader helpers must express
  owned-handle safety preconditions. Fixed by making owned native-result reader
  helpers unsafe with safety docs, adding explicit safety comments at public
  crate call sites, removing now-redundant public-crate reader shims, and adding
  offline-region copy failure tests for nonempty null metadata and null geometry
  pointers.
- Iteration 23 started Milestone 6 by moving copied runtime event payload types,
  runtime event raw copying, payload validation, and `empty_runtime_event` into
  `maplibre-native-core::events`. Core now preserves raw source type and raw
  source address in `CopiedRuntimeEvent`; `maplibre-native` keeps only the
  `MapId` lookup/source policy wrapper. Also moved `LogRecord` and log record
  copying into `maplibre-native-core::logging`, leaving process-global callback
  installation, retention, panic containment, and invocation policy in the
  public Rust crate.
- Iteration 24 completed the Milestone 6 reflection and review round. Reviewers
  found one raw-ABI boundary issue and one unsafe-precondition issue; fixed both
  by making runtime event copying unsafe and moving offline-region status raw
  conversion behind a core module helper rather than a public inherent method.
  Added targeted tests for non-null raw source address preservation and invalid
  UTF-8 log callback fallback, then verified and prepared Milestone 6 for
  commit.
- Iteration 25 started Milestone 7 by moving bridge-neutral resource primitives
  into `maplibre-native-core::resource`: `ByteRange`, `ResourceRequest`,
  `ResourceProviderDecision`, `ResourceResponse`, `NativeResourceResponse`, and
  `ResourceTransformRequest`. Public Rust callback state, panic handling,
  replacement URL retention, and the request-handle state machine remain in
  `maplibre-native`; they now delegate request copying and response
  materialization to core. Added core tests for request copy ownership, invalid
  prior data, response fields, and transform request copying.
- Iteration 26 moved the resource request-handle state machine into
  `maplibre-native-core::resource`, including complete/cancelled/release
  function tables, provider decision finalization, exception fallback, and
  exactly-once release accounting. `maplibre-native` now keeps the ergonomic
  `ResourceRequestHandle` wrapper with its Rust `!Sync` marker and callback
  policy, while delegating bridge-neutral handle mechanics to core.
- Iteration 27 completed Milestone 7 review. Fixed the exposed core state
  machine's repeated provider-decision finalization behavior so a second
  finalization preserves the first decision and still releases provider-owned
  handles exactly once. Added a lifetime to `NativeResourceResponse` so native
  byte pointers cannot outlive the borrowed response in safe Rust, and added
  core tests for nonempty response bytes and double successful completion.
- Iteration 28 started and implemented the main Milestone 8 handle-state slice.
  Added `maplibre-native-core::handle::NativeHandleState` with close-once
  status-returning destroy, infallible destroy, live/closed inspection, and
  leak-report-only finalizer support. Core result guards now use this state for
  infallible result handles, and public Rust `ThreadAffineNativeHandle` wraps it
  while keeping Rust's `Rc`-based `!Send` marker and owner-thread `Drop` policy.
- Iteration 29 completed the Milestone 8 review round and fixes. The core state
  now stores addresses as integers with typed reconstruction, so bridges can
  move it into lock/runtime storage while public Rust keeps thread-affinity
  policy in its wrapper. Unsafe destroy preconditions are expressed on close
  methods, leak-reporting documents that it consumes logical ownership, and
  tests cover `Send`, infallible close-once behavior, status close retry, and
  leak without destroy.
