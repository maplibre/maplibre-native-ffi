# MapLibre Native patches

`.mise/bin/sync-submodules` applies these to the `third_party/maplibre-native`
worktree after checking out the pinned commit. The submodule keeps tracking
upstream, so the pin stays honest and each patch is a change we are carrying
only until it lands there.

`0002-windows-local-file-urls.patch` removes the URI-only leading slash from
canonical Windows drive paths and opens UTF-8 filenames through wide filesystem
APIs. This lets the local file source load percent-encoded `file:///C:/...`
resources whose paths contain spaces or non-ASCII characters.

`0004-opengl-valid-api-calls.patch` uses indexed extension enumeration on OpenGL
ES, allocates storage before copying a uniform buffer, and isolates allocation
errors from earlier OpenGL calls. This prevents strict implementations and the
API 26 Android emulator from turning stale errors into false allocation
failures.

`0005-unwrapped-unprojection.patch` adds wrap-mode overloads to map and
standalone projection coordinate conversion. The C API uses them to expose
continuous longitudes while the existing overloads keep wrapped behavior.

`0006-process-lifetime-logging.patch` gives the global logger, its observer,
mutex, severity settings, and scheduler process lifetime. This prevents static
destruction from joining a logging worker whose thread-local cleanup must detach
from an already shut down host VM. Explicit observer replacement and removal
still release the previous observer.

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
