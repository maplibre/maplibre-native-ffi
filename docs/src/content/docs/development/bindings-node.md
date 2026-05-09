---
title: Node.js Binding Conventions
description: Language-specific implementation conventions for Node.js bindings.
---

Resources:

- Tracking issue:
  [#50](https://github.com/maplibre/maplibre-native-ffi/issues/50)
- [Node-API documentation](https://nodejs.org/api/n-api.html)
- [`napi-rs`](https://napi.rs/)
- [Node.js worker threads](https://nodejs.org/api/worker_threads.html)

The Node binding targets Node.js 22.14+ and newer with TypeScript declarations
and a native N-API add-on. Use Node-API v10 as the ABI baseline.

Build the add-on in Rust with `napi-rs` over the shared internal crates defined
by the
[Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/).
Python and Node share Rust ABI adaptation with the public Rust crate, but
neither wraps the public Rust API. `napi-rs` supplies stable N-API packaging,
TypeScript generation, thread-safe functions, and environment cleanup hooks.

Handles provide explicit `close()` methods and implement `Symbol.dispose`. Use
`FinalizationRegistry` and native finalizers for leak reporting. Use them for
cleanup only for native resources whose release function is documented as
thread-independent and infallible.

A runtime created from JavaScript is owned by that Node environment's JavaScript
thread. Owner-thread-affine methods execute synchronously on the calling
JavaScript thread; the binding does not dispatch ordinary calls through the
libuv worker pool. Handles stay within the N-API environment that created them.
A Node Worker can create its own runtime, owned by that Worker's JavaScript
thread.

Use `bigint` where 64-bit integer exactness matters.

`NativePointer` stores the backend-native address privately. Provide an explicit
unsafe constructor from `bigint` only for backend interop.

Texture frame access uses callback-scoped helpers. The callback must finish
synchronously because the native frame is released when it returns; TypeScript
signatures exclude `Promise` returns.

Native callbacks from MapLibre worker, network, logging, or render-related
threads use N-API thread-safe functions to hand work back to the owning
JavaScript thread. Matching resource-provider requests complete later through a
one-shot JavaScript request object. Custom geometry fetch and cancel callbacks
notify JavaScript through the same bridge; JavaScript posts tile data later
through the owner-thread map API. Logging uses a bounded best-effort queue.
Resource transforms stay synchronous by using native-owned rewrite rules
configured from JavaScript, not a JavaScript callback on the MapLibre network
thread.
