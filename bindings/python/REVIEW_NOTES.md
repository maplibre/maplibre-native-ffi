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

- [ ] `py-metal-render-integration-tests`: Add Metal render workflow integration
      coverage under macOS.
  - severity: high
  - complexity: high
  - area: BND-162 through BND-173 on Metal variants
  - outcome: Add macOS Python fixtures that create `MTLDevice`, `CAMetalLayer`,
    and caller-owned `MTLTexture` objects, expose their raw Objective-C object
    pointers through `NativePointer`, and mirror the Vulkan/OpenGL successful
    attach, readback, frame acquire/release, active-frame rejection, stale-frame
    behavior, caller-owned resource preservation, and native wrong-thread tests.
  - rationale: This branch now covers Vulkan on `linux-x64-vulkan` and
    OpenGL/EGL on `linux-x64-egl`. The binding specification also applies to
    Metal configured variants.
  - implementation notes: Use a macOS validation pass before landing the
    fixture. `pyobjc-framework-Metal` is the likely device/texture bridge;
    `pyobjc-framework-Quartz` and `pyobjc-framework-Cocoa` are likely needed for
    `CAMetalLayer` and any hidden-window surface setup.

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
  now exercises live Vulkan and OpenGL/EGL render sessions.
- `py-frame-acquire-construction-failure`: Owned texture frame acquisition now
  uses a cleanup guard that releases the native frame and clears active-frame
  state if Python frame-handle construction fails.
- `py-status-diagnostics-tests`: Python tests now cover every public native
  status category, OK status, unknown future status, stale diagnostic copying,
  binding-owned diagnostics without stale native leakage, embedded NUL
  validation, and cleanup/support-work diagnostic preservation.
- `py-abi-mismatch-test`: Python runtime construction now has a test-only native
  seam that injects an ABI version, verifies ABI mismatch maps to
  `UnsupportedFeatureError`, and proves the public wrapper does not store a
  native handle after validation failure.
- `py-resource-integration-tests`: Public Python tests now cover resource
  transform URL rewrite, transform clear, copied transform/provider request
  values, native HTTP pass-through, inline/deferred/cross-thread handled style
  completions, double completion, released/stale handles, cancellation before
  late completion, terminal completion after native failure, release/check
  synchronization, and resource error events for both map loading and offline
  region download.
- `py-vulkan-render-integration-tests`: Public Python tests now use the `vulkan`
  and `glfw` Python packages to cover Vulkan surface, session-owned texture, and
  caller-owned texture attach paths; second-session invalid state; render-update
  invalid state; resize; CPU readback; owned-frame metadata, release,
  active-frame rejection, failed release retry, and stale handle behavior;
  caller-owned resource preservation; unsupported frame acquisition on
  caller-owned sessions; and real wrong-thread diagnostics for live Vulkan
  render sessions.
- `py-opengl-egl-render-integration-tests`: Public Python tests now use
  PyOpenGL's EGL/OpenGL ES bindings to cover Linux EGL surface, session-owned
  texture, and caller-owned texture attach paths; second-session invalid state;
  render-update invalid state; resize; CPU readback; owned-frame metadata,
  release, active-frame rejection, failed release retry, and stale texture-name
  behavior; caller-owned resource preservation; unsupported frame acquisition on
  caller-owned sessions; and real wrong-thread diagnostics for live OpenGL/EGL
  render sessions.
