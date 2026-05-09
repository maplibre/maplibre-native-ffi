---
title: Go Binding Conventions
description: Language-specific implementation conventions for Go bindings.
---

Resources:

- Tracking issue:
  [#43](https://github.com/maplibre/maplibre-native-ffi/issues/43)
- [Go `cgo` documentation](https://pkg.go.dev/cmd/cgo)
- [`runtime.Pinner`](https://pkg.go.dev/runtime#Pinner)
- [`runtime/cgo.Handle`](https://pkg.go.dev/runtime/cgo#Handle)

The Go binding uses `cgo` over the public C headers and keeps raw C declarations
private.

The Go binding targets Go 1.21 or newer. Go 1.21 provides `runtime.Pinner`, the
interop floor for retained Go pointers. `cgo` gives direct header checking,
struct layout, callback exports, and C compiler diagnostics.

The binding is a low-level layer. Adapters above it may provide
goroutine-friendly execution models, such as a locked owner goroutine that pumps
a runtime.

Represent public handles as structs with explicit `Close() error` methods. Use
Go finalizers for leak reporting. Reserve finalizer cleanup for native resources
whose release function is documented as thread-independent and infallible,
because finalizers run on GC threads.

Document that Go goroutines do not preserve OS-thread identity. Callers use
`runtime.LockOSThread` when they need deterministic owner-thread affinity. The
low-level binding preserves caller execution: ordinary calls run on the calling
goroutine and return the native wrong-thread error when the call reaches the
wrong owner thread.

Status-returning calls capture native diagnostics on the same OS thread that
returned the status. Use per-call C helpers that copy diagnostics before
returning to Go, or use a short `runtime.LockOSThread` window to keep the C call
and diagnostic read together. Expose stable Go error categories that work with
`errors.Is` and include the copied diagnostic message.

Follow cgo pointer rules strictly. Pin Go pointers for the full retention period
before C stores them. Use `runtime.Pinner` for retained Go memory. Let cgo's
per-call pinning cover ordinary buffers, and store callback closures through
`runtime/cgo.Handle` or binding-owned registry tokens. Use C-owned storage for
retained strings, buffers, and callback `user_data` cells.

Callbacks use exported Go trampolines and recover panics before returning to C.
Resource-provider request wrappers enforce one-shot completion and release the C
request handle exactly once.

Represent backend-native addresses as:

```go
type NativePointer uintptr
```

It converts to `unsafe.Pointer` only at the `cgo` boundary.
