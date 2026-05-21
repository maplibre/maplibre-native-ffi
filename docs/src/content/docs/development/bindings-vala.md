---
title: Vala Binding Conventions
description: Language-specific implementation conventions for Vala bindings.
---

Resources:

- Tracking issue:
  [#119](https://github.com/maplibre/maplibre-native-ffi/issues/119)
- [Vala manual](https://docs.vala.dev/)
- [Vala bindings documentation](https://docs.vala.dev/developer-guides/bindings.html)
- [Generating a VAPI with GObject Introspection](https://docs.vala.dev/developer-guides/bindings/generating-a-vapi-with-gobject-introspection.html)
- [GObject API reference](https://docs.gtk.org/gobject/)
- [GObject Introspection](https://gi.readthedocs.io/)
- [Rust `glib` crate](https://docs.rs/glib/)

## Architecture

The Vala binding presents a GLib/GObject-shaped low-level API over the public
MapLibre Native C API. Maintain it as a Rust `glib` adapter over the shared Rust
ABI-adaptation crates. The Rust adapter implements GObject classes, boxed
values, `GError` mapping, handle lifetime, callback state, and thread-safety
policy.

Binding metadata drives the repetitive ABI surface. A generator derives
scanner-facing annotated C headers and Rust `extern "C"` entry stubs for the
GObject-style ABI: type declarations, function prototypes, transfer ownership,
nullability, `out`/`inout`, array lengths, closure and destroy-notify metadata,
and error behavior. The generated C headers describe the compiled Rust shared
library to GObject Introspection; they are not a separate C implementation.

`g-ir-scanner` consumes the generated annotated headers and compiled Rust
library to produce GIR and typelib files. `vapigen` produces the generated
`.vapi`. Do not maintain a manual `.vapi` over `maplibre_native_c.h`: the raw C
API does not carry the GObject classes, `GError`, boxed values, callback
ownership, or introspection annotations needed for safe Vala use. When
`g-ir-scanner`, `vapigen`, or a small Vala compile test loses a safety contract,
fix the binding metadata, generator, or Rust adapter first.

Keep this layer low-level. It exposes runtime, map, render-session, resource,
event, camera, geometry, and render-target concepts directly. GLib main-loop
integration, widgets, async frameworks, and application lifecycle policy belong
above this binding.

## Public Types

Long-lived native objects are `GObject` classes with the shared `Handle` suffix.
Each class stores the native pointer privately, records live or closed state,
and exposes `close() throws MaplibreNative.Error`. Successful `close()` releases
exactly once; later calls no-op. Failed native destruction leaves the object
live so callers can retry on the owner thread.

Child objects hold strong GObject references to parents while native validity
depends on them: maps retain runtimes, render sessions retain maps, and
style-scoped objects retain their owner. `dispose` releases managed references
after native unregistration or `close()` makes callbacks unreachable and
in-flight callbacks complete. `finalize` reports leaks; it must preserve native
callback state that may still be reachable from leaked thread-affine handles.
`MapProjectionHandle` follows the shared exception: it owns a standalone
projection snapshot and does not retain its source `MapHandle` for native
validity after creation.

Copied descriptors, events, snapshots, camera values, geometry values, and
render metadata are GLib-friendly value objects. Use boxed types for immutable
or copyable value domains that need identity at the ABI boundary. Mutable C
option structs become GLib objects or boxed descriptors with explicit setters;
the adapter writes `size` fields, field masks, UTF-8 views, arrays, and nested
native descriptors internally.

`NativePointer` is a boxed value for borrowed backend-native addresses. It wraps
a non-null `void*`, grants no memory access, and transfers no ownership. Use it
only where the C API already accepts or returns opaque backend handles.

## Errors, Metadata, and Ownership

Expose one public GLib error domain that maps C status categories directly. Vala
sees it as an `errordomain`:

```vala
public errordomain MaplibreNative.Error {
    INVALID_ARGUMENT,
    INVALID_STATE,
    WRONG_THREAD,
    UNSUPPORTED,
    NATIVE_ERROR,
    UNKNOWN_STATUS
}
```

Every status-returning operation captures the native thread-local diagnostic
immediately on the same thread and stores it in the `GError` message. The
adapter validates GLib-owned state before crossing into C: closed wrappers,
active frame borrows, nullable arguments, embedded NUL in null-terminated
strings, one-shot resource request completion, and callback scope. The C API
continues to validate native state, enum domains, ranges, and owner-thread
rules.

Treat GIR annotations as part of the ABI. Each public function and property must
describe transfer mode, nullability, `out`/`inout`, array length, closure,
destroy notify, and error behavior accurately. Fix annotations at the adapter
source rather than patching generated VAPI files. Generated VAPI output, GIR
diffs, and Vala compile tests are review artifacts for API changes.

## Threading, Events, and Callbacks

Owner-thread-affine methods run on the calling thread. The binding does not
dispatch internally. Native `MLN_STATUS_WRONG_THREAD` becomes
`MaplibreNative.Error.WRONG_THREAD` with the copied diagnostic. Higher-level
GLib adapters may add `GMainContext` dispatch or async APIs; this layer keeps
native thread identity visible.

Runtime event polling returns copied GLib/Vala values. Signals may mirror events
as a convenience only while the owner thread explicitly drains the runtime event
queue. A signal must not imply background pumping, main-loop ownership, or UI
thread handoff.

Callback adapters keep closures, destroy notifies, and user data alive for the
native owner scope. Native upcalls may arrive on worker, network, logging, or
render-related threads, so callback state must be safe for those threads. The
adapter catches Vala/GLib failures and converts them to the documented C
callback behavior; exceptions must not unwind through native frames.

Resource-provider callbacks copy borrowed request fields before user code can
retain them. Matching requests own a handled request object that completes or
releases the C request handle exactly once, possibly from another thread when
the C API allows it. Pass-through requests return immediately and do not retain
the native handle. Custom geometry callbacks stay map/style scoped, track active
upcalls, and hand work to the map owner thread before calling thread-affine map
APIs.

## Rendering and Memory

`RenderSessionHandle` represents one attached render target for one map and
keeps the map wrapper alive. Surface and borrowed-texture descriptors carry
backend handles as `NativePointer`; callers keep those backend objects valid and
synchronized for the C API's documented lifetime.

Session-owned texture targets expose explicit frame handles. A frame handle
keeps the native frame acquired, exposes copied metadata and scoped
`NativePointer` values, and releases on `close()`. Access after close reports a
binding error. The binding prevents nested acquisition and rejects render
updates, resize, detach, and session destruction while a frame is active.

Temporary native storage lives for one adapter call. Native result, snapshot,
and list handles stay private; internal guards copy data into GLib-owned values
and release native handles on every exit path. Large reusable readback storage
may use GLib bytes or an explicit native buffer type with deterministic release.

## Testing

Treat successful `g-ir-scanner`, typelib generation, `vapigen`, and Vala
compilation as the bindability gate. Add Vala tests against the real adapter for
ownership transfer, nullability, `GError` mapping, diagnostic capture, closed
handles, wrong-thread statuses, callback replacement, one-shot resource request
completion, event copying, and frame lifetime. When the C ABI already proves
native behavior, the Vala test proves that GLib annotations and wrapper policy
preserve that behavior at the Vala boundary.
