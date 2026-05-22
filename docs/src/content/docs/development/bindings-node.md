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

## Architecture

The Node binding exposes a low-level TypeScript API backed by one native N-API
add-on. It targets Node.js 22.14 and newer, with Node-API v10 as the ABI
baseline.

Build the add-on in Rust with `napi-rs` over the shared Rust bridge crates from
the [Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/).
Node shares ABI adaptation with other bridge bindings, but it does not wrap the
public Rust API. Rust code owns status conversion, descriptor materialization,
callback trampolines, native-result copying, and handle state. `napi-rs` owns
the N-API entry points, TypeScript declaration generation, JavaScript class
exports, thread-safe functions, and environment cleanup hooks.

Keep generated or raw N-API details below the package boundary. Public
TypeScript modules group C API concepts such as runtime, map, render, resource,
camera, and style. The package exports concrete classes and value objects.
Higher-level adapters may add Promise, React, Electron, or framework scheduling
integrations above this layer.

## TypeScript Surface

Long-lived native objects use the shared `Handle` suffix. They expose explicit
`close()` methods and implement `Symbol.dispose` for `using` blocks. Successful
`close()` releases once; later calls no-op. Failed native destruction leaves the
handle live so callers can retry or report the diagnostic.

C option structs become TypeScript descriptor interfaces or small classes. Input
descriptors use ordinary optional properties for field-mask presence. Internal
Rust materializers write C `size` fields, masks, string views, and temporary
arrays. Public callers set semantic fields only.

Closed C enum domains become string-literal unions or exported frozen objects
with explicit native mappings. Output domains that may grow preserve unknown raw
values in an `Unknown` object shape. User-visible C bit masks become explicit
set-like arrays or purpose-built mask objects; C field masks stay internal.

Use `bigint` where exact 64-bit integer values matter. JSON and GeoJSON values
use JavaScript-owned value trees, while native snapshots and result handles stay
private and copy into independent JavaScript values before release.

`NativePointer` is a borrowed backend address value. It stores the address
privately and grants no memory access. Provide an explicitly named unsafe
constructor from `bigint` only for backend interop APIs whose C contract accepts
an opaque host pointer.

## Handles, Environments, and Threads

A runtime created from JavaScript is owned by the JavaScript thread of the N-API
environment that created it. Owner-thread-affine methods execute synchronously
on the calling JavaScript thread. The binding does not dispatch ordinary calls
through the libuv worker pool, a hidden scheduler, or another event loop. Native
`MLN_STATUS_WRONG_THREAD` results become `WrongThreadError` with the copied
thread-local diagnostic.

Handles stay within their creating N-API environment. They are not structured
cloneable, transferable, or valid in another Node Worker. A Worker may create
its own runtime, maps, and render sessions; those handles belong to that
Worker's JavaScript thread and environment. Cross-Worker coordination belongs in
application code or adapters above this binding.

Each handle stores private native state: pointer, released flag, parent
references, owner environment, and optional leak context. Child handles keep
parents strongly alive while native validity depends on them.
`MapProjectionHandle` follows the shared exception: it owns a standalone
projection snapshot after creation and does not depend on the source `MapHandle`
for native validity.

Use `FinalizationRegistry`, native finalizers, and N-API environment cleanup
hooks for leak reporting and JavaScript-side state cleanup. They destroy native
resources only when the release function is documented as thread-independent and
infallible. Thread-affine resources close through `close()` on the owner
JavaScript thread.

## Status, Diagnostics, and Calls

Public fallible operations throw `MaplibreError` subclasses. Each error carries
a stable kind, the raw native status, and the diagnostic copied immediately on
the same native thread before another C call can replace it. Binding-owned
validation covers closed wrappers, wrong environment use, active frame scopes,
invalid string shapes, descriptor depth, and one-shot request completion. The C
API validates native state, ranges, enum domains, and MapLibre-specific rules.

Ordinary methods are synchronous unless they represent a callback handoff that
native code can complete later. Synchronous methods either finish the native
call or throw. They do not return `Promise` as a scheduling hint. Promise-based
conveniences can live in optional adapters that choose an event-loop and
cancellation policy explicitly.

## Callbacks and Event-Loop Handoff

MapLibre callbacks may arrive on worker, network, logging, or render-related
threads. JavaScript user callbacks run on the owning JavaScript thread through
N-API `ThreadsafeFunction` bridges only for callback contracts that can notify,
complete, or cancel asynchronously. Native trampolines copy borrowed request
fields, enqueue JavaScript work, and return the immediate C result from native
routing, queue failure, or cancellation rules—not from a later JavaScript return
value. They never run JavaScript directly on an arbitrary MapLibre thread.

Resource provider matching uses native-owned routing rules before crossing into
JavaScript. Non-matching requests pass through immediately. Matching requests
copy request data and create a one-shot JavaScript request object that owns the
provider's native request reference. The object completes during the JavaScript
callback on the owner event loop or later from that event loop when the C API
permits cross-thread completion. It releases the native request exactly once.

Resource transforms stay synchronous by using native-owned rewrite rules
configured from JavaScript. Avoid JavaScript transform callbacks on MapLibre
network threads. Custom geometry callbacks notify JavaScript through the same
thread-safe handoff; JavaScript posts tile data later through owner-thread map
APIs. Logging uses a bounded best-effort queue so native logging threads keep
moving when JavaScript falls behind.

## Render Targets

Render target descriptors are JavaScript-owned values. Surface and
borrowed-texture descriptors store backend handles as `NativePointer`; callers
keep backend objects valid and synchronized for the lifetime required by the C
API.

Attach methods return `RenderSessionHandle`. Texture readback supports
caller-provided `Uint8Array` or `ArrayBuffer` storage and may offer a
convenience method that returns a copied image object.

Session-owned texture frames use synchronous callback scopes. A helper acquires
the native frame, passes a scoped frame view to the callback, and releases the
frame before returning to JavaScript. TypeScript signatures express a
non-`Promise` return; any returned promise is silently ignored because the
native frame is released before the microtask queue drains. Frame metadata may
be copied; backend `NativePointer` accessors reject use after the scope ends.
Callers close the frame scope before resize, another render update, detach, or
session destruction.

## Testing

Node tests exercise the TypeScript surface against the real native add-on. Cover
handle close idempotence, wrong-thread and wrong-environment errors with
Workers, copied diagnostics, descriptor materialization, callback handoff,
one-shot resource request completion, and frame-scope invalidation. Keep tests
small and focused on Node adaptation; C ABI tests cover native behavior.
