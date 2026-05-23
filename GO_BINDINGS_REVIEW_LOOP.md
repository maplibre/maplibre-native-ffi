# Go bindings review loop record

This file preserves the review-loop disposition for the `golang` branch/PR. It
summarizes the code-review rounds, applied fixes, rejected/deferred findings,
user-input-needed findings, and validation used to close the iterative review
loop.

## Prior review-loop state

- Branch: `golang`, pushed to `origin/golang`.
- The original review loop reached no-actionable state at Round 13 after commit
  `4aec32e` (`Correct Go texture frame spec names`). Later commits implement the
  previously deferred outcome decisions and start a new review loop over those
  changes.
- Round 13 automated validation passed:
  - `mise run //bindings/go:ci` passed.
  - `git diff --check origin/main...HEAD` passed.
  - `git status --short --branch` reported a clean branch matching
    `origin/golang`.
- Round 13 completed with correctness, tests/validation, and
  maintainability/API-docs reviewers. All three reported no actionable
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

## Deferred finding interview outcomes and implementation

These findings were out of scope for the completed review loop because they
needed explicit lifetime or API decisions. The follow-up interview resolved the
next design direction for each finding, and the first implementation pass
applied those decisions.

### Resource-transform replacement URL storage

Decision: investigate native ownership before changing Go storage. Native
`invoke_resource_transform` copies `out_response->url` into a `std::string`
during the same transform invocation, immediately after the callback returns.
The Go binding now uses a C bridge callback with per-thread replacement URL
storage. The Go callback returns a call-scoped C string, the C bridge copies it
into thread-local storage before returning to native, and native copies that
thread-local URL before the next callback can reuse the same thread-local slot.
This removes the previous unbounded Go-side append-only URL retention.

### Custom geometry callbacks after asynchronous `SetStyleURL`

Decision: mirror the Java FFM binding lifecycle. Go now retains custom geometry
callback state on the map, releases it on explicit source removal,
`SetStyleJSON`, and map close, and releases detached custom geometry sources
after asynchronous `SetStyleURL` when `RuntimeHandle.PollEvent` observes
`RuntimeEventMapStyleLoaded`. Runtime handles keep a private map registry so the
event source can be mapped back to its `MapHandle`.

Cleanup is best-effort, matching Java FFM. On `RuntimeEventMapStyleLoaded`, Go
queries each retained custom geometry source. It releases callback state when
the source is missing or its current type is not `StyleSourceTypeCustomVector`.
If a source-type query fails, Go keeps the callback state for a later event or
map close; `PollEvent` still delivers the event rather than failing because
cleanup had a query error.

## User-input-needed findings

The interview resolved the preferred directions above, and the first
implementation pass applied them. No currently recorded deferred finding is
waiting on user input.

## Prior no-actionable review evidence

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
