# Dart binding review notes

This log contains only findings that need architectural or infrastructure work
after updating the `dart` branch to `main` at `3de8ea9763`. The binding is
prerelease; these are design tasks rather than compatibility commitments.

## Logged for triage

- [ ] **DART-OWNER-THREAD — Native owner-thread executor**
  - Severity: high
  - Complexity: high
  - Area: owner-thread execution model and BND-192
  - Rationale: a Dart isolate is a logical concurrency boundary, but the VM may
    resume it on another operating-system thread after an asynchronous
    suspension. Isolate checks catch cross-isolate misuse without guaranteeing
    that later direct FFI calls run on a C handle's native owner thread.
    Synchronous methods currently preserve the C API's direct error semantics
    and surface `MLN_STATUS_WRONG_THREAD`.
  - Suggested next step: design a native-bound dispatcher that owns runtime
    creation, pumping, operations, event copying, and destruction across the
    complete handle surface.

- [ ] **DART-RESOURCE-CALLBACKS — General synchronous resource callbacks**
  - Severity: high
  - Complexity: high
  - Area: resource callback rules and BND-140 through BND-153
  - Rationale: `NativeCallable.listener` accepts calls from arbitrary native
    worker threads but returns `void` asynchronously. The binding therefore uses
    native-owned exact URL rewrite/response tables and native-routed queued
    providers. It cannot safely expose a general synchronous Dart transform
    callback or let Dart select pass-through after inspecting every request.
  - Suggested next step: add a native synchronous decision layer or solve this
    as part of the native owner-thread dispatcher design.

- [ ] **DART-STYLE-GENERATION — Correlate URL style callback retirement**
  - Severity: medium
  - Complexity: high
  - Area: BND-124
  - Rationale: accepted URL style loads retain snapshots of custom-geometry
    callback state, while C event payloads do not identify the exact load
    generation. Failed or superseded loads can retain stale callback roots until
    map close. Successful inline JSON replacement and explicit source removal
    use ordered native retirement barriers.
  - Suggested next step: add native style-generation or terminal-load
    correlation to the C event surface, then retire the exact Dart snapshot.

- [ ] **DART-TOKEN-ALIASES — Report loss of all transfer-token aliases**
  - Severity: medium
  - Complexity: high
  - Area: owned-handle leak reporting
  - Rationale: `ResourceRequestToken` is isolate-sendable and may have
    independent copies. The native registry releases it on explicit completion
    or close but cannot identify when every Dart copy has been lost. A finalizer
    on one copy could terminate a request still owned by another isolate, while
    making the token `Finalizable` would make it unsendable.
  - Suggested next step: design registry-backed alias leases or an explicit
    transfer protocol with deterministic last-owner reporting.

- [ ] **DART-BACKEND-HARNESS — Real backend rendering integration coverage**
  - Severity: high
  - Complexity: high
  - Area: BND-160 through BND-173
  - Rationale: CI builds the Dart binding against supported native presets and
    tests descriptor validation and deterministic frame-cleanup seams, but the
    Dart suite does not create real EGL, Vulkan, or Metal contexts for
    successful attach, frame, and readback workflows.
  - Suggested next step: add Dart platform-context test support equivalent to
    the repository's native backend harnesses and exercise each supported
    backend end to end.
