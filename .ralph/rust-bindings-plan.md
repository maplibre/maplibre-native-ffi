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
- [ ] Milestone 3: Move GeoJSON feature and feature-collection materializers and
      readers.
- [ ] Milestone 3: Preserve depth checks, integer width, object member order,
      duplicate keys, and finite-number validation.
- [ ] Milestone 4+: Continue descriptor/result/event/resource/handle milestones
      in small buildable slices.

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
