# Kotlin Publishing Review

This file tracks the review of the initial Kotlin Multiplatform snapshot
publishing implementation. The publication design is documented in
[`kotlin-publishing.md`](../../docs/src/content/docs/development/kotlin-publishing.md).

## Fix In This Pass

- [x] `KMP-PUB-001`: publish valid cross-host root metadata
  - severity: high
  - complexity: medium
  - area: Kotlin Multiplatform root Gradle module metadata
  - rationale: a host that cannot build a registered cinterop target currently
    writes fallback `kotlin-<target>:unspecified` coordinates into the root
    module metadata.
  - acceptance: every root `available-at` reference uses the
    `org.maplibre.nativeffi` group, the expected target module, and the
    configured version; staging verification checks these values.

- [x] `KMP-PUB-002`: use portable jextract output for the JVM publication
  - severity: high
  - complexity: low
  - area: generated JVM FFM declarations
  - rationale: the pinned jextract build emits a runtime cast for C `long` which
    fails when Unix-generated declarations run on Windows.
  - acceptance: use the current JDK 25 jextract build, normalize its
    host-derived C `long` layout to the fixed 64-bit JVM layout, and keep the
    public C headers free of plain C `long`.

- [x] `KMP-PUB-003`: consume the Cargo-packaged Rustls Android helper
  - severity: medium
  - complexity: low
  - area: Android core AAR
  - rationale: the Cargo-locked `rustls-platform-verifier-android` crate already
    contains the upstream helper AAR, so maintaining a reformatted Kotlin copy
    is unnecessary and can drift from the Rust dependency.
  - acceptance: package the locked upstream helper bytecode in the core AAR,
    retain its licenses and consumer R8 rule, and remove the vendored source.

- [x] `KMP-PUB-004`: validate every runtime publication input
  - severity: medium
  - complexity: low
  - area: backend runtime KLIBs, AARs, and JVM classifier JARs
  - rationale: host-install fallback can silently label a native artifact with
    the wrong backend or platform when publication tasks are invoked outside the
    snapshot script.
  - acceptance: publication requires explicit install inputs and checks each
    `artifact.json` backend, operating system, and architecture before
    packaging; ordinary local build and test tasks may use the host install.

- [x] `KMP-PUB-005`: consolidate runtime Gradle configuration
  - severity: medium
  - complexity: medium
  - area: OpenGL, Vulkan, and Metal runtime modules
  - rationale: the three build scripts duplicate target, cinterop, Android, JVM
    classifier, and Maven publication logic.
  - acceptance: shared typed build logic owns packaging and validation while
    each module declares only its backend-specific target and metadata choices.

- [x] `KMP-PUB-006`: make staging verification semantic
  - severity: high
  - complexity: low
  - area: staged Maven repository verification
  - rationale: parsing `.module` files as JSON does not detect invalid target
    coordinates or publication payload provenance.
  - acceptance: verification checks root target coordinates, Rustls payload
    provenance, classifier payload identity, and the existing KLIB/AAR/JAR
    contents.

- [ ] `KMP-PUB-007`: make the staged repository the release source of truth
  - severity: high
  - complexity: medium
  - area: snapshot and future release orchestration
  - rationale: host jobs currently recreate publications while uploading instead
    of finalizing the exact repository that was merged and verified.
  - acceptance: host jobs only stage; a finalization layer consumes the merged
    repository. Stable versions can be signed, checksummed, bundled, and sent to
    the Central Publisher API without changing host build orchestration.
  - status: the stable-release finalizer contract is documented; implementing
    its signing, bundle upload, and release trigger remains.

- [x] `KMP-PUB-008`: publish snapshot roots after leaf modules
  - severity: medium
  - complexity: low
  - area: Central Portal snapshot workflow
  - rationale: snapshots use a mutable Maven-deploy endpoint with no atomic
    deployment boundary; publishing roots last prevents a new root from
    advertising a leaf set whose upload failed.
  - acceptance: Linux and Apple leaf publication completes before canonical
    roots publish, and concurrent snapshot workflows cannot interleave.

- [x] `KMP-PUB-009`: document the actual host runtime boundary
  - severity: low
  - complexity: low
  - area: Kotlin publishing documentation
  - rationale: JVM runtime JARs package MapLibre Native FFI and selected
    redistributable companions, while Linux continues to provide common system
    libraries such as zlib and libuv in addition to graphics loaders.
  - acceptance: documentation no longer promises a complete dependency set and
    lists the current Linux host prerequisites.

## Logged For Triage

- [ ] `KMP-PUB-101`: automate published JVM runtime execution
  - severity: medium
  - complexity: medium
  - area: published classifier verification
  - rationale: compilation verifies dependency resolution but does not execute
    the classpath extractor and loader.
  - suggested next step: manually test each JVM operating system for the first
    snapshot; revisit automation after publication behavior stabilizes.

- [ ] `KMP-PUB-102`: add a final Kotlin/Native consumer link test
  - severity: medium
  - complexity: high
  - area: transitive cinterop linker options
  - rationale: KLIB inspection proves that the static archive is embedded but
    not that an external application completes its final native link.
  - suggested next step: exercise this through the planned iOS target in the
    Compose map example rather than adding a standalone fixture.

## Accepted Constraints

- Linux hosts may provide zlib and libuv for the initial snapshots. Replacing
  those native dependencies with Rust crates is a future native-runtime change.
- The Maven POM identifies the main project license as BSD-2-Clause. Embedded
  third-party license texts remain in the Android artifact.
- The initial snapshot does not require an automated GUI or headless example
  execution mode.
- Central Portal snapshots are mutable Maven deployments and do not provide the
  atomic validation boundary used for stable release bundles.

## Invalidated

- `KMP-PUB-X01`: rely on a jextract upgrade alone for portable C `long`
  - rationale: JDK 25 jextract build `25-jextract+2-4` still emits a
    host-derived `C_LONG`. The narrowly validated normalization in `KMP-PUB-002`
    remains necessary for one JVM artifact to serve every supported 64-bit
    operating system.

- `KMP-PUB-X02`: require snapshot publication to be globally atomic
  - rationale: the Central snapshot endpoint intentionally provides mutable,
    unvalidated Maven deployment semantics. Root-last ordering and workflow
    serialization mitigate incomplete publication without claiming atomicity.
