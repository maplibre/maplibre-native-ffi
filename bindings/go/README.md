# MapLibre Native Go binding

These draft low-level bindings expose the MapLibre Native C API through
goroutine-safe runtime, map, and render-session handles. Native workers make
progress on their own, so a command or an ordered query returns a future that
carries the terminal disposition and the snapshot generation the work committed
at.

Runtime creation is synchronous, and closing a runtime or a map returns the
future for its native teardown. Render-session driver work stays thread-affine.
Poll RuntimeHandle.DrainEvents and RenderSessionHandle.DrainFrameResults from a
cadence the host already has, and keep servicing a caller-driver session while
presentation is paused.
