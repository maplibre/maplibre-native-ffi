# Go bindings review loop record

This file preserves the review-loop disposition for the `golang` branch/PR. It
summarizes the code-review rounds, applied fixes, rejected/deferred findings,
user-input-needed findings, and validation used to close the iterative review
loop.

## Final state

- Branch: `golang`, pushed to `origin/golang`.
- Latest fully reviewed non-record commit: `82ce49e`
  (`Refresh Go review loop
  record`). If this file is later adjusted by a
  record-only commit, that commit cannot self-reference its own final hash;
  treat the containing commit as the current record commit and `82ce49e` as the
  latest reviewed source/doc fix.
- Final automated validation after the latest source/doc fix:
  - `mise run //bindings/go:ci` passed.
  - `git diff --check origin/main...HEAD` passed.
  - `git status --short --branch` reported a clean branch matching
    `origin/golang`.
- Final code-review loop: Round 11 completed with correctness, tests/validation,
  and maintainability/API-docs reviewers. All three found no Go binding code,
  validation, or API-doc findings remaining; the only record-accuracy finding
  was to make this tracked record avoid stale latest-commit/final-round
  metadata, which this record-only update applies.

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
- Round 9: review-only loop; no code fixes needed.
- Round 10: completion review after preserving this tracked record; fixed stale
  record metadata and one stale public doc comment.
- Round 11: final post-refresh review; no Go binding code, validation, or
  API-doc findings remained. The only issue was this record's stale exact commit
  and final-round wording, addressed by this record-only update.

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

Round 11 reviewer conclusions:

- Correctness/ownership/cgo/concurrency: no Go binding code issue remained; the
  tracked review record needed stale exact-commit/final-round wording refreshed.
- Tests/validation/CI: no tests or CI issue remained; the tracked review record
  needed stale exact-commit/final-round wording refreshed.
- Maintainability/API docs/SPEC accuracy/scope hygiene: no API-doc issue
  remained; the tracked review record needed stale exact-commit/final-round
  wording refreshed.

Those Round 11 record findings were applied by this record-only update.
