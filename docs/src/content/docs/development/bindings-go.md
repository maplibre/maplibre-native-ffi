---
title: Go Binding Conventions
description: Language-specific implementation conventions for Go bindings.
---

Resources:

- Tracking issue:
  [#43](https://github.com/maplibre/maplibre-native-ffi/issues/43)
- [Go `cgo` documentation](https://pkg.go.dev/cmd/cgo)
- [`runtime.Pinner`](https://pkg.go.dev/runtime#Pinner)

The Go binding uses `cgo` over the public C headers and keeps raw C declarations
private.

Use the repository's supported Go toolchain when adding the package. Go 1.21 is
the interop floor because it provides `runtime.Pinner`; newer baselines are fine
when they match CI. `cgo` gives direct header checking, struct layout, callback
exports, and C compiler diagnostics.

Represent public handles as structs with explicit `Close() error` methods. Use
Go finalizers for leak reporting. Use them for cleanup only for native resources
whose release function is documented as thread-independent and infallible,
because finalizers run on GC threads.

Document that callers can use `runtime.LockOSThread` when they need
deterministic owner-thread affinity. The low-level binding preserves caller
execution: ordinary calls run on the calling goroutine and are not silently
marshaled to another goroutine.

Follow cgo pointer rules strictly. Pin Go pointers for the full retention period
before C stores them. Use C-owned storage for retained strings and buffers, and
use `runtime/cgo.Handle` or binding-owned registry tokens for callback state.

Callbacks use exported Go trampolines and recover panics before returning to C.
Resource-provider request wrappers enforce one-shot completion and release the C
request handle exactly once.

Represent backend-native addresses as:

```go
type NativePointer uintptr
```

It converts to `unsafe.Pointer` only at the `cgo` boundary.
