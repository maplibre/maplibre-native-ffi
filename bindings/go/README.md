# MapLibre Native Go binding

These draft low-level bindings expose the MapLibre Native C API through
goroutine-safe runtime, map, and camera handles. Native workers make progress
autonomously. Commands return futures with terminal disposition and snapshot
generation.

Runtime creation and public handle retirement are synchronous. Map creation,
ordered queries, and lifecycle transitions return typed futures. Render-session
graphics calls remain thread-affine. Poll event and frame-result drains from an
existing host cadence, and keep caller-driver service active while presentation
is paused.
