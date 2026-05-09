---
title: Node Binding Notes
description: Language-specific implementation notes for Node bindings.
---

Tracking issue:
[Add Node bindings](https://github.com/maplibre/maplibre-native-ffi/issues/50).

The Node binding targets Node.js 22 LTS and newer with TypeScript declarations
and a native N-API add-on. Use stable Node-API with an N-API v8 baseline.

Package names:

```text
@maplibre/native          npm package
@maplibre/native/internal private native add-on boundary
```

Build the add-on in Rust with `napi-rs` over the shared internal
`maplibre-native-support` crate and `maplibre-native-sys` declarations. This
shares low-level Rust ABI adaptation with the public Rust crate and Python
extension without wrapping the public Rust API. `napi-rs` supplies stable N-API
packaging, TypeScript generation, thread-safe functions, and environment cleanup
hooks.

Public APIs expose TypeScript classes, interfaces, enums or string unions,
copied data, and opaque `NativePointer` values.

Publish prebuilt native packages for the supported macOS, Linux glibc, Linux
musl, and Windows x64/arm64 targets when matching MapLibre artifacts exist.
Contributors can still build from source.

Owned native objects use `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`,
and `RenderSessionHandle`. Handles provide explicit `close()`, `destroy()`, or
`dispose()` methods and may implement `Symbol.dispose`. `FinalizationRegistry`
and native finalizers are leak-reporting aids unless the binding can prove
owner-thread destruction and parent-before-child order.

Status-returning C calls throw `MapLibreNativeError` with a stable status value
and copied same-thread diagnostic.

Runtime creation records the current JavaScript thread as the native owner
thread. Owner-thread calls run synchronously on that thread, not on the libuv
worker pool. Handles are not transferable across Node Worker threads or N-API
environments; a Worker may create and own its own runtime.

Use TypeScript-owned descriptor and event types. Materialize C descriptor graphs
and JSON values at the native boundary, reject embedded `NUL` bytes for
null-terminated strings, reject non-finite numbers for JSON-like values, and
copy borrowed native output before its borrow window ends. Use `bigint` where
64-bit integer exactness matters.

`NativePointer` is an opaque class for a borrowed backend-native address. It
transfers no ownership and grants no memory access. Provide an explicit unsafe
constructor from `bigint` only for backend interop.

Texture frame access uses callback-scoped helpers. The helper invokes a
synchronous callback and releases the frame in a `finally` path; TypeScript
signatures exclude `Promise` returns, and the runtime rejects thenables.

JavaScript callbacks use N-API thread-safe functions or native queues rather
than direct calls from MapLibre worker, network, logging, or render-related
threads. Resource-provider requests may use an asynchronous JavaScript shape
with one-shot completion. Resource-transform and custom-geometry callbacks stay
out of the first public surface until the binding has a bounded bridge for
arbitrary native callback threads. Logging may use a bounded best-effort queue.
