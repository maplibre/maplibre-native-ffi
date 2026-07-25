# Dart binding review notes

This file records findings deferred while updating the `dart` branch to `main`
at `3de8ea9763`. The binding is prerelease; these items remain design work
rather than compatibility commitments.

## Native owner-thread executor

- Severity/complexity: high/high
- Requirements: owner-thread execution model and BND-192
- Finding: a Dart isolate is a logical concurrency boundary, while the Dart VM
  may resume it on another operating-system thread after an asynchronous
  suspension. Isolate identity checks therefore catch cross-isolate misuse but
  cannot guarantee that later direct FFI calls run on the C handle's native
  owner thread.
- Current behavior: synchronous methods preserve the C API's direct error
  semantics and surface `MLN_STATUS_WRONG_THREAD`.
- Deferral rationale: a complete adapter needs a native-bound dispatcher that
  owns runtime creation, pumping, operations, event copying, and destruction.
  Adding isolated wrappers around individual methods would leave gaps in the
  handle surface.
- Follow-up owner: Dart binding maintainers, coordinated with the binding
  specification owners.

## General synchronous resource callbacks

- Severity/complexity: high/high
- Requirements: resource callback rules and BND-140 through BND-153
- Finding: Dart `NativeCallable.listener` safely accepts calls from arbitrary
  native worker threads but returns `void` asynchronously. The binding therefore
  exposes native-owned exact URL rewrite/response tables and queued providers
  with native-owned routing. This supports safe asynchronous completion, but it
  does not expose a general Dart transform callback or let a queued Dart
  provider select pass-through after inspecting every request.
- Deferral rationale: the full public shape needs a native synchronous decision
  layer or the native owner-thread executor above. Calling Dart synchronously
  from arbitrary MapLibre worker threads is outside Dart's callback contract.
- Follow-up owner: Dart binding maintainers.

## URL style callback generation

- Severity/complexity: medium/high
- Requirement: BND-124
- Finding: accepted URL style loads retain snapshots of custom-geometry callback
  state, while the current C event payload does not identify the exact load
  generation. Failed or superseded loads can retain stale callback roots until
  map close. Successful inline JSON replacement and explicit source removal use
  ordered native retirement barriers.
- Deferral rationale: exact retirement needs native style-generation or terminal
  load correlation in the C event surface. FIFO event inference can retire the
  wrong source state when loads overlap.
- Follow-up owner: C API and Dart binding maintainers.

## Transfer-token loss reporting

- Severity/complexity: medium/high
- Requirements: owned-handle leak reporting and BND-151
- Finding: `ResourceRequestToken` is isolate-sendable and may have independent
  copies. The native registry releases it on explicit completion or close, but
  cannot currently report when every Dart copy is lost.
- Deferral rationale: a finalizer on one copy could terminate a request still
  owned by another isolate, and making the token `Finalizable` would make it
  unsendable. Safe reporting needs registry-backed alias leases or an explicit
  transfer protocol.
- Follow-up owner: Dart binding maintainers.

## Backend rendering integration coverage

- Severity/complexity: high/high
- Requirements: BND-160 through BND-173
- Finding: CI builds the Dart binding against the supported native presets and
  tests render descriptor validation, but the Dart suite does not yet create
  real EGL, Vulkan, or Metal platform contexts for successful attach, frame, and
  readback workflows.
- Deferral rationale: deterministic coverage needs Dart platform-context test
  support equivalent to the repository's Python and native backend harnesses,
  plus explicit failure seams for acquire/release construction paths.
- Follow-up owner: Dart binding maintainers.

## Remaining lifecycle and event coverage

- Severity/complexity: high/high
- Requirements: callback, offline, event, and resource lifecycle matrices
- Finding: the native-backed suite covers copied style-event identity, queue
  exhaustion, offline completion identity, cross-isolate request completion,
  callback exception containment, and callback replacement. Deterministic
  failure seams are still needed for cancellation races, lost transfer-token
  aliases, superseded URL style loads, and frame construction/release failures.
- Deferral rationale: these cases need native test hooks that force precise race
  and failure points. Timing-only tests would not provide stable coverage.
- Follow-up owner: Dart binding maintainers.

## Callback retirement barrier decision

- Audit disposition: reviewed and resolved
- A reviewer raised a possible race between listener retirement sentinels and
  native worker invocations. The provider state is retired only after maps are
  destroyed with the runtime, and custom-geometry state is retired after source,
  style, or map teardown. Those native teardown points synchronously end
  callback reachability; MapLibre actor destruction waits until the actor can
  receive no more messages. The sentinels are invoked afterwards through the
  same listener ports and therefore form ordered queue barriers for
  already-posted Dart deliveries. Both custom fetch and cancel ports acknowledge
  retirement before their roots close.
