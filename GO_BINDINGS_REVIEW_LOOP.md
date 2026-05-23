# Go bindings review loop record

This file preserves the review-loop disposition for the `golang` branch/PR. It
summarizes the code-review rounds, applied fixes, rejected/deferred findings,
user-input-needed findings, and validation used to close the iterative review
loop.

## Final state

- Latest reviewed/pushed commit: `b7f63ce` (`Document Go C payload test hook`).
- Branch: `golang`, pushed to `origin/golang`.
- Final automated validation after the latest commit:
  - `mise run //bindings/go:ci` passed.
  - `git diff --check origin/main...HEAD` passed.
  - `git status --short --branch` reported a clean branch matching
    `origin/golang`.
- Final code-review loop: Round 9 completed with correctness, tests/validation,
  and maintainability/API-docs reviewers. All three reported no actionable
  findings remaining.

## Applied findings by round

- Round 1: serialized native handle close state; serialized render-session frame
  state transitions; rejected embedded NUL strings in resource responses;
  expanded callback docs; corrected SPEC JSON semantics.
- Round 2: kept readback buffers alive across cgo; added callback panic-recovery
  tests for logging, custom geometry, resource transform, and resource provider;
  expanded logging callback docs.
- Round 3: expanded custom geometry callback docs; expanded session-owned
  texture frame docs; replaced stale exact SPEC file tree with a high-level
  implementation summary.
- Round 4: documented render-target/backend-handle lifetimes, synchronization,
  owner-thread, presentation/layout, detach, and session-close requirements.
- Round 5: guarded texture readback while a session-owned frame is acquired;
  moved owned texture frame close state behind shared unexported state so copied
  frame values cannot double-release.
- Round 6: copied known map runtime event payloads (render frame, render map,
  style image missing, tile action); added public conversion tests; corrected
  SPEC ownership wording for `internal/callback` and removed stale map-registry
  wording.
- Round 7: added focused `internal/capi` coverage that constructs C-shaped
  runtime event payload structs and verifies Go copying.
- Round 8: documented why the cgo runtime-event test hook lives in a non-test
  file and must remain small/test-only.
- Round 9: final review-only loop; no fixes needed.

## Rejected or deferred findings

- Real resource-provider integration test: deferred. It is valuable, but it
  exercises native loading/event timing and is broader than the focused review
  fixes in this branch. Current coverage includes lifecycle validation, public
  NUL validation, and internal provider panic/fallback paths.
- Rename the cgo runtime-event test hook to `*_test.go`: rejected. Local CI
  showed cgo declarations in that test file are unsupported for this
  package/platform, so the hook remains a normal unexported package file with an
  explanatory test-only comment.

## User-input-needed findings

These are recorded for future design work rather than silently implemented in
this review loop. They require explicit C/Go lifetime or API decisions before a
safe bounded implementation.

- Bounded resource-transform replacement URL storage needs an explicit C/Go
  lifetime design.
- URL-style custom-geometry callback release timing after asynchronous
  `SetStyleURL` needs lifecycle/API design input.

## Final no-actionable review evidence

Round 9 reviewer conclusions:

- Correctness/ownership/cgo/concurrency: no actionable findings remain.
- Tests/validation/CI: no actionable findings remain.
- Maintainability/API docs/SPEC accuracy/scope hygiene: no actionable findings
  remain.
