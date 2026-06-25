# Review Findings

## Needs Maintainer Decision

These findings are API-shape decisions. The implementation path depends on
whether the Python binding should optimize for strict low-level minimalism or
keep a small amount of Python-native ergonomics.

- [ ] `py-json-carriers`: Decide whether low-level JSON APIs accept Python
      carrier values.
  - severity: medium
  - complexity: medium
  - area: `json.py`, style property APIs, feature-state APIs
  - decision needed: Keep accepting Python `dict`, raw `int`, and raw `float`
    values as opt-in Python ergonomics, or require explicit `JsonValue` and
    `JsonObject` values at every low-level API boundary.
  - rationale: Broad carriers create alternate workflows and can hide
    duplicate-key or unsigned-width intent compared with the explicit JSON model
    required by the binding spec.
  - implementation if narrowed: Move Python-container conversion behind explicit
    helper APIs and narrow low-level public method annotations and materializers
    to explicit JSON value types.

- [ ] `py-value-factories`: Decide whether convenience value factories belong in
      the low-level public API.
  - severity: low
  - complexity: medium
  - area: `geo.py`, `json.py`, `resource.py`
  - decision needed: Keep helpers such as `geo.point()`, `json_uint()`, and
    `ResourceResponse.ok()` as Python ergonomics, or expose only the semantic
    dataclass/model constructors in the low-level binding.
  - rationale: The helpers add a second way to construct the same public values.
    They are convenient, but they are not required for ownership, lifetime, or
    type-safety policy.
  - implementation if narrowed: Remove or relocate convenience factories,
    keeping only helpers that preserve required value semantics better than raw
    constructors.

- [ ] `py-allocating-readback`: Decide whether readback includes an allocating
      helper.
  - severity: low
  - complexity: medium
  - area: `render.py`
  - decision needed: Keep `read_premultiplied_rgba8()` as Python-owned
    ergonomics, or expose only caller-buffer readback in the low-level binding.
  - rationale: `read_premultiplied_rgba8_into()` maps directly to the C
    caller-owned mutable storage contract. The allocating helper is safe and
    Pythonic, but it is a redundant workflow above the C operation.
  - implementation if narrowed: Move the allocating helper outside the low-level
    render session API or remove it from the supported surface.

## Clear Engineering Backlog

These findings have a clear desired outcome and do not need further maintainer
input before implementation.

- [ ] `py-abi-mismatch-test`: Add ABI mismatch coverage.
  - severity: medium
  - complexity: medium
  - area: BND-001, native loading
  - outcome: Prove ABI mismatch fails with the public ABI-version error before a
    public wrapper stores a native handle.
  - rationale: Runtime creation validates ABI before storing a public handle,
    and tests cover the happy path, but there is no mismatch seam.
  - implementation notes: Add an internal loader or version seam that forces
    mismatch before public handle creation.

- [ ] `py-status-diagnostics-tests`: Expand status and diagnostic coverage.
  - severity: medium
  - complexity: medium
  - area: BND-020 through BND-026
  - outcome: Cover every public status category, unknown future status, stale
    diagnostic copying, and cleanup-preserved diagnostics.
  - rationale: Current tests cover representative invalid-argument diagnostics,
    but not the full status and diagnostic matrix required by the spec.
  - implementation notes: Add internal conversion and diagnostic seams for
    status categories that are hard to produce with real native calls.

- [ ] `py-render-integration-tests`: Add render workflow integration coverage.
  - severity: high
  - complexity: high
  - area: BND-162 through BND-173
  - outcome: Cover successful attach, readback, frame acquire/release,
    active-frame rejection, stale-frame behavior, caller-owned resource
    preservation, and native render-session wrong-thread behavior for configured
    backends.
  - rationale: Current tests rely heavily on descriptors, fake natives, and
    invalid attach paths. The spec requires successful backend-specific native
    workflows.
  - implementation notes: Add backend-gated public tests that use real render
    resources for each configured backend.

- [ ] `py-resource-integration-tests`: Add resource provider and transform
      integration coverage.
  - severity: high
  - complexity: high
  - area: BND-140 through BND-153
  - outcome: Cover URL rewrite, copied request data, native pass-through,
    inline/deferred/cross-thread handled completion, cancellation, stale
    handles, double completion, release races, and terminal completion behavior.
  - rationale: Current tests cover adapters, fake request handles, and
    registration bounds. The spec requires real public workflows crossing the
    binding/C boundary.
  - implementation notes: Mirror the dedicated C/Zig resource scenarios through
    public Python APIs.

- [ ] `py-frame-acquire-construction-failure`: Add post-acquire cleanup guards
      for owned texture frames.
  - severity: medium
  - complexity: medium
  - area: owned texture frame acquisition in `src/lib.rs`
  - outcome: If Python wrapper construction fails after native frame
    acquisition, the binding releases the native frame and clears the active
    frame state.
  - rationale: Frame acquisition marks a frame active and returns a PyO3 frame
    object. Frame `Drop` intentionally does not release native state from
    cleanup hooks, so construction failure needs an explicit guard.
  - implementation notes: Add an acquisition guard that releases the native
    frame on post-acquire failure and disarms after successful object creation.

## Invalidated

- `py-sealed-handle-tests`: Sealed handle tests instantiate non-public
  constructors.
  - rationale: The current tests use private factories for internal seams and
    assert direct public construction fails.

## Completed In This Branch

- `py-api-wire-methods`: Raw native-wire value conversions now use underscored
  helpers such as `_from_native()`, `_to_native()`, and
  `_from_runtime_payload()`.
- `py-runtime-close-race`: Runtime operations now pass through a binding-owned
  close gate before calling C, and closed-runtime map creation reports a copied
  binding diagnostic.
- `py-native-stub`: `_native.pyi` now declares the wrapper-facing native handle
  classes, methods, and module functions instead of exporting a catch-all
  `__getattr__`.
- `py-wrong-thread-coverage`: Resource transform set/clear now have real
  wrong-thread tests, and render-session public methods now have wrapper-level
  wrong-thread propagation coverage. Native render-session wrong-thread coverage
  remains part of `py-render-integration-tests`, where backend render fixtures
  can exercise real session handles.
