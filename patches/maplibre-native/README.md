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

`0003-run-loop-budget.patch` adds `RunLoop::runOnce(Duration budget)`, which
stops dequeuing tasks once the budget has elapsed. At least one task runs, the
remaining tasks stay queued, and the loop wakes itself so the next pass picks
them up. The C API uses it to bound one pump's drain. Upstream:
[maplibre-native#4577](https://github.com/maplibre/maplibre-native/pull/4577).

`0004-opengl-valid-api-calls.patch` allocates storage before copying a uniform
buffer and isolates allocation errors from earlier OpenGL calls. This prevents
strict implementations and the API 26 Android emulator from turning stale errors
into false allocation failures. Upstream:
[maplibre-native#4578](https://github.com/maplibre/maplibre-native/pull/4578).

`0005-unwrapped-unprojection.patch` adds wrap-mode overloads to map and
standalone projection coordinate conversion. The C API uses them to expose
continuous longitudes while the existing overloads keep wrapped behavior.
Upstream:
[maplibre-native#4573](https://github.com/maplibre/maplibre-native/pull/4573).

`0006-process-lifetime-logging.patch` gives the global logger, its observer,
mutex, severity settings, and scheduler process lifetime. This prevents static
destruction from joining a logging worker whose thread-local cleanup must detach
from an already shut down host VM. Explicit observer replacement and removal
still release the previous observer. Upstream:
[maplibre-native#4574](https://github.com/maplibre/maplibre-native/pull/4574).

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

`0012-vulkan-surface-acquire-timeout.patch` lets a surface resource bound
swapchain image acquisition. When the bound expires, the frame aborts with
`SurfaceNotReady` before it records or submits GPU work. Android's Vulkan loader
tells an app-owned BufferQueue to wait for a free buffer only when the timeout
is finite, and it reports buffer starvation under an unbounded acquire as a lost
surface. The Android native surface target uses a 16 ms bound, and the C API
reports the target as not ready, so the host retries with the same render
session. The patch includes a Native regression that injects acquisition stalls
and checks that frame fences remain usable for later GPU submissions. See
[maplibre-compose#1370](https://github.com/maplibre/maplibre-compose/issues/1370).

`0013-projection-from-transform-state.patch` adds a standalone projection
constructor that copies a transform state. The C API uses it to expose the
projection of the update that a render session rendered. Upstream:
[maplibre-native#4647](https://github.com/maplibre/maplibre-native/pull/4647).

`0014-global-state.patch` adds the `global-state` expression, root `state`
defaults, and Native's runtime state APIs. State changes update dependent paint
properties, filters, layout, and color ramps. The patch includes the upstream
tests and render fixtures. It carries Taiyu Yoshizawa's (NEKOYASAN) existing
[maplibre-native#4516](https://github.com/maplibre/maplibre-native/pull/4516),
at commit `844751cacb9b32e971076f24fb74a3ec8db1c2ea`, as an unmodified diff from
base `1805fa27a55c59c84d72831ded4fa5cf0a042e25`. The C API exposes the runtime
state setter and snapshot getter.

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
The Android SDK also cancels transitions before invoking Native and needs a
separate change to expose this behavior.

`0016-location-indicator-color-transitions.patch` refreshes the location
indicator's evaluated paint properties each frame so that accuracy-circle fill
and border colors reach their transition targets. It includes the upstream
pixel-readback regression. Upstream:
[maplibre-native#4639](https://github.com/maplibre/maplibre-native/pull/4639),
at commit `7176ab92c871b4f99f96755d4e2ce11b73bfcceb`.

`0017-location-indicator-top-image-hit-testing.patch` includes the location
indicator's top image in rendered-feature queries. Each top and bearing image
has independent bounds, and a hit returns the feature once with
longitude-latitude geometry. The patch includes the upstream regression for a
top-only indicator and checks that shadow and accuracy-circle coverage outside
the image stays excluded. Upstream:
[maplibre-native#4640](https://github.com/maplibre/maplibre-native/pull/4640),
at commit `363acddb8471dc344cb17dac1e2637e17cff5535`.

`0021-location-indicator-bearing-accuracy.patch` adds a bearing-accuracy sector
with an angular half-width, a radius in logical pixels, and a color that fades
toward the outer edge. The paint properties support zoom expressions and
transitions. The patch includes the upstream conversion test and three render
fixtures with their binary reference images. Upstream:
[maplibre-native#4644](https://github.com/maplibre/maplibre-native/pull/4644),
at commit `02d9a4b2ccb4f3d15cdca438fd08ea6fa2cd530b`.

Each patch is the squashed diff of its upstream branch, applied on top of the
patches before it, so a patch that adds a test next to an earlier patch's test
carries that placement rather than the branch's own context.

Drop a patch once the pin moves to a commit that carries it. The sync checks out
the pinned commit with `--force`, so it discards whatever the last sync applied
before applying the list again. A pin bump, an edit to a patch, and a dropped
patch all take effect on a worktree that still carries the old version. A patch
that no longer applies fails the sync rather than being skipped.

A sync records the pinned commit and a hash of the worktree's diff against it in
the submodule's git directory, and a later sync that finds the same record
leaves the worktree alone. Local edits to the submodule worktree, including
edits inside a nested vendor submodule, change that record and are discarded by
the next sync's checkout. When such an edit sits outside every listed patch's
paths, the sync prints the path first. A forced checkout also removes an
untracked file that sits where a new pin adds a tracked one, and the sync
removes a file that a listed patch adds before applying that patch again.
