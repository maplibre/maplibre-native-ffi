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

`0003-run-loop-process-gate.patch` adds an optional gate callback to
`RunLoop::process()`, consulted before each queued task is dequeued. The C API
uses it to bound one pump's drain; the budget logic stays on the C API side, and
an unset gate keeps upstream behavior. Upstream:
[maplibre-native#4577](https://github.com/maplibre/maplibre-native/pull/4577).
The upstream proposal uses a deadline budget in place of the gate callback.

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

`0010-background-drawable-replacement.patch` recreates background drawables when
a style change replaces their property updater. This keeps the drawables and
their uniform buffers associated with the same updater after same-ID layer
replacement, paint changes, and solid/pattern changes. The patch includes Native
pixel-readback regression tests. See
[issue #709](https://github.com/maplibre/maplibre-native-ffi/issues/709).
Upstream:
[maplibre-native#4616](https://github.com/maplibre/maplibre-native/pull/4616).

`0011-opengl-uniform-block-order.patch` declares shared uniform blocks before
fragment-only blocks in five OpenGL shaders. SwiftShader otherwise reads
incorrect paint values and renders symbols, dashed lines, and patterned
backgrounds transparently on Android x86_64. Native map pixel-readback
regressions cover all five shaders using the existing glyph fixture. See
[issue #713](https://github.com/maplibre/maplibre-native-ffi/issues/713).

Drop a patch once the pin moves to a commit that carries it. The sync checks out
the pinned commit with `--force`, so it discards whatever the last sync applied
before applying the list again. A pin bump, an edit to a patch, and a dropped
patch all take effect on a worktree that still carries the old version. A patch
that no longer applies fails the sync rather than being skipped.

Local edits to the submodule worktree, including edits inside a nested vendor
submodule, are discarded by the same checkout, and a sync runs it whenever the
worktree carries a tracked change that no listed patch accounts for. The sync
prints those paths first. A forced checkout also removes an untracked file that
sits where a new pin adds a tracked one.
