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

- [ ] Define native status error mapping.
- [ ] Define public binding error set for status and binding-local validation
      errors.
- [ ] Add owned diagnostic record and `DiagnosticStore`.
- [ ] Implement `checkStatus(status, diagnostics)` with thread-local message
      copying before later C calls.
- [ ] Add ABI version validation with diagnostics.
- [ ] Add tests for invalid status mapping, copied diagnostics, and unknown
      status preservation.
- [ ] Review, apply findings, commit, and push.

### Phase 3: runtime and map vertical slice

- [ ] Implement `RuntimeHandle` with create/init, `runOnce`, `pollEvent`, and
      fallible close.
- [ ] Implement `MapHandle` with creation, basic options, style setter, and
      fallible close.
- [ ] Implement initial `MapProjectionHandle` ownership and close.
- [ ] Add private live-handle checks and retryable close behavior.
- [ ] Verify no public API leaks raw C handle types.
- [ ] Add vertical-slice binding test.
- [ ] Review, apply findings, commit, and push.

### Phase 4: foundational tests

- [ ] Port `diagnostics.zig` assertions to binding tests.
- [ ] Port `runtime.zig` assertions to binding tests.
- [ ] Port `map_lifecycle.zig` assertions to binding tests.
- [ ] Retire exact duplicate direct C API assertions only after binding coverage
      preserves intent.
- [ ] Keep a private C import compile test if needed for early header
      bindability.
- [ ] Review, apply findings, commit, and push.

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

- [ ] Add binding-assigned `MapId`.
- [ ] Register maps with runtimes for source lookup while handles are live.
- [ ] Copy runtime events into owned Zig values before next poll.
- [ ] Preserve unknown event and payload raw values.
- [ ] Apply binding-owned side effects required by other bindings where
      applicable.
- [ ] Port `events.zig` coverage.
- [ ] Review, apply findings, commit, and push.

### Phase 7: logging, resources, offline, and callbacks

- [ ] Add network status and logging callback APIs.
- [ ] Add runtime ambient cache operations.
- [ ] Add offline region APIs with copied value types.
- [ ] Add resource transform callbacks.
- [ ] Add resource provider callbacks and `ResourceRequestHandle`.
- [ ] Add custom geometry source callbacks and lifetime management.
- [ ] Model callbacks as function pointers plus context pointers first.
- [ ] Ensure C trampolines use `callconv(.c)`, copy borrowed data before
      returning, and never let failures escape through C frames.
- [ ] Enforce one-shot request completion or release.
- [ ] Port logging, resources, offline, and custom geometry tests.
- [ ] Review, apply findings, commit, and push.

### Phase 8: render targets and readback

- [ ] Add `NativePointer` borrowed opaque backend address value.
- [ ] Add public render target descriptors for supported Metal and Vulkan
      surfaces, borrowed textures, and owned textures.
- [ ] Add `RenderSessionHandle` with fallible close.
- [ ] Add readback APIs for caller-owned buffers and allocator-backed owned
      images.
- [ ] Add owned texture frame handles with scoped native pointer access.
- [ ] Port render backend, surface, texture, feature-state, and query render
      coverage.
- [ ] Review, apply findings, commit, and push.

### Phase 9: examples and documentation

- [ ] Port `examples/zig-readback` to the binding.
- [ ] Port `examples/zig-map` to the binding.
- [ ] Update contributor docs and command lists for Zig binding tasks.
- [ ] Add the Zig binding suite to root `mise run test` when supported by native
      variants.
- [ ] Update `.github/config/variants.toml` and workflow task lists for
      supported platform/render-backend variants and explicit exclusions.
- [ ] Keep examples small and focused on low-level binding usage.
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
  (`Add initial Zig
  binding package skeleton`) and pushed
  `zig-binding-implementation` to origin.

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

## Notes and decisions

- Keep direct Zig C tests until equivalent public-binding assertions land.
- Prefer narrow binding test commands early; integrate root `mise run test`
  after the binding suite has a supported path.
- Commit and push every completed phase or meaningful milestone so the branch
  always carries the latest Ralph artifact and implementation state.
- Phase 1 uses a tiny public root (`cAbiVersion`) to prove native linking while
  keeping the raw `@cImport` private in `src/c.zig` and out of the package root.
