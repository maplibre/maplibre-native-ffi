# MapLibre Native Go binding

These draft low-level bindings expose the MapLibre Native C API through
goroutine-safe runtime, map, and camera handles. Native workers make progress
autonomously, and commands return runtime-wide monotonic IDs.

Runtime and map creation and closure wait for their native operations to
complete. Ordered camera reads return typed operation handles. Render-session
graphics calls remain thread-affine.

Install a runtime notification callback to schedule receiver work. The receiver
calls `DrainReady` and then drains the ready typed queues.
