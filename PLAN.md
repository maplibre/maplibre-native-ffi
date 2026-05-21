# Plan: redesign offline database bindings around the async C API

Audience: contributors implementing or reviewing the binding redesign. Category:
guide.

## Problem statement

The branch changes the C offline database API from synchronous calls to async
operations. That C shape is acceptable: callers start an operation, receive an
`mln_offline_operation_id`, drive the runtime, observe
`MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED`, and then take or discard the
operation result.

The binding changes do not preserve that model. Java FFM, Rust, and Zig expose
mostly synchronous methods that call `*_start`, spin `run_once`/`poll_event` in
a private loop, sleep for 1 ms, consume the matching completion event, and then
call `*_take_result` or `discard`. This moves the wait up a layer instead of
redesigning the bindings for the new API.

## Goals

- Expose the async operation lifecycle directly in each low-level binding.
- Keep runtime pumping and event polling under the host's control.
- Make offline operation completion visible through normal runtime events.
- Model operation IDs, operation kinds, result kinds, and completion payloads as
  typed binding values while preserving unknown raw values.
- Make result ownership explicit: start returns an `OfflineOperationHandle`,
  result methods consume completed successful operations, and discard suppresses
  or releases operations that the caller no longer wants.
- Remove blocking compatibility helpers from the low-level bindings.

## Non-goals

- Do not add schedulers, futures, coroutines, executors, UI dispatch, or thread
  hopping to the low-level bindings.
- Do not make `discard` sound like cancellation. The C API suppresses completion
  delivery and drops stored state; it does not cancel native database work.
- Do not hide completion events from callers.
- Do not keep synchronous offline database helper APIs in the low-level
  bindings.
- Do not add compatibility shims for the old C ABI while `mln_c_version()` is
  still `0`.

## Current issues to fix

### Blocking wait loops

Remove the private wait loop from public offline database methods:

- Java FFM: `RuntimeHandle.waitForOfflineOperation()` loops
  `mln_runtime_run_once`, `mln_runtime_poll_event`, and `Thread.sleep(1)`.
- Rust: `RuntimeHandle::wait_for_offline_operation()` loops `run_once`,
  `mln_runtime_poll_event`, and `std::thread::sleep(Duration::from_millis(1))`.
- Zig: `RuntimeHandle.waitForOfflineOperation()` loops `runOnce`,
  `mln_runtime_poll_event`, and `usleep(1000)`.

These loops violate the binding conventions: low-level bindings preserve the C
API model, and higher-level adapters own scheduling policy.

### Consumed completion events

The blocking helpers consume the matching offline operation completion event.
That makes `pollEvent()` incomplete for callers and undermines the C contract
that completion is reported through runtime events.

### Missing operation surface

Add public start, take, and discard APIs. Today the bindings either expose no
`OfflineOperationHandle` or expose only raw IDs internally.

### Incomplete typed event mapping

Rust currently lacks public mapping for
`MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED` and
`MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED`. Java and Zig expose the
payload, but operation kind and result kind are still raw integers.

### Incorrect cleanup after take failures

The C docs say `*_take_result()` consumes the operation entry on failure.
Binding code must not call `discard` after a failed take and mask the original
error. For async completion with a non-OK `result_status`, the caller should
surface that status and discard the operation if no result will be taken.

## Target binding model

### Operation handle

Each binding should expose an `OfflineOperationHandle`. This is not a C opaque
handle; it is a binding-level lifecycle handle for runtime-owned operation state
keyed by `mln_offline_operation_id`. The `Handle` suffix is intentional because
callers must eventually take or discard it, or the runtime keeps operation state
until runtime teardown.

The handle contains:

- operation ID;
- expected operation kind;
- expected result kind;
- runtime association;
- live, consumed, or discarded state.

The handle must stay owner-thread-affine because start, take, and discard are
runtime owner-thread APIs.

### Start methods

Start methods should only validate binding-owned inputs, materialize temporary C
storage, call the matching C `*_start` function, and return the
`OfflineOperationHandle`. They must not pump the runtime, poll events, sleep,
take results, or discard.

Required starts:

- ambient cache operation;
- create offline region;
- get offline region;
- list offline regions;
- merge offline regions database;
- update offline region metadata;
- get offline region status;
- set offline region observed;
- set offline region download state;
- invalidate offline region;
- delete offline region.

### Completion

Callers drive progress with the existing runtime methods:

1. call a start method;
2. call `runOnce()` as their event loop requires;
3. call `pollEvent()` until they see `OfflineOperationCompleted` with the
   matching operation ID;
4. if `result_status` is OK and the operation has a result, call the matching
   take method;
5. if the operation has no result or completed with an error, call discard when
   they no longer need the operation state.

### Take methods

Expose typed take methods for result-bearing operations:

- create region -> copied `OfflineRegionInfo`;
- get region -> optional copied `OfflineRegionInfo`;
- list regions -> copied list;
- merge database -> copied list;
- update metadata -> copied `OfflineRegionInfo`;
- get status -> copied `OfflineRegionStatus`.

Take methods return host-owned copied values. Native C result handles such as
`mln_offline_region_snapshot*` and `mln_offline_region_list*` stay private to
the binding: call the C `*_take_result`, copy the snapshot or list into language
values, destroy the native snapshot/list handle immediately, and then return the
copied value. Java and Rust callers should not receive another native cleanup
handle. Zig results may own allocator-backed host memory and keep their existing
`deinit()` responsibilities, but they must not wrap native snapshot/list
handles.

A take method should verify that the `OfflineOperationHandle` is live and has
the expected kind/result kind before calling C. On any returned status, mark the
operation consumed exactly as the C API specifies.

### Discard

Expose explicit discard for any live operation. Discard may be used before
completion to suppress later completion delivery, or after completion for
result-less and failed operations. The API name and docs should say "discard" or
"ignore", not "cancel".

### Blocking helpers

Remove the current synchronous offline database helpers from the low-level
bindings. A method that starts offline database work must return an
`OfflineOperationHandle` immediately. A method that takes or discards an
operation must never pump the runtime or sleep. Higher-level adapters may build
blocking or async-friendly convenience APIs above this layer.

## Java FFM plan

Files likely touched:

- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/runtime/RuntimeHandle.java`
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/runtime/RuntimeEventPayload.java`
- new operation types under `org.maplibre.nativeffi.offline` or
  `org.maplibre.nativeffi.runtime`
- `bindings/java-ffm/src/test/java/org/maplibre/nativeffi/runtime/*Offline*Test.java`

Steps:

1. Add typed operation metadata:
   - `OfflineOperationKind`;
   - `OfflineOperationResultKind`;
   - `OfflineOperationHandle` class with ID, kind, result kind, runtime owner,
     and consumed/discarded state. It should implement `AutoCloseable` so
     `close()` discards a live operation on the runtime owner thread.
2. Change `RuntimeEventPayload.OfflineOperationCompleted` to expose typed kind,
   typed result kind, raw kind, raw result kind, raw status, mapped status, and
   `found`.
3. Add non-blocking start methods to `RuntimeHandle`, for example:
   - `startAmbientCacheOperation(AmbientCacheOperation operation)`;
   - `startCreateOfflineRegion(OfflineRegionDefinition definition, byte[] metadata)`;
   - `startOfflineRegion(long id)`;
   - `startOfflineRegions()`;
   - `startMergeOfflineRegionsDatabase(String path)`;
   - `startUpdateOfflineRegionMetadata(long id, byte[] metadata)`;
   - `startOfflineRegionStatus(long id)`;
   - `startSetOfflineRegionObserved(long id, boolean observed)`;
   - `startSetOfflineRegionDownloadState(long id, OfflineRegionDownloadState state)`;
   - `startInvalidateOfflineRegion(long id)`;
   - `startDeleteOfflineRegion(long id)`.
4. Add explicit take methods on `RuntimeHandle` or on typed operation wrappers.
   Keep FFM `MemorySegment` details internal. Take methods should return copied
   Java records, optionals, or lists and destroy native snapshot/list result
   handles internally.
5. Make `discardOfflineOperation` public enough for low-level users. It should
   mark the operation discarded only after native discard succeeds. Java
   cleaners must not call runtime-affine discard from an arbitrary GC thread;
   use a leak report if an operation is abandoned without explicit discard or
   take.
6. Remove `waitForOfflineOperation` and delete the synchronous offline database
   helper methods from the low-level API.
7. Fix cleanup so failed `take_result` calls do not trigger a second discard
   that masks the original error.
8. Add Java tests for start without blocking, manual completion polling, typed
   completion payloads, take methods, and discard behavior.

## Rust plan

Files likely touched:

- `bindings/rust/crates/maplibre-native-core/src/enums.rs`
- `bindings/rust/crates/maplibre-native-core/src/events.rs`
- `bindings/rust/crates/maplibre-native-core/src/runtime.rs`
- `bindings/rust/crates/maplibre-native/src/runtime.rs`
- Rust runtime/offline tests in the same crates

Steps:

1. In `maplibre-native-core`, add:
   - `OfflineOperationKind` with `Unknown(u32)`;
   - `OfflineOperationResultKind` with `Unknown(u32)`;
   - `RuntimeEventType::OfflineOperationCompleted`;
   - `RuntimeEventPayload::OfflineOperationCompleted` with copied payload data.
2. In `maplibre-native`, add owner-thread-affine operation handles. A generic
   shape is preferable:
   - `OfflineOperationHandle<T>` for result-bearing operations;
   - marker result types for region, optional region, region list, status, and
     no-result operations.
3. Store `Rc<RuntimeState>` in the operation handle so the runtime remains live
   while operations are live. Keep the handle `!Send`/`!Sync` through the same
   thread-affine pattern as other runtime objects.
4. Add `start_*` methods that return `OfflineOperationHandle` values and do not
   call `wait_for_offline_operation`.
5. Add typed `take(self)` methods for result-bearing operation handles. A take
   consumes `self`, calls the matching C `*_take_result`, copies the native
   result, destroys native snapshot/list handles internally, returns normal
   Rust-owned values, and marks the operation consumed.
6. Add `discard(self) -> Result<()>` for all operation handles. Also implement
   `Drop` to discard a still-live operation, matching the existing Rust RAII
   policy for thread-affine native handles and texture frame guards. Safe Rust
   keeps the handle on the owner thread through `Rc` and `!Send`/`!Sync` state;
   explicit discard returns native errors, while `Drop` cannot report errors and
   ignores non-OK discard statuses. Prefer explicit discard in examples and
   tests.
7. Replace the current synchronous offline database methods with async start,
   take, and discard APIs. Delete `wait_for_offline_operation` from the public
   API path.
8. Add Rust tests for typed event mapping, non-blocking starts, manual
   run/poll/take flow, discard before completion, discard after failed
   completion, and wrong-result take errors.

## Zig plan

Files likely touched:

- `bindings/zig/src/runtime.zig`
- `bindings/zig/src/maplibre_native.zig`
- `bindings/zig/tests/resources.zig`

Steps:

1. Add typed operation metadata values:
   - `OfflineOperationKind`;
   - `OfflineOperationResultKind`;
   - typed fields or accessors on `OfflineOperationCompletedPayload` while
     preserving raw values.
2. Add an `OfflineOperationHandle` resource struct containing the operation ID,
   expected kind/result kind, `*RuntimeHandle`, and state. Document that callers
   pass it by pointer and do not copy it.
3. Add start methods on `RuntimeHandle` that return `OfflineOperationHandle`
   without calling `waitForOfflineOperation`.
4. Add take methods that consume `*OfflineOperationHandle` and return existing
   Zig owned values:
   - `OwnedOfflineRegion`;
   - `?OwnedOfflineRegion`;
   - `OfflineRegionList`;
   - `OfflineRegionStatus`.
5. Add `discard()` on `OfflineOperationHandle`. It should call
   `mln_runtime_offline_operation_discard`, mark the handle discarded on
   success, and leave it live on failure so callers can retry or report the
   diagnostic. Zig has no destructor hook, so cleanup is explicit through
   `discard()` or a successful take. Take methods should return existing
   host-owned Zig values and destroy native snapshot/list result handles
   internally.
6. Remove `waitForOfflineOperation` and delete the synchronous offline database
   helper methods from the low-level API.
7. Consider copying payload bytes before interpreting offline operation payloads
   rather than casting `const void*` directly, so event decoding does not depend
   on payload alignment.
8. Add Zig tests for non-blocking starts, manual event completion, typed
   completion payloads, and take/discard state.

## Test plan

Run the full suite after the redesign:

```sh
mise run test
```

Add targeted tests in each binding before relying on the full suite:

1. Start returns a non-zero `OfflineOperationHandle` before any manual runtime
   pumping.
2. Manual `runOnce()` + `pollEvent()` surfaces `OfflineOperationCompleted` with
   the matching operation ID.
3. Successful completion plus matching take returns copied language-owned data.
4. Non-OK completion maps to the binding's normal error type and leaves cleanup
   to discard, Java `close()`, Rust `Drop`, or Zig explicit discard.
5. Discard before completion suppresses later completion delivery for that
   operation.
6. Discard after successful take fails or no-ops according to the
   `OfflineOperationHandle` state, without calling C with an already-consumed
   operation ID.
7. Wrong take method on a completed operation reports invalid state and consumes
   the operation entry according to the C docs.
8. Unknown operation kind/result kind values preserve raw values in event
   payloads.
9. Existing resource, map, and render event polling still preserves unrelated
   event ordering.

## Suggested implementation order

1. Add typed operation kind/result kind and completion event mapping in all
   bindings.
2. Add `OfflineOperationHandle` state machines and discard APIs.
3. Add non-blocking start APIs.
4. Add typed take APIs.
5. Delete synchronous offline database methods from the low-level bindings.
6. Add tests for each language.
7. Run `mise run test`, then `mise run fix`.

## Acceptance criteria

- Java FFM, Rust, and Zig each expose a low-level async offline operation API.
- No public low-level offline database method spins, sleeps, or privately pumps
  the runtime.
- Runtime event polling can observe offline operation completion events.
- Operation completion payloads are typed and preserve unknown raw values.
- Result-bearing operations use explicit take methods; result-less or unwanted
  operations use explicit discard.
- Take methods return host-owned copied values and do not expose native
  snapshot/list result handles to binding callers.
- The low-level bindings contain no synchronous offline database helpers that
  pump, poll, sleep, or consume completion events internally.
- Tests cover the manual start, pump, poll, take/discard workflow through the
  real C ABI where practical.
