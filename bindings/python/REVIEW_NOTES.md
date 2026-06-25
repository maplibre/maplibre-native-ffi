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

- [ ] `py-api-wire-methods`: Make raw bridge conversion methods private.
  - severity: medium
  - complexity: medium
  - area: public value types in `python/maplibre_native`
  - outcome: Rename public-looking native-wire conversion methods such as
    `from_native()` and `to_native()` to private helpers such as
    `_from_native()` and `_to_native()`.
  - rationale: Exported value classes currently expose methods that accept or
    return private bridge dictionaries. The binding spec keeps raw C and host
    FFI carrier shapes outside the safe public API.
  - implementation notes: Migrate domain by domain, starting with
    runtime/resource/render/query values, and update tests to use private seams
    where raw wire values are intentional.

- [ ] `py-native-stub`: Replace the `Any` catch-all native extension stub.
  - severity: low
  - complexity: medium
  - area: `_native.pyi`, typed package boundary
  - outcome: Give wrapper-facing native calls typed internal interfaces instead
    of `__getattr__(name: str) -> Any`.
  - rationale: The raw layer stays private by convention, but type checking
    cannot catch accidental raw-layer drift inside wrappers.
  - implementation notes: Use an explicit private `_native.pyi` or typed
    internal protocols for wrapper-facing native objects.

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
    active-frame rejection, stale-frame behavior, and caller-owned resource
    preservation for configured backends.
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

- [ ] `py-wrong-thread-coverage`: Add the remaining wrong-thread tests.
  - severity: low
  - complexity: medium
  - area: BND-190 and BND-191
  - outcome: Cover wrong-thread errors and diagnostics for resource transform
    set/clear and render-session methods.
  - rationale: Python already covers runtime close, `run_once`, `poll_event`,
    and one map method from the wrong thread. Render-session and resource
    registration coverage remains.
  - implementation notes: Use public owner-thread handles and assert
    `WrongThreadError` with copied diagnostics where native supplies them.

- [ ] `py-runtime-close-race`: Add an explicit runtime releasing state.
  - severity: medium
  - complexity: medium
  - area: `src/lib.rs` runtime handle state
  - outcome: Runtime operations fail with a binding-owned releasing or closed
    error before crossing into C while `close()` is in progress.
  - rationale: The PyO3 runtime close path marks the native handle closed,
    releases the GIL, and then runs `mln_runtime_destroy()`. Concurrent runtime
    methods can observe a null native pointer and surface native
    invalid-argument behavior.
  - implementation notes: Extend the runtime operation gate or handle state and
    check it from every runtime method before C calls.

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
