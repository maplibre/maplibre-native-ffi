---
title: Rust Binding Conventions
description: Language-specific implementation conventions for Rust bindings.
---

Resources:

- Tracking issue:
  [#41](https://github.com/maplibre/maplibre-native-ffi/issues/41)
- [Rust Nomicon: FFI](https://doc.rust-lang.org/nomicon/ffi.html)
- [`bindgen` user guide](https://rust-lang.github.io/rust-bindgen/)
- [Rust API Guidelines: FFI](https://rust-lang.github.io/api-guidelines/interoperability.html)

## Architecture

The Rust binding serves two roles: a direct low-level Rust API and the shared
native implementation base for bridge bindings. Bridge bindings depend on
`maplibre-native-support`, keeping each host runtime's exception types,
schedulers, and package conventions separate while sharing the C ABI adaptation
code.

```text
maplibre-native-sys
  Generated unsafe declarations for the public C ABI.

maplibre-native-support
  Shared glue above sys: status conversion, diagnostics, descriptor
  materializers, callback trampolines, and build/link utilities.

maplibre-native
  Public safe Rust crate. Handles, owned values, events, errors,
  and narrow unsafe backend interop points.
```

Generate `maplibre-native-sys` with `bindgen` from
`include/maplibre_native_c.h`. Successful generation, compilation, and layout
testing doubles as the Rust bindability check for the public C headers. The
`sys` crate mirrors the ABI: raw `extern "C"` functions, constants, C layouts,
and opaque handle pointer types. Don't hand-edit generated bindings; refresh
with the appropriate mise task when the C headers change.

FFI details stay below the public crate boundary. Public modules group C API
concepts (for example, `runtime`, `map`, `render`). Generated types, raw
pointers, field masks, and callback trampolines stay internal to `sys` or
`support`.

The native library is loaded dynamically at runtime. The search order:
`MAPLIBRE_NATIVE_FFI_LIBRARY_PATH` for an exact file path, then the system
library search path.

## Type Surface

Owned values model copied C data as plain Rust structs. Mutable C option structs
become Rust structs with `Default` and builder-style setters; C field masks
derive from `Option<T>` fields or explicit setters and stay internal. Support
materializers write `size` fields and masks—callers set semantic fields only.
Native result, snapshot, and list handles stay internal; readers copy into owned
Rust values before releasing the native handle.

Closed C enum domains map to Rust enums with explicit raw conversions. All
public C-backed enums are `#[non_exhaustive]`. Output enums that may drift
across C ABI versions include an `Unknown(u32)` or `Unknown(i32)` variant
preserving the raw code for diagnostics. C bit masks become `bitflags` types for
user-visible masks; C field masks stay behind descriptors.

JSON and GeoJSON model as owned Rust value trees, preserving integer width,
object member order, and duplicate keys.

Runtime event polling returns owned `RuntimeEvent` values. Events identify
source maps with copied metadata (a Rust-assigned `MapId`). Unknown payloads
become `RuntimeEventPayload::Unknown`.

## Lifetimes and Threading

Thread-affine handles use `PhantomData<Rc<()>>` to opt out of auto-`Send` and
auto-`Sync`. Owner-thread assignments follow the shared convention.
`ResourceRequestHandle` is `Send` because the C API permits completion from any
thread. `MapProjectionHandle` is still `!Send` despite not retaining its parent
map—the projection is pinned to the map's owner thread at creation.

No internal dispatch to another thread. Async adapters above this crate own any
confinement or owner-thread executor policy. The Rust type system enforces
owner-thread confinement for thread-affine handles; native
`MLN_STATUS_WRONG_THREAD` results still map to `WrongThread` errors.

Parent retention follows the shared convention. A child holds its parent
strongly while live, so closing a parent with live children is a compile-time or
runtime error rather than native invalid state. `MapProjectionHandle` follows
the shared exception: standalone snapshot, no parent retention.

Handle state (released/live, parent reference, leak context) lives in a private
field on each handle wrapper. Because `!Send` prevents thread escape and
constructors run on the owner thread, the compiler proves thread-affine handles
drop on the owner thread in safe Rust. `Drop` calls the C destroy function for
still-live handles, records diagnostics on failure, and avoids double release.
Explicit `close` methods return `Result<()>` for deterministic status and
diagnostics.

## Status and Diagnostics

```rust
pub type Result<T> = std::result::Result<T, Error>;
```

Fallible public operations surface through `Result`. The binding never panics on
a native status. `Drop` and callback adapters follow their own documented paths
rather than returning `Result`.

Each C status category maps to a stable Rust error kind (for example,
`MLN_STATUS_WRONG_THREAD` → `WrongThread`). `Error` stores the mapped kind, raw
`mln_status`, and the copied thread-local diagnostic. Unknown future values map
to an `Unknown` kind with the raw code and diagnostic preserved.

The binding validates Rust-owned state before crossing into C—released wrappers,
active callback-scoped borrows, threading constraints, one-shot request
completion—and lets the C API validate native arguments, state, and ranges.

Handle-creating functions initialize raw out-pointers to null and wrap only
successful non-null results. C functions reporting presence through output
booleans become `Result<Option<T>>` or `Result<bool>`.

## FFI Memory

Public safe methods materialize C inputs at the call boundary. Temporary storage
uses stack values, `CString`, `Vec<T>`, or support-owned arenas scoped to the C
call. Object-owned native memory is reserved for storage the C API needs beyond
one call (callback state, reusable buffers, resource-provider request state).

Descriptor materializers own ABI bookkeeping and backing storage lifetime.
Snapshot, result, and list handles use internal RAII guards. Readers copy all
borrowed data, then release the native handle even on copy failure.

`NativePointer` construction and reconversion are `unsafe`, limited to C APIs
whose contract accepts the relevant opaque backend handle. Public safe APIs are
free of raw `sys` pointers. Public `unsafe` functions are small, named for the
native invariant they require, and document caller obligations.

## Callbacks

Trampolines live in `support`. They adapt C function pointers to Rust closures
or trait objects, copy or wrap callback arguments, catch panics with
`catch_unwind`, and convert failures to the C callback's documented behavior.
Panics never unwind through C frames.

Callback scoping follows the shared convention. State for callbacks that may
arrive on MapLibre worker, network, logging, or render threads requires
`Send + Sync + 'static`.

When replacing a callback, install the new native descriptor before closing the
old Rust state. If native installation fails, close the replacement state and
keep the previous state active.

Resource provider callbacks copy the borrowed `mln_resource_request` into an
owned `ResourceRequest` before user code can retain it. Pass-through decisions
return immediately; the binding must not retain or release the native handle.
`ResourceRequestHandle` is `Send`, enforces one-shot completion, and releases
the C request handle exactly once—on `complete`, explicit release, or `Drop`.

Resource transform callbacks copy the request URL before invoking user code.
Replacement URL storage stays alive until native consumes it. Per-thread
response scratch storage closes on the next callback for that thread and during
runtime teardown.

Custom geometry source callbacks are map/style scoped. They catch user failures,
track active upcalls, and delay state release until in-flight callbacks finish.
Callbacks that need map methods hand work back to the map owner thread before
calling thread-affine map APIs.

## Render Targets

Render target descriptors are Rust value types. Surface and borrowed-texture
descriptors store backend objects as `NativePointer`.

Attach methods return `RenderSessionHandle`. A session represents one attached
target for one map and holds the map strongly, so the map cannot be closed while
the session is live. Violations of the single-session rule surface as
`InvalidState` errors.

Texture readback supports two shapes:

- `read_premultiplied_rgba8_into(&mut [u8]) -> Result<TextureImageInfo>` for
  caller-owned reusable storage.
- A convenience method returning a copied `PremultipliedRgba8Image`.

Session-owned texture frames use closure-scoped accessors (`with_metal_frame`,
`with_vulkan_frame`). The helper acquires the native frame, passes a frame view
tied to a mutable borrow of the session, and releases the frame on return or
unwind. Frame views expose copied metadata through safe accessors. Backend
handles use scoped `NativePointer` accessors marked `unsafe`—callers honor
backend synchronization and lifetime rules.

Safe Rust borrowing prevents reentrant session calls through the same handle
while a frame is acquired. Backend `NativePointer` values are tied to the frame
lifetime and cannot escape the closure.
