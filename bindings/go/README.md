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
event draining, operations, and close on one native owner thread.

## Layer plugins

The binding exposes no plugin API. MapLibre Native's layer plugins are native
code whose callbacks run on tile workers and the render thread for the process
lifetime, so a plugin is written in C or another native language and registers
through the exported `mln_plugin_register_v1`. Its contract is
`maplibre_native_c/plugin.h` from the installed headers, which a cgo file can
include alongside `maplibre_native_c.h`.
