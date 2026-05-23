# Java JNI branch review report

## Audience and documentation role

Audience: contributors and reviewers evaluating the Java JNI scaffold branch.
Category: reference report. This file summarizes branch-vs-`main` review
findings, dispositions, and final validation evidence.

## Scope

Parallel review rounds covered API parity, public surface, Rust JNI/native
bridge correctness, lifecycle and callback safety, diagnostics, signed/unsigned
JNI boundaries, tests, validation tasks, loader behavior, and SPEC/REVIEW
maintainability.

Reviewers compared the active Java JNI worktree against `main` and the Java FFM
binding conventions. All actionable findings were either integrated or rejected
with rationale. No deferred findings remain.

## Integrated findings

### Public API parity and inventory

- Removed FFM/`java.lang.foreign` leakage from Java JNI sources and tests.
- Restored public lifetime and ownership Javadocs for runtime, map, projection,
  resource request, render readback, and owned texture frame APIs.
- Made internal raw-address and access-token seams package-private where JNI can
  keep them out of the exported public surface.
- Expanded `verify_public_inventory.py` to catch missing public files, extra
  public sources, stale FFM imports, public/protected signature drift, enum
  constant drift, internal access leaks, and missing exports.
- Updated `SPEC.md` inventories for public packages, internal bridge helpers,
  `JniTestNative`, tests, `tools/jni_library_path.py`, and the single-file Rust
  bridge with future split labels.

### Native bridge correctness

- Converted Java strings through standard UTF-8 before creating C strings and
  retained embedded-NUL rejection coverage.
- Added JNI thread-local diagnostics so JNI validation failures do not reuse
  stale C diagnostics.
- Routed direct JNI invalid-argument returns through the JNI diagnostic helper
  and added verifier coverage.
- Wrapped `vulkan_borrowed_texture_attach` fully in a panic boundary.
- Validated signed Java inputs before unsigned Rust/C casts for:
  - map and render target dimensions;
  - scaled render dimensions that must fit Java `int` metadata;
  - runtime `maximumCacheSize`;
  - tile-source `tileSize`;
  - custom-geometry `tileSize` and `buffer`;
  - Vulkan graphics queue family, format, initial layout, and final layout.
- Reworked log and custom-geometry callback state from raw retired pointers to
  token registries with `Arc`-backed live state. Replacement, clear, and source
  removal now drop registry entries without invalidating in-flight callbacks.

### Native loading and validation wiring

- Standardized the JNI native library name as `maplibre_native_jni` in Java and
  Cargo metadata.
- Added `tools/jni_library_path.py`.
- Added Java JNI `mise` tasks for verify, native build, native tests, and branch
  readiness checks.
- Made `check` build native prerequisites before Cargo check/test.
- Made native-required Java tests fail instead of skip when native loading is
  required.
- Forwarded `org.maplibre.nativejni.library.path`,
  `MAPLIBRE_NATIVE_JNI_LIBRARY_PATH`, `java.library.path`,
  `org.maplibre.nativejni.tests.requireNative`, and
  `--enable-native-access=ALL-UNNAMED` into Gradle test execution where
  relevant.
- Added loader smoke coverage for exact-path system property, environment path,
  and `System.loadLibrary` via `java.library.path`.

### Lifecycle and behavior parity

- Documented and tested JNI `NativeBuffer` behavior: direct `ByteBuffer`
  storage, `Integer.MAX_VALUE` capacity limit, copied readback behavior, and
  close invalidation.
- Matched Java FFM behavior for `discardOfflineOperation` after runtime close by
  consuming the operation before rethrowing closed-runtime errors.
- Added regression coverage for callback references, UTF-8 conversion,
  diagnostics, native loader paths, signed/unsigned validation, resource request
  lifecycles, and render buffer behavior.

### Documentation and review log

- Replaced early scaffold/proof-slice wording in `SPEC.md` with the current
  implementation snapshot.
- Recorded JVM distribution packaging and Android/AAR packaging as out of scope
  for this implementation pass.
- Created this review report next to `SPEC.md` and condensed the round-by-round
  audit trail into this single report.

## Rejected findings

- Staged-index findings were rejected because this review-readiness work
  operates on the active worktree, not commit preparation. The worktree content
  and `git diff` are the branch review source.
- A proposed SPEC note for process-lifetime custom-geometry state retirement was
  rejected after that implementation was replaced by token-registry drop
  semantics.

## Non-blocking note

`cargo test -p maplibre-native-jni` currently reports zero Rust unit/doc tests
and functions as a compile-smoke gate. Java JNI native tests are the behavioral
gate for this scaffold pass.

## Final review outcome

Round 12 reported no novel actionable findings. Clean review angles included:

- API parity and module exports;
- SPEC and REVIEW coherence;
- JNI registration and native bridge correctness;
- signed/unsigned validation;
- callback-state token registries;
- native loader coverage;
- Gradle input modeling;
- Java native validation.

Deferred findings: none.

## Final validation evidence

The final validation set passed:

- `mise run fix`
- `mise run //bindings/java-jni:verify`
  - public inventory: 117 files;
  - internal struct inventory: 8 files;
  - native coverage: 212 JNI declarations/Rust registrations and 42 recorded
    unsupported helper replacements.
- `mise run //bindings/java-jni:build`
- `mise run //bindings/java-jni:check`
  - `mise run //bindings/java-jni:verify`
  - `mise run //bindings/java-jni:native:build`
  - `cargo check -p maplibre-native-jni`
  - `cargo test -p maplibre-native-jni`
  - `mise run //bindings/java-jni:test:native`
  - `./gradlew :bindings:java-jni:javadoc`
- Gradle loader smoke tests for exact path, environment path, and
  `java.library.path`.
- `git diff --check`

Latest Java native test result summary: 87 tests, 0 skipped, 0 failures, 0
errors.
