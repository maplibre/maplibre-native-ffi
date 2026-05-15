# Zig binding completion plan

## Audience and purpose

This plan is for maintainers and contributors finishing the initial Zig binding
before merge. It tracks the remaining work from the parallel branch review and
keeps the path to merge focused.

Long-term rules live in the binding conventions documents. This plan covers the
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
public Zig API, and the retained direct C suite covers behavior that needs raw C
ABI access.

## Remaining work

### 1. Fix binding correctness

Fix the correctness issues that can surprise Zig callers even when the current
test suite passes.

Work items:

- Allow embedded NUL bytes in explicit-length C string views. Keep embedded-NUL
  rejection only for null-terminated inputs.
- Preserve native diagnostics for public status-returning APIs where callers can
  provide a `DiagnosticStore`, including resource-request completion/cancel
  helpers and projection free helpers.

Acceptance:

- Explicit-length string conversion passes byte spans through to C without
  truncating or rejecting valid data.
- Null-terminated string helpers still reject embedded NUL bytes.
- Public helpers that can surface native diagnostics accept or use a diagnostic
  store consistently with the Zig binding conventions.
- Focused tests cover the changed string and diagnostic behavior.

### 2. Finish binding-level migration coverage

Move remaining behavior tests from retained `@cImport` coverage into public Zig
binding tests when the binding exposes the needed API.

Work items:

- Add binding-level tests for projection invalid/error behavior that currently
  lives only in `src/c_api/tests/zig/projection.zig`.
- Add binding-level tests for feasible resource/backend integration behavior,
  including file, asset, and missing URL style loading.
- Add binding-level tests for native HTTP, pass-through, and ambient-cache style
  loading where the public binding can express the scenario.
- Add binding-level tests for PMTiles range metadata when the public binding can
  observe the behavior.
- Add binding-level tests for offline response-error events and offline database
  merge behavior.
- Keep raw null pointers, undersized structs, invalid raw enum values, stale
  native handles, backend host scaffolding, and descriptor contracts in the
  retained C ABI suite.

Acceptance:

- Each legacy scenario has equivalent public-binding coverage or a short reason
  to keep it in the retained C ABI suite.
- Binding behavior lives in `bindings/zig/tests/`.
- Raw C ABI behavior lives in `src/c_api/tests/zig/`.
- `mise run test` runs both suites successfully.

### 3. Complete reviewed Rust/Java parity gaps

Add the missing Zig APIs that Rust and Java already expose over the C ABI.

Work items:

- Add camera and navigation helpers for still-image requests, animated gesture
  variants, fitting coordinate lists and geometry, unwrapped bounds, bounds
  constraints, and free-camera get/set.
- Add style layer lifecycle and JSON helpers to add, remove, check, move, and
  fetch layer JSON.
- Add projection `setVisibleGeometry`.
- Add a readback metadata helper, such as `textureImageInfo`.
- Make `setResourceTransform` accept `?ResourceTransform`, using `null` to
  restore pass-through behavior instead of adding a separate clear helper.

Acceptance:

- The Zig binding exposes the reviewed APIs with names and semantics consistent
  with the Zig binding conventions.
- New APIs have binding-level tests through the public Zig package.

### 4. Clean public API leaks

Simplify the public binding surface so implementation details stay private and
future storage changes remain possible.

Work items:

- Hide public handle internals such as `RuntimeHandle.state`, `MapHandle.state`,
  `MapProjectionHandle.state`, `RenderSessionHandle.state`, and
  `ResourceRequestHandle` registry fields.
- Move slot-reuse and generation-counter assertions into private module tests or
  cover them through public behavior.
- Replace public reserved-bit fields in `LogSeverityMask` with a semantic public
  API for supported severities. Keep raw invalid-bit coverage in retained C ABI
  tests or private binding tests.
- Simplify `TempStorage` by using one allocation strategy where practical.
- Make owned image cleanup consistent with other owned values: clear fields
  after free and reduce double-free risk.
- Factor repeated Windows test-runner runtime-path setup in
  `bindings/zig/build.zig`.

Acceptance:

- Public handles expose behavior, not registry or lifetime internals.
- Public logging severity APIs expose supported concepts instead of raw reserved
  fields.
- Cleanup changes reduce duplication without broad rewrites.
- Binding tests keep covering ownership, stale-handle, logging, and cleanup
  behavior through stable public APIs where possible.

## Validation before merge

Run these checks after the remaining work lands:

```sh
mise run //bindings/zig:test
mise run test
mise run fix
```

The branch is ready to merge when these checks pass, reviewed parity work is
implemented, and retained C ABI tests cover only behavior that needs raw C or
host integration access.
