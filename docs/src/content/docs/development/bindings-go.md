---
title: Go Binding Notes
description: Language-specific implementation notes for Go bindings.
---

Tracking issue:
[Add Go bindings](https://github.com/maplibre/maplibre-native-ffi/issues/43).

The Go binding uses `cgo` over the public C headers and keeps raw C declarations
private.

Package and module names:

```text
github.com/maplibre/maplibre-native-ffi/bindings/go  Go module
maplibre                                           public package
internal/capi                                      private cgo layer
```

Use the repository's supported Go toolchain when adding the package. Go 1.21 is
the interop floor because it provides `runtime.Pinner`; newer baselines are fine
when they match CI. `cgo` gives direct header checking, struct layout, callback
exports, and C compiler diagnostics. Pure Go dynamic loading can be reconsidered
later after the `cgo` binding is stable.

Public APIs expose `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`, and
`RenderSessionHandle` structs with explicit `Close() error` methods. Finalizers
are for leak reporting or best-effort cleanup only, because owner-thread-affine
native destroy functions can fail with `MLN_STATUS_WRONG_THREAD`.

The binding preserves the native owner-thread model. Callers that need
deterministic ownership should create and use runtimes on a locked OS thread
with `runtime.LockOSThread`. The low-level binding should not silently marshal
ordinary calls to another goroutine.

Status-returning C calls return Go `error` values that preserve the C status
category and copied same-thread diagnostic. Repeated `Close` after a successful
close is a no-op.

Follow cgo pointer rules strictly. Do not store Go pointers in C memory unless
they are pinned for the full retention period. Use C-owned storage for retained
strings and buffers, and use `runtime/cgo.Handle` or binding-owned registry
tokens for callback state.

Callbacks use exported Go trampolines and recover panics before returning to C.
Resource-provider request wrappers enforce one-shot completion and release the C
request handle exactly once.

Represent backend-native addresses as:

```go
type NativePointer uintptr
```

It is opaque, borrowed, and converted to `unsafe.Pointer` only at the `cgo`
boundary.
