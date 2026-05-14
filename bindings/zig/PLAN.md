# Zig binding implementation plan

## Audience and purpose

This plan is for maintainers and contributors who will implement the initial Zig
binding. It gives a start-to-finish path from the current direct Zig C API tests
to a supported low-level Zig package.

The long-term rules live in the Zig binding conventions document. This plan may
include one-time migration steps, temporary staging, and acceptance checkpoints
that belong to the initial implementation only.

## References

- [Binding conventions](../../docs/src/content/docs/development/bindings.md)
- [Zig binding conventions](../../docs/src/content/docs/development/bindings-zig.md)
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md)
- [Concepts](../../docs/src/content/docs/concepts.md)
- Existing Zig C API tests in `tests/c/`
- Existing Zig examples in `examples/zig-map/` and `examples/zig-readback/`

## Design decisions

The Zig binding should stay close to C without exposing raw C details publicly.
Use one Zig package with private implementation modules, not a Rust-style
`sys/core/public` split.

Key decisions:

- The binding exposes one safe low-level Zig package.
- `@cImport` of `maplibre_native_c.h` stays private to the package.
- Public APIs preserve C API concepts: runtime, map, render session, events,
  resources, style, camera, geometry, and query operations.
- Private helpers cover repeated concerns: status mapping, diagnostics, handle
  liveness, native struct materialization, result copying, callback trampolines,
  and render-target utilities.
- Handles use explicit fallible `close()` methods. Callers manage ownership with
  `defer handle.close() catch ...` in reverse construction order.
- The first implementation avoids parent child counters. Child handles borrow
  parent validity by convention; callers keep parents live while children are
  live. Add targeted guards only for relationships that prove unsafe without
  extra state.
- Zig error tags cannot carry diagnostic payloads. Public operations return
  idiomatic Zig errors and copy native status details into an explicit
  diagnostic store when the caller provides one.
- Existing Zig C API tests are ported milestone by milestone as the public
  binding surface needed by each test exists. Until a test is ported, the direct
  C API test remains the source of coverage. Once a binding test covers the same
  behavior, retire the direct C API version. At the end of the migration, the
  Zig binding suite is the primary Zig-path test suite.

## Target package shape

Start with this layout and adjust names as implementation pressure requires:

```text
bindings/zig/
  PLAN.md
  build.zig
  build.zig.zon
  src/
    maplibre_native.zig      # public package root and re-exports
    c.zig                    # private @cImport only
    status.zig               # status mapping and diagnostics
    diagnostics.zig          # diagnostic store and formatting helpers
    handle.zig               # close-once handle helpers
    memory.zig               # strings, string views, native result guards
    runtime.zig
    map.zig
    projection.zig
    render.zig
    resource.zig
    offline.zig
    style.zig
    logging.zig
    camera.zig
    geo.zig
    query.zig
    events.zig
    value.zig
  tests/
    main.zig
    support.zig
    runtime.zig
    map_lifecycle.zig
    diagnostics.zig
    events.zig
    logging.zig
    style.zig
    style_values.zig
    resources.zig
    geojson.zig
    feature_state.zig
    query.zig
    camera.zig
    projection.zig
    map_tuning.zig
    render_backend.zig
    surface.zig
    texture.zig
    ... backend-specific render tests ...
```

The package should expose a `linkMaplibreNativeC` build helper so consumers,
examples, and tests use the same include paths, library paths, rpaths, and
backend configuration.

## Phase 1: create the package skeleton

1. Add `bindings/zig/build.zig` and `bindings/zig/build.zig.zon`.
2. Add `src/maplibre_native.zig` as the public package root.
3. Add private `src/c.zig` with
   `@cImport({ @cInclude("maplibre_native_c.h"); })`.
4. Add build options for:
   - CMake artifact directory;
   - render backend variant;
   - platform-specific include and library directories;
   - dynamic library rpath for local tests.
5. Add a `zig build test` step that compiles an empty binding test binary
   against `maplibre-native-c`.
6. Add or update mise tasks for `//bindings/zig:build`, `//bindings/zig:test`,
   and `//bindings/zig:ci`.
7. Keep the initial task local to `bindings/zig` until foundational tests pass;
   add root `mise run test` and CI matrix integration in the documentation and
   examples phase.

Acceptance:

- `zig build test` compiles the private C import and links the native library.
- The package root exposes no raw C symbols.
- The build fails clearly when the required native artifact directory or render
  backend option is missing.

## Phase 2: implement status and diagnostics

1. Define the native status error set, for example:

   ```zig
   pub const NativeStatusError = error{
       InvalidArgument,
       InvalidState,
       WrongThread,
       Unsupported,
       NativeError,
       UnknownStatus,
   };
   ```

2. Define a public binding error set that composes native status errors with
   binding-local validation errors, such as closed handles, active borrows,
   invalid string shapes, ABI version mismatch, and already-completed one-shot
   requests.
3. Add a diagnostic record with copied message text and raw status:

   ```zig
   pub const Diagnostic = struct {
       raw_status: ?i32,
       message: []u8,
   };
   ```

4. Add `DiagnosticStore` that owns the latest copied diagnostic with a caller
   supplied allocator.
5. Add `checkStatus(status, diagnostics)` that:
   - returns immediately on `MLN_STATUS_OK`;
   - copies `mln_thread_last_error_message()` before any other C call;
   - stores raw status and message when a diagnostic store is present;
   - maps the status to the stable Zig error tag.
6. Add ABI version validation with diagnostic support.

Acceptance:

- Invalid C calls map to the expected Zig error tag.
- Diagnostics are copied before a later C call can replace the thread-local C
  diagnostic.
- Unknown future status values map to `error.UnknownStatus` and preserve the raw
  status in the diagnostic store.

## Phase 3: implement the runtime and map vertical slice

1. Add `RuntimeHandle` with:
   - private nullable `*c.mln_runtime`;
   - `init`/`create` using native default and explicit options;
   - `runOnce`;
   - `pollEvent` for at least empty or unknown events;
   - fallible idempotent-after-success `close()`.
2. Add `MapHandle` with:
   - private nullable `*c.mln_map`;
   - creation from a live runtime;
   - basic map options;
   - `setStyleUrl` or `setStyleJson`;
   - fallible idempotent-after-success `close()`.
3. Add the first `MapProjectionHandle` slice: creation from a live map,
   standalone snapshot ownership, and fallible close. Add projection operations
   later as their value types land.
4. Implement private live-handle checks that return binding errors before
   crossing into C when a wrapper is already closed.
5. Keep close failure retryable: if native destroy returns a non-OK status,
   leave the pointer live.
6. Validate that no public API leaks raw C handle types.

Acceptance:

- A binding test can create a runtime, create a map, create a projection
  snapshot, run the runtime, and close projection, map, then runtime.
- Closing a handle twice after success is a no-op.
- Using a closed handle returns a Zig error before calling C.
- Wrong-thread native failures propagate as `error.WrongThread` with a copied
  diagnostic when a diagnostic store is present.

## Phase 4: port foundational tests

Port only the tests supported by the runtime and map vertical slice. Binding
tests should call the public Zig binding for the behavior under test. Keep the
direct C API test in `tests/c/` until the binding test covers the same
assertion. Test-private platform scaffolding may still call SDL, Metal, Vulkan,
or OS APIs directly.

Port these first:

1. `diagnostics.zig`
2. `runtime.zig`
3. `map_lifecycle.zig`

During migration:

- Move one test area at a time.
- Implement the binding surface required by that test before or alongside the
  test port.
- Retire exact duplicate direct C API assertions after the binding test
  preserves their intent.
- Keep a small private C import compile test if no binding test exercises header
  bindability early enough.
- Prefer adapting existing assertions over rewriting test intent.

Acceptance:

- Foundational binding tests cover diagnostics, runtime lifecycle, map creation,
  and close behavior through the public Zig API.
- Tests still run against the real `maplibre-native-c` library.
- The binding suite contains no direct `c.mln_*` calls for behavior already
  exposed through the public binding.

## Test migration coverage rule

Port assertions, not just files. Each binding test should preserve the native
behavior and edge cases that the direct Zig C test covered while routing
MapLibre behavior through the public Zig API. During migration, keep direct C
tests until their binding equivalents land. By the end of the migration, the Zig
binding suite replaces the separate direct Zig C suite.

## Phase 5: materialize options, values, and copied results

1. Add public Zig value types for camera, geometry, render extents, style
   images, query options, offline regions, events, and resource metadata as
   needed by the ported tests.
2. Represent C option structs as Zig value descriptors. Use optional fields or
   explicit setters for field-mask domains; materialize C `size` fields and
   masks internally.
3. Add private temporary storage helpers for:
   - null-terminated UTF-8 strings that reject embedded NUL;
   - explicit-length string views;
   - temporary arrays and nested descriptor graphs;
   - output pointer initialization.
4. Add private native result guards for snapshot, list, and query result
   handles. Guards must release native handles after copying, including error
   paths.
5. Add owned output values that deallocate with an explicit `deinit(allocator)`
   or type-specific `deinit` when they own allocations.
6. As each value domain lands, port the matching tests, such as `style.zig`,
   descriptor/value portions of `style_values.zig`, `camera.zig`,
   `projection.zig`, `map_tuning.zig`, and `geojson.zig`. Defer behavioral
   `feature_state.zig` and `query.zig` coverage that needs a render session
   until render targets land.

Acceptance:

- Public callers set semantic fields, not C ABI bookkeeping fields.
- Snapshot/list/result handles remain private implementation details.
- Allocated copied values document and test their deallocation path.
- Ported value and descriptor tests use public binding APIs instead of direct C
  API calls.
- Tests that require rendering remain in the direct C suite only until the
  binding has `RenderSessionHandle` support.

## Phase 6: implement events and source identity

1. Add a binding-assigned `MapId`.
2. Register maps with their runtime for source lookup while both handles are
   live.
3. Copy runtime events into owned Zig values before the next poll.
4. Represent unknown event and payload domains without losing raw values.
5. Apply any binding-owned side effects currently required by Java/Rust, such as
   releasing detached custom geometry source state after style replacement.
6. Port `events.zig` as event copying and source identity become available.

Acceptance:

- Runtime event tests prove copied message and payload data remain valid after a
  later poll.
- Map-originated events carry stable copied source metadata rather than borrowed
  native handles.
- The ported event tests use public binding APIs for runtime and map behavior.

## Phase 7: implement logging, resources, offline, and callbacks

1. Add process-global network status APIs and logging callbacks.
2. Add runtime ambient cache operations.
3. Add offline region create, get, list, metadata update, status, download
   state, observation, invalidation, deletion, and database merge APIs with
   copied value types.
4. Add runtime-scoped resource transform callbacks.
5. Add runtime-scoped resource provider callbacks and `ResourceRequestHandle`.
6. Add map/style-scoped custom geometry source callbacks and state lifetime
   management.
7. Model public callbacks as Zig function pointers plus context pointers first.
   Add typed helper wrappers only after the low-level shape is stable.
8. Keep callback state alive for the native owner scope.
9. Ensure C trampolines use `callconv(.c)`, copy borrowed data before returning,
   convert Zig errors to the documented C callback behavior, and never let a
   failure escape through C frames.
10. Enforce one-shot completion or release for handled resource requests.
11. Port `logging.zig`, `resources.zig`, offline-region tests, and the custom
    geometry portions of `style_values.zig` as those APIs land.

Acceptance:

- Network status and ambient cache operations use public binding APIs.
- Offline region APIs copy native snapshots/lists and release native result
  handles exactly once.
- Callback replacement releases old state after native installation of the new
  state succeeds.
- Failed replacement keeps old callback state active.
- Resource request completion is exactly-once and works inline and, where the C
  API permits, from another thread.
- Custom geometry callback state remains live during active upcalls and releases
  when the owner scope ends.
- Ported callback, resource, offline, and custom geometry tests call the public
  binding for MapLibre behavior.

## Phase 8: implement render targets and readback

1. Add `NativePointer` as a borrowed opaque backend address value.
2. Add public render target descriptors for supported Metal and Vulkan surfaces,
   borrowed textures, and owned textures.
3. Add `RenderSessionHandle` with fallible `close()`.
4. Add readback APIs:
   - `readPremultipliedRgba8Into(buffer: []u8)` for caller-owned storage;
   - an allocator-backed convenience API returning an owned image.
5. Add explicit owned texture frame handles with scoped native pointer access.
6. Port `render_backend.zig`, `surface*.zig`, `texture*.zig`,
   `texture_owned.zig`, behavioral `feature_state.zig`, and behavioral
   `query.zig` coverage to the binding as each render API lands.

Acceptance:

- Render tests cover attach, render update, resize, detach/close, readback, and
  frame access lifetimes through the public binding.
- Feature-state and query tests that require a rendered map use public binding
  render sessions.
- Backend handles remain borrowed and opaque at the public API boundary.
- Platform scaffolding stays test-private; MapLibre behavior goes through the
  public binding.

## Phase 9: update examples and documentation

1. Port `examples/zig-readback` to the binding once readback APIs exist.
2. Port `examples/zig-map` after render target and event APIs stabilize.
3. Update contributor docs and command lists for the new binding tasks.
4. Add the Zig binding suite to root `mise run test` after foundational tests
   pass and the first direct C test group has been replaced.
5. Update `.github/config/variants.toml` and workflow task lists so CI runs the
   Zig binding on supported platform/render-backend variants with explicit
   exclusions.
6. Keep examples small and focused on low-level binding usage.

Acceptance:

- Examples import the binding package instead of direct `@cImport` of the C API.
- Root test commands and CI run the Zig binding suite where the native variant
  is supported.
- Documentation points contributors to conventions for long-term decisions and
  to this plan only for initial implementation history while it remains useful.

## Definition of done

The Zig binding is ready for broad contributor work when:

- the public package exposes no raw C declarations;
- relocated Zig binding tests pass through public binding APIs;
- the Zig binding suite covers the behavior previously covered by the direct Zig
  C API tests;
- handle close, diagnostic capture, copied output, callback state, and render
  frame lifetimes have tests;
- examples use the binding package;
- `mise run test` includes the Zig binding suite in the normal project path;
- conventions describe ongoing rules without depending on this initial plan.
