# MapLibre Native patches

`.mise/bin/sync-submodules` applies these to the `third_party/maplibre-native`
worktree after checking out the pinned commit. The submodule keeps tracking
upstream, so the pin stays honest and each patch is a change we are carrying
only until it lands there.

`0002-windows-local-file-urls.patch` removes the URI-only leading slash from
canonical Windows drive paths and opens UTF-8 filenames through wide filesystem
APIs. This lets the local file source load percent-encoded `file:///C:/...`
resources whose paths contain spaces or non-ASCII characters.

`0003-run-loop-process-gate.patch` adds an optional gate callback to
`RunLoop::process()`, consulted before each queued task is dequeued. The C API
uses it to bound one pump's drain; the budget logic stays on the C API side, and
an unset gate keeps upstream behavior.

Drop a patch once the pin moves to a commit that carries it. Applying is
idempotent, and the sync restores the files a patch touches before moving the
submodule, so a pin bump and an edit to a patch both take effect on a worktree
that already carries the old version. A patch that no longer applies fails the
sync rather than being skipped. Each patch changes files that the pinned commit
already has, because restoring a path is how the sync clears what it applied.
