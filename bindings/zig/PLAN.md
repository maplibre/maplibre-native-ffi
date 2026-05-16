# Zig lifecycle handle refactor plan

## Goal

Use Zig-owned lifecycle structs around C-owned native objects.

The Zig binding should match the Java and Rust ownership shape:

- C owns long-lived runtime, map, projection, and render-session objects.
- Zig owns small wrapper structs that contain native pointers and Zig adapter
  state.
- Callers explicitly close wrappers in reverse dependency order.
- After `close()` succeeds, the same wrapper value is inert; copied wrapper
  values are invalid.

This replaces the current pointer-backed enum handle model. The enum model makes
wrapper-state lifetime ambiguous: freeing the cell creates use-after-free risks
for copied handles, while keeping the cell leaks wrapper state.

## Target API shape

Lifecycle handles become structs, not enum IDs:

```zig
var runtime = try maplibre.RuntimeHandle.init(null);
defer runtime.close() catch @panic("runtime close failed");

var map = try maplibre.MapHandle.create(&runtime, .{});
defer map.close() catch @panic("map close failed");

var session = try maplibre.attachOwnedTexture(&map, .{
    .extent = .{ .width = 32, .height = 16, .scale_factor = 1.0 },
});
defer session.close() catch @panic("session close failed");
```

Methods that mutate lifecycle wrapper state take `*Handle`. Methods that only
inspect wrapper state or call native read APIs may take `*const Handle` when
that keeps call sites clear. Prefer pointer receivers for lifecycle handle
methods so method calls do not copy owned wrapper state.

A successful `close()`:

1. calls the matching C destroy/clear operation;
2. releases Zig-owned adapter state;
3. stores `null` in the wrapper's native pointer;
4. leaves the same wrapper safe to close again as a no-op.

A failed `close()` leaves the native pointer and Zig adapter state live so the
caller can retry.

## Ownership rules

- Treat lifecycle handles and texture frame handles like `std.ArrayList`: they
  are ordinary Zig values but are owned resources. Do not copy them and then use
  both copies.
- Share lifecycle and texture frame handles by pointer.
- Keep parents live while children are live.
- Close children before parents:
  - texture frames before render sessions;
  - render sessions and projections before maps when applicable;
  - maps before runtimes.
- Keep C callback user-data pointers stable by allocating callback state on the
  heap when native code can call it later. Do not pass pointers to movable Zig
  lifecycle structs as long-lived C `user_data`.

## Non-goals

- Do not introduce generation-checked registries for normal lifecycle handles.
  They add machinery to preserve stale copied-handle behavior that Zig does not
  normally promise for owned resource structs.
- Do not make Zig own native runtime/map/render-session storage. The C API keeps
  owning those objects.
- Do not add automatic destruction from finalizers. Zig callers explicitly close
  fallible, thread-affine native objects.

## Implementation steps

### 1. RuntimeHandle

- Replace the pointer-backed enum with a struct containing runtime-owned state:
  - `native: ?*c.mln_runtime`
  - `diagnostic_store: ?*diagnostics.DiagnosticStore`
  - stable map registration state, for example `?*RuntimeRegistry`
  - `resource_transform: ?*ResourceTransform`
  - resource-provider callback state, likely `?*ResourceProviderState`
- Remove `runtimeHandleFromState()` and `state(RuntimeHandle)`.
- Change `init()` and `create()` to return `RuntimeHandle` values directly.
- Change `close()` to `pub fn close(self: *RuntimeHandle) status.Error!void`.
- Change public runtime methods to pointer receivers.
- Change helper functions such as `native`, `diagnosticStore`, `registry`,
  `registerMap`, and `unregisterMap` to use runtime pointers or the stable
  runtime registry.
- Move resource-provider callback storage out of `RuntimeHandle` if native code
  needs a stable `user_data` pointer:
  - allocate a `ResourceProviderState` on `setResourceProvider()`;
  - pass that heap state as native `user_data`;
  - free the previous state only after native replacement succeeds;
  - free current provider state during runtime close after native teardown has
    stopped callbacks.

### 2. MapHandle

- Replace the pointer-backed enum with a struct containing the current
  `MapState` fields inline:
  - `native: ?*c.mln_map`
  - stable runtime registration state used to unregister the map
  - `id: values.MapId`
  - `diagnostic_store: ?*diagnostics.DiagnosticStore`
  - `custom_geometry_sources: std.ArrayList(*CustomGeometrySourceState)`
- Remove `mapHandleFromState()` and `state(MapHandle)`.
- Change `MapHandle.create()` to accept `*RuntimeHandle` and return a
  `MapHandle` value.
- Change public map methods to pointer receivers.
- Keep custom-geometry source callback state heap allocated, because native code
  stores those callback user-data pointers independently of the map wrapper.
- On successful close, destroy the native map, unregister from the runtime
  registration state, free custom-geometry source states, deinit the source
  list, and set `native = null`.

### 3. MapProjectionHandle

- Replace the pointer-backed enum with a struct containing:
  - `native: ?*c.mln_map_projection`
  - `diagnostic_store: ?*diagnostics.DiagnosticStore`
- Change `create()` to accept `*MapHandle`.
- Change methods to pointer receivers.
- On successful close, destroy the projection and set `native = null`.

### 4. RenderSessionHandle and frame handles

- Replace the pointer-backed enum with a struct containing current
  `RenderSessionState` fields inline.
- Change attach functions to accept `*MapHandle` and return a
  `RenderSessionHandle` value.
- Change session methods to pointer receivers.
- Change `MetalOwnedTextureFrameHandle` and `VulkanOwnedTextureFrameHandle` to
  owned resource structs with pointer-receiver `release()` methods that
  invalidate the same frame handle.
- Keep frame handles scoped: callers release frames before closing the render
  session, and copied frame handles are invalid duplicate owners.
- On successful session close, reject active frame borrows, destroy the native
  session, and set `native = null`.

### 5. Public exports and call sites

- Update `maplibre_native.zig` re-exports if needed.
- Update all Zig tests and examples:
  - declare lifecycle handles as `var`;
  - pass parents by pointer, for example `MapHandle.create(&runtime, .{})`;
  - remove tests that require copied handles or released frame handles to keep
    returning `ClosedHandle`;
  - keep tests that prove same-wrapper double close/release is a no-op;
  - keep failed-close retry coverage.
- Update any helper functions in tests to accept pointers instead of copied
  handles.

### 6. Documentation follow-up

- Keep `docs/src/content/docs/development/bindings-zig.md` aligned with this
  model.
- Add short safety notes where callback or frame APIs require stable wrapper
  addresses while a borrow or callback registration is active.
- Avoid documenting copied lifecycle or texture frame handles as supported.

## Validation

Run, in order:

```sh
mise run fix
mise run //bindings/zig:test
mise run test
```

If callback storage changes affect native callback behavior, also run the C ABI
resource tests through `mise run test` and inspect resource-provider and
resource-transform tests closely.

## Risks to watch

- Passing a pointer to an inline Zig handle as long-lived C `user_data` would be
  unsafe if callers move the handle. Use heap callback state for those cases.
- Frame handles need stable active-borrow state because render session wrappers
  can move as ordinary Zig values. Keep frame borrows short and explicit.
- Some tests currently rely on stale copied handles returning `ClosedHandle`.
  Those tests should change because copied lifecycle and texture frame handles
  are invalid under the target model.
- Parent-owned registration state must stay stable when callers return or move
  aggregate structs that contain both parent and child handles. Keep child
  wrapper pointers limited to stable state, or document shorter scoped borrows
  such as frame handles explicitly.
