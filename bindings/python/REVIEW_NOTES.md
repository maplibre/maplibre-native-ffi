# Review Findings

## Logged For Triage

- [ ] `py-api-wire-methods`: Raw bridge conversion methods are public-looking
      API.
  - severity: medium
  - complexity: medium
  - area: public value types in `python/maplibre_native`
  - rationale: Exported value classes expose methods such as `from_native()` and
    `to_native()` that accept or return private bridge dictionaries. The binding
    spec keeps raw C and host FFI carrier shapes outside the safe public API,
    but renaming these methods to `_from_native()` and `_to_native()` touches
    most domains and tests.
  - suggested next step: Rename native-wire conversion methods behind private
    helpers domain by domain, starting with runtime/resource/render/query
    values.

- [ ] `py-json-carriers`: JSON APIs accept broad Python carriers alongside
      explicit JSON values.
  - severity: medium
  - complexity: medium
  - area: `json.py`, style property APIs, feature-state APIs
  - rationale: Broad carriers such as Python `dict`, raw `int`, and raw `float`
    are ergonomic, but they create alternate workflows and can hide
    duplicate-key or unsigned-width intent compared with the explicit JSON model
    required by the spec.
  - suggested next step: Decide whether Python-container conversion belongs in a
    separate helper layer; otherwise narrow low-level public APIs to explicit
    `JsonValue` and `JsonObject` inputs.

- [ ] `py-value-factories`: Convenience value factories duplicate dataclass
      constructors.
  - severity: low
  - complexity: medium
  - area: `geo.py`, `json.py`, `resource.py`
  - rationale: Helpers such as `geo.point()`, `json_uint()`, and
    `ResourceResponse.ok()` add a second way to construct values. Removing or
    relocating them is low behavioral risk but broad API churn.
  - suggested next step: Review whether these helpers are Python
    safety/ergonomics or convenience API; keep only the helpers that preserve
    required value semantics.

- [ ] `py-allocating-readback`: Readback exposes both caller-buffer and
      allocating workflows.
  - severity: low
  - complexity: medium
  - area: `render.py`
  - rationale: `read_premultiplied_rgba8_into()` maps directly to caller-owned
    mutable storage, while `read_premultiplied_rgba8()` allocates and copies
    bytes. The allocating path may be acceptable Python ergonomics, but it is a
    redundant workflow in the low-level binding.
  - suggested next step: Decide whether the allocating helper stays in the
    low-level API or moves to a higher-level helper module.

- [ ] `py-native-stub`: The private native extension stub is `Any`-only.
  - severity: low
  - complexity: medium
  - area: `_native.pyi`, typed package boundary
  - rationale: The package includes `py.typed`, but `_native.pyi` exposes
    `__getattr__(name: str) -> Any`. This keeps the raw layer private by
    convention, but type checking cannot catch accidental raw-layer drift inside
    wrappers.
  - suggested next step: Generate or maintain an explicit private `_native`
    stub, or add internal protocols for wrapper-facing native objects.

- [ ] `py-abi-mismatch-test`: ABI mismatch coverage is missing.
  - severity: medium
  - complexity: medium
  - area: BND-001, native loading
  - rationale: Runtime creation validates the ABI before storing a public
    handle, and tests cover the happy path, but there is no mismatch seam
    proving the ABI-version error path.
  - suggested next step: Add an internal loader or version seam that forces
    mismatch before public handle creation.

- [ ] `py-status-diagnostics-tests`: Status and diagnostic coverage is
      incomplete.
  - severity: medium
  - complexity: medium
  - area: BND-020 through BND-026
  - rationale: Tests cover representative invalid-argument diagnostics, but not
    every status category, unknown future status, stale diagnostic copying, or
    cleanup-preserved diagnostics.
  - suggested next step: Add internal conversion and diagnostic seams for status
    categories that are hard to produce with real native calls.

- [ ] `py-render-integration-tests`: Render workflow coverage is incomplete.
  - severity: high
  - complexity: high
  - area: BND-162 through BND-173
  - rationale: The public API exposes attach, readback, and frame-lifecycle
    workflows, but tests rely heavily on descriptors, fake natives, and invalid
    attach paths. Successful backend-specific native workflows require
    configured render resources and broader setup.
  - suggested next step: Add backend-gated integration tests for successful
    attach, readback, frame acquire/release, active-frame rejection, and
    stale-frame behavior.

- [ ] `py-resource-integration-tests`: Resource provider and transform coverage
      is incomplete.
  - severity: high
  - complexity: high
  - area: BND-140 through BND-153
  - rationale: Current tests cover adapters, fake request handles, and
    registration bounds. The spec requires real rewrite behavior, handled style
    loads, deferred and cross-thread completion, cancellation, stale handles,
    double completion, and release races.
  - suggested next step: Add public Python integration tests mirroring the
    dedicated C/Zig resource scenarios.

- [ ] `py-wrong-thread-coverage`: Owner-thread wrong-thread tests are narrow.
  - severity: low
  - complexity: medium
  - area: BND-190 and BND-191
  - rationale: Python tests runtime close, `run_once`, `poll_event`, and one map
    method from the wrong thread, but render-session methods and resource
    transform/provider registration still need coverage.
  - suggested next step: Add wrong-thread tests for resource transform set/clear
    and render-session methods.

- [ ] `py-runtime-close-race`: Runtime close can expose closed native state
      during destroy.
  - severity: medium
  - complexity: medium
  - area: `src/lib.rs` runtime handle state
  - rationale: The PyO3 runtime close path marks the native handle closed,
    releases the GIL, and then runs `mln_runtime_destroy()`. Concurrent runtime
    methods can observe a null native pointer and surface native
    invalid-argument behavior instead of a binding-owned releasing or closed
    error.
  - suggested next step: Add an explicit runtime releasing state checked by
    every runtime operation before crossing into C.

- [ ] `py-frame-acquire-construction-failure`: Acquired texture frames need
      post-acquire construction-failure cleanup.
  - severity: medium
  - complexity: medium
  - area: owned texture frame acquisition in `src/lib.rs`
  - rationale: Frame acquisition marks a frame active and returns a PyO3 frame
    object. If wrapper construction fails after native acquisition, the frame
    `Drop` implementations intentionally do not release native state from
    cleanup hooks.
  - suggested next step: Add an acquisition guard that releases the native frame
    on post-acquire construction failure and disarms after successful object
    creation.

## Invalidated

- `py-sealed-handle-tests`: Sealed handle tests instantiate non-public
  constructors.
  - rationale: The current tests use private factories for internal seams and
    assert direct public construction fails.
