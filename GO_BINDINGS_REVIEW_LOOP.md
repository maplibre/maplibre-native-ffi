# Go bindings review loop record

This file preserves the review-loop disposition for the `golang` branch/PR. It
summarizes the code-review rounds, applied fixes, rejected/deferred findings,
user-input-needed findings, and validation used to close the iterative review
loop.

## Final state

- Branch: `golang`, pushed to `origin/golang`.
- Latest fully reviewed non-record commit: `4aec32e`
  (`Correct Go texture frame
  spec names`). If this file is later adjusted by a
  record-only commit, that commit cannot self-reference its own final hash;
  treat the containing commit as the current record commit and `4aec32e` as the
  latest reviewed source/doc fix.
- Final automated validation after the latest source/doc fix:
  - `mise run //bindings/go:ci` passed.
  - `git diff --check origin/main...HEAD` passed.
  - `git status --short --branch` reported a clean branch matching
    `origin/golang`.
- Final code-review loop: Round 13 completed with correctness, tests/validation,
  and maintainability/API-docs reviewers. All three reported no actionable
  source/doc findings remaining.

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
- Round 11: post-refresh review; no Go binding code, validation, or API-doc
  findings remained. The only issue was this record's stale exact commit and
  final-round wording, addressed by a record-only update.
- Round 12: final-final review found one small SPEC accuracy issue; corrected
  session-owned texture frame type names from non-existent `*Handle` names to
  `MetalOwnedTextureFrame` and `VulkanOwnedTextureFrame`.
- Round 13: final source/doc review after the SPEC correction; correctness,
  tests/validation/CI, and maintainability/API-docs reviewers all reported no
  actionable source/doc findings remaining.

## Rejected or deferred findings

- Real resource-provider integration test: deferred. It is valuable, but it
  exercises native loading/event timing and is broader than the focused review
  fixes in this branch. Current coverage includes lifecycle validation, public
  NUL validation, and internal provider panic/fallback paths.
- Rename the cgo runtime-event test hook to `*_test.go`: rejected. Local CI
  showed cgo declarations in that test file are unsupported for this
  package/platform, so the hook remains a normal unexported package file with an
  explanatory test-only comment.

## Deferred finding interview outcomes

These findings were out of scope for the completed review loop because they
needed explicit lifetime or API decisions. The follow-up interview resolved the
next design direction for each finding without implementing the changes.

### Resource-transform replacement URL storage

Direction: investigate native ownership before changing Go storage. The desired
contract is that native code copies a replacement URL during the transform
callback. If inspection shows native already does this, Go can bound replacement
URL storage to the callback scope. If native still borrows past the callback,
first assess whether making C/C++ copy immediately is small and ABI-compatible.
If that native change is broad, keep Go's conservative retention and document
the required C lifetime change for a later design pass.

### Custom geometry callbacks after asynchronous `SetStyleURL`

Decision: mirror the Java FFM binding lifecycle. Go should retain custom
geometry callback state on the map, release it on explicit source removal,
`SetStyleJSON`, and map close, and release detached custom geometry sources
after asynchronous `SetStyleURL` when `RuntimeHandle.PollEvent` observes
`RuntimeEventMapStyleLoaded`. This requires a runtime map registry so the event
source can be mapped back to its `MapHandle`.

Cleanup should be best-effort, matching Java FFM. On
`RuntimeEventMapStyleLoaded`, query each retained custom geometry source.
Release callback state when the source is missing or its current type is not
`StyleSourceTypeCustomVector`. If a source-type query fails, keep the callback
state for a later event or map close; `PollEvent` should still deliver the event
rather than fail because cleanup had a query error.

## User-input-needed findings

The interview resolved the preferred directions above. Implementation still
requires code changes and validation, so these items remain deferred until a
follow-up implementation pass.

- Bounded resource-transform replacement URL storage needs native ownership
  investigation before Go storage changes.
- URL-style custom-geometry callback release should mirror Java FFM with
  event-driven, best-effort cleanup after `RuntimeEventMapStyleLoaded`.

## Final no-actionable review evidence

Round 13 reviewer conclusions:

- Correctness/ownership/cgo/concurrency: no actionable source/doc findings
  remained; handle ownership, render frame guards, callback/resource ownership,
  runtime event copying, and the SPEC texture frame name correction were
  checked.
- Tests/validation/CI: no actionable findings remained;
  `mise run
  //bindings/go:ci`, `git diff --check origin/main...HEAD`, and
  clean branch status were verified, and Go CI/test coverage was inspected.
- Maintainability/API docs/SPEC accuracy/scope hygiene: no actionable findings
  remained; the SPEC texture frame names now match the public Go types.

No Round 13 source/doc fixes were needed.
