# MapLibre Native Go Binding Status

These bindings are draft low-level Go wrappers over the MapLibre Native C API.

## Known draft deviations

The owner-thread helper described by the binding specification is deferred.
Until that helper lands, Go callers are responsible for pinning runtime/map
lifecycles to one OS thread.

Call `runtime.LockOSThread()` before creating a `RuntimeHandle`, keep the
runtime and its child handles on that locked goroutine, and close those handles
before unlocking the thread. Owner-thread-affine methods called from another OS
thread return `ErrWrongThread` with the native diagnostic when the C API reports
that status.

TODO: add a binding-owned owner-thread helper that serializes create, pump,
event polling, operations, and close on one native owner thread.
