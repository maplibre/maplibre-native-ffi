# MapLibre Native patches

`.mise/bin/sync-submodules` applies these to the `third_party/maplibre-native`
worktree after checking out the pinned commit. The submodule keeps tracking
upstream, so the pin stays honest and each patch is a change we are carrying
only until it lands there.

1. `0001-configurable-emdawnwebgpu-suspension.patch` turns the suspension link
   option `vendor/dawn.cmake` fixes on `mbgl-vendor-dawn` into a cache variable,
   leaving `-sASYNCIFY=1` as the default. It is an INTERFACE option, so it lands
   after a consumer's directory-level options and cannot be overridden from
   here; the browser WebGPU build needs `-sJSPI` instead. See
   `cmake/mln_ffi_emscripten.cmake` for why that matters. Upstream:
   [maplibre-native#4451](https://github.com/maplibre/maplibre-native/pull/4451).
2. `0002-expose-retained-tile-source-tileset.patch` adds a read-only accessor
   for the parsed TileJSON state that a tile source retains. The C API uses it
   to return live source metadata without depending on MapLibre's private source
   implementation or reparsing a style document.

Drop a patch once the pin moves to a commit that carries it. Applying is
idempotent, and the sync restores the files a patch touches before moving the
submodule, so a pin bump and an edit to a patch both take effect on a worktree
that already carries the old version. A patch that no longer applies fails the
sync rather than being skipped. Each patch changes files that the pinned commit
already has, because restoring a path is how the sync clears what it applied.
