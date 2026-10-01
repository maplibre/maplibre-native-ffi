# MapLibre Native patches

`.mise/bin/sync-submodules` applies these to the `third_party/maplibre-native`
worktree after checking out the pinned commit. The submodule keeps tracking
upstream, so the pin stays honest and each patch is a change we are carrying
only until it lands there.

`0002-windows-local-file-urls.patch` removes the URI-only leading slash from
canonical Windows drive paths and opens UTF-8 filenames through wide filesystem
APIs. This lets the local file source load percent-encoded `file:///C:/...`
resources whose paths contain spaces or non-ASCII characters. Upstream:
[maplibre-native#4572](https://github.com/maplibre/maplibre-native/pull/4572).

`0004-opengl-valid-api-calls.patch` allocates storage before copying a uniform
buffer and isolates allocation errors from earlier OpenGL calls. This prevents
strict implementations and the API 26 Android emulator from turning stale errors
into false allocation failures. Upstream:
[maplibre-native#4578](https://github.com/maplibre/maplibre-native/pull/4578).

`0007-retain-active-on-demand-images.patch` keeps present on-demand images
registered to their requestor when the same request also needs missing images.
This prevents cache cleanup from evicting active tile dependencies. The patch
includes an ImageManager regression test for retention, delivery, and
reclamation after the requestor releases its images. Upstream:
[maplibre-native#4575](https://github.com/maplibre/maplibre-native/pull/4575).

`0009-synchronous-symbol-dependencies.patch` registers glyph and image
dependencies before requesting either set. Cached glyphs can return inline for
synchronous GeoJSON tiles; layout must wait for the images too. The patch
includes a Native map regression test that checks icon and label pixels across
zoom changes with a cached glyph range. See
[issue #698](https://github.com/maplibre/maplibre-native-ffi/issues/698).
Upstream:
[maplibre-native#4580](https://github.com/maplibre/maplibre-native/pull/4580).

`0011-opengl-uniform-block-order.patch` declares shared uniform blocks before
fragment-only blocks in five OpenGL shaders. SwiftShader otherwise reads
incorrect paint values and renders symbols, dashed lines, and patterned
backgrounds transparently on Android x86_64. Native map pixel-readback
regressions cover all five shaders using the existing glyph fixture. See
[issue #713](https://github.com/maplibre/maplibre-native-ffi/issues/713).
Upstream:
[maplibre-native#4625](https://github.com/maplibre/maplibre-native/pull/4625).

`0015-independent-camera-animations.patch` lets partial camera commands animate
independently. Replacing a property preserves the timing of other properties,
including properties from the same command. Flights couple center and zoom;
anchors couple center with the properties in their command. Native composes the
active values before applying camera constraints, and reports each command
finished once all its properties have ended. The patch includes Transform
regressions for timing, partial replacement, coupled motion, constraints, and
callback reentrancy. See
[maplibre-native#2790](https://github.com/maplibre/maplibre-native/issues/2790).
Upstream:
[maplibre-native#4637](https://github.com/maplibre/maplibre-native/pull/4637).
The Android and macOS SDK camera entry points cancel transitions before partial
commands to preserve their existing behavior.

`0016-location-indicator-color-transitions.patch` refreshes the location
indicator's evaluated paint properties each frame so that accuracy-circle fill
and border colors reach their transition targets. It includes the upstream
pixel-readback regression. Upstream:
[maplibre-native#4639](https://github.com/maplibre/maplibre-native/pull/4639),
at commit `c6b6de35de0001d1b6005105199cc639a7956fa0`.

`0017-location-indicator-top-image-hit-testing.patch` includes the location
indicator's top image in rendered-feature queries. Each top and bearing image
has independent bounds, and a hit returns the feature once with
longitude-latitude geometry. The patch includes the upstream regression for a
top-only indicator and checks that shadow and accuracy-circle coverage outside
the image stays excluded. Upstream:
[maplibre-native#4640](https://github.com/maplibre/maplibre-native/pull/4640),
at commit `0b5d2502bb39aa2098c585ae29684956d9e1ffd6`.

`0021-location-indicator-bearing-accuracy.patch` adds a bearing-accuracy sector
with an angular half-width, a radius in logical pixels, and a color that fades
toward the outer edge. The paint properties support zoom expressions and
transitions. The patch includes the upstream conversion test and three render
fixtures with their binary reference images. It carries the closed
[maplibre-native#4644](https://github.com/maplibre/maplibre-native/pull/4644),
at commit `02d9a4b2ccb4f3d15cdca438fd08ea6fa2cd530b`, until a location indicator
plugin can replace the core layer.

`0022-legacy-annotations-option.patch` adds the `MLN_WITH_LEGACY_ANNOTATIONS`
CMake option. Turning it off compiles the legacy annotation manager as disabled,
so a loaded style no longer gains the `org.maplibre.annotations` source and its
symbol layer. The C API exposes no annotation entry points and builds with the
option off. Upstream:
[maplibre-native#4675](https://github.com/maplibre/maplibre-native/pull/4675).

`0023-sourceless-style-rendering.patch` renders a style that has no sources. The
orchestrator added layers that take no source while it updated the first source,
so a style with none produced no render items and no background color, and a
style load whose parse added no layer published no update to the renderer. The
legacy annotation source hid both, because it joined every style. Upstream:
[maplibre-native#4674](https://github.com/maplibre/maplibre-native/pull/4674).

`0024-render-update-publication.patch` publishes render updates after image and
source removal, source insertion, tile-selection changes, and transition
changes. Generated layer transition setters also refresh the style's immutable
layer collection. Native regressions cover collection changes, transition
options, and tile-selection updates. Upstream:
[maplibre-native#4676](https://github.com/maplibre/maplibre-native/pull/4676),
[maplibre-native#4677](https://github.com/maplibre/maplibre-native/pull/4677),
and
[maplibre-native#4678](https://github.com/maplibre/maplibre-native/pull/4678).
See [issue #736](https://github.com/maplibre/maplibre-native-ffi/issues/736).

`0025-vertex-buffer-upload-timestamps.patch` initializes vertex buffer upload
timestamps and preserves them during moves in Metal, Vulkan, and WebGPU. An
uninitialized timestamp can suppress uploads after feature-state changes. The
patch includes the upstream resource regression. Upstream:
[maplibre-native#4679](https://github.com/maplibre/maplibre-native/pull/4679),
at commit `02ddb45fbfad`.

`0026-empty-symbol-placement.patch` clears deferred symbol placement and query
state when no layers supply placement data. Source fade bookkeeping and paint
transitions continue, and symbols that return receive a fresh placement. Native
regressions cover repeated background-only frames, paint transitions, and symbol
removal and reappearance within the placement update interval. See
[issue #735](https://github.com/maplibre/maplibre-native-ffi/issues/735).

`0027-line-hit-test-endpoint-offset.patch` applies the full line offset to the
last vertex during rendered-feature hit testing. The Native regression checks
painted pixels and queries near the endpoint for positive and negative offsets.
Upstream:
[maplibre-native#4702](https://github.com/maplibre/maplibre-native/pull/4702).

`0028-hit-test-camera-zoom.patch` evaluates hit-test paint properties at the
camera zoom. The Native regression covers composite line and circle properties
at fractional zooms and beyond the source's maximum zoom. Upstream:
[maplibre-native#4703](https://github.com/maplibre/maplibre-native/pull/4703).

`0029-query-filter-overscaled-zoom.patch` evaluates rendered-feature query
filters at the overscaled tile zoom, matching layer filters and source queries.
The Native regression covers symbols and circles at fractional and overzoomed
camera zooms. Upstream:
[maplibre-native#4704](https://github.com/maplibre/maplibre-native/pull/4704).

`0030-custom-geometry-query-before-data.patch` returns no features from a custom
geometry tile that has no data yet, as GeoJSON and vector tiles already do. A
source-feature query that reached a fetched tile before the host delivered its
data otherwise dereferenced null. The C suite's custom source cases cover it.
Upstream: not yet filed.

`0031-webgpu-frame-stats.patch` counts frames in the WebGPU backend's rendering
stats and resets its per-frame draw call count at the start of each frame, as
the Metal, Vulkan, and OpenGL backends do. It also adds each draw to the total
draw call count. The WebGPU frame count otherwise stayed at zero. The C suite's
render lifecycle case covers it. Upstream: not yet filed.

`0032-in-memory-database-reset.patch` keeps a reset of the in-memory database
from deleting a file named `:memory:`. The reset removed that path relative to
the working directory, which deleted any such file there and failed where the
directory was read-only. The C suite's ambient cache reset case covers it.
Upstream: not yet filed.

`0033-destroy-thread-local-run-loop.patch` keeps the destructor of the run loop
that `Scheduler::GetCurrent()` creates for a thread with no scheduler. The
library compiles MapLibre Native without static destructors, so that nothing a
MapLibre thread reads is destroyed while the process exits, and that option also
drops thread-local destructors. The run loop is the one piece of MapLibre
thread-local state that owns resources, so the patch marks it
`[[clang::always_destroy]]` to free it when its thread ends. Upstream: not
applicable; it serves this build's compile options.

Each patch is a squashed diff applied on top of the patches before it. Patch
context and test placement follow the pinned source and earlier patches. The
publication patch includes the transition setters for our bearing-accuracy
properties, and the query-filter patch preserves the preceding camera-zoom fix.

Drop a patch once the pin moves to a commit that carries it. The sync checks out
the pinned commit with `--force`, so it discards whatever the last sync applied
before applying the list again. A pin bump, an edit to a patch, and a dropped
patch all take effect on a worktree that still carries the old version. A patch
that no longer applies fails the sync rather than being skipped.

A sync records the pinned commit, a hash of the worktree's diff against it, and
a hash of the patch list in the submodule's git directory, and a later sync that
finds the same record leaves the worktree alone. Local edits to the submodule
worktree, including edits inside a nested vendor submodule, change that record
and are discarded by the next sync's checkout. When such an edit sits outside
every listed patch's paths, the sync prints the path first. A forced checkout
also removes an untracked file that sits where a new pin adds a tracked one, and
the sync removes a file that a listed patch adds before applying that patch
again.
