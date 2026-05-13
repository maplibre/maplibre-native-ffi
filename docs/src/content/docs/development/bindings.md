---
title: Binding Conventions
description: Shared design rules for low-level language bindings over the C API.
sidebar:
  order: 3
---

Language bindings sit directly above the public C API. They preserve the C API's
core model while adapting ownership, memory, error, and threading to the target
language's conventions.

See the language-specific binding conventions in this section for implementation
choices in each target language.

## Naming

Use "MapLibre" when referring to the project or upstream library in prose.
Inside code identifiers, treat `maplibre` as one word and apply the target
language's normal casing rules:

```text
MaplibreMap
maplibre_map
maplibreMap
MAPLIBRE_MAP
```

Avoid splitting the brand into separate identifier words such as `MapLibreMap`
or `map_libre_map`.

## Design Model

Bindings balance cross-language consistency with target-language conventions:

1. Protect host programs. Public APIs manage ownership, validate binding-owned
   state, and translate C statuses and diagnostics into language-appropriate
   errors.
2. Preserve the C API model. Bindings expose the same core concepts and
   operations so docs and examples transfer across languages.
3. Keep wrappers regular. Prefer generated or mechanically derived code from the
   public headers and docs. Handwritten code follows simple, repeatable
   patterns.
4. Respect language idioms at the boundary. Use idiomatic naming, resource
   management, errors, callbacks, and memory tools where they preserve the
   shared low-level model.

Owned long-lived native objects use a `Handle` suffix:

```text
RuntimeHandle
MapHandle
MapProjectionHandle
RenderSessionHandle
```

`Handle` means the object wraps an explicitly releasable native object with
identity across operations. Language-owned values, descriptors, events, copied
data, and one-shot snapshots omit the suffix. Keep public names close to C
concepts, and rename them when the target language gains clarity or avoids
namespace collisions.

Separate raw C access from the public binding:

```text
internal c layer
  Generated or handwritten declarations that call the public C headers.

internal support layer
  Status conversion, diagnostics, handle state, memory helpers, native library
  loading, callback bridging, and low-level utilities.

public binding layer
  Low-level API with stable names, ownership rules, diagnostics, and lifetime
  control.
```

Bindings use one of two implementation paths. A direct C import generates or
writes the internal C declarations from the public headers, then wraps them—this
works when the target language can consume C headers and call C functions as
part of its normal package model. A bridge library is built in Rust over the
shared `-sys` and ABI-adaptation crates, then adapted to the target runtime—this
is needed when the package boundary is a native extension or native-method entry
point (Python, Node.js, Java JNI) or an object and introspection ABI
(GLib/GObject for Vala). Both paths keep raw ABI details internal.

## Handle Lifetime

Every long-lived C-owned opaque handle maps to a public `*Handle` with
deterministic release. The release operation calls the matching C destroy
function. A successful release makes later release calls no-ops. If C reports
`MLN_STATUS_WRONG_THREAD`, the binding reports the matching wrong-thread error
with the native diagnostic.

Destroy operations are fallible for thread-affine handles. Public close,
destroy, or release APIs report native status through the language's ordinary
error mechanism. Finalizers, cleaners, `Drop` implementations, and garbage
collector callbacks may run on the wrong thread or in the wrong order; use them
for leak reporting unless the binding can prove owner-thread destruction and
dependency order.

A handle stores:

- the native pointer as private implementation state;
- the parent relationship needed for native validity;
- live or released state;
- optional debug leak context.

Bindings preserve parent validity while child handles are live. Depending on the
language's memory model, this uses strong references, ownership or lifetimes,
ARC retention, or a documented destruction order.

`MapProjectionHandle` is created from a `MapHandle`, owns a standalone transform
snapshot, and releases with `mln_map_projection_destroy()`. Later map camera or
projection changes do not update it. It does not depend on the source
`MapHandle` for native validity after creation.

The C API owner-thread model constrains handle lifecycle:

```text
RuntimeHandle         runtime owner thread
MapHandle             map owner thread
MapProjectionHandle   projection owner thread
RenderSessionHandle   session owner thread
```

Runtime creation records the owner thread. Map creation runs on the runtime
owner thread. Projection helper creation runs on the map owner thread. Surface
and texture attachment create render sessions whose owner thread is the map
owner thread. Bindings rely on native owner-thread validation for ordinary
calls. Native `MLN_STATUS_WRONG_THREAD` results become the language's
wrong-thread error. If a binding needs to inspect owner threads directly, add a
C getter.

Resource provider request completion may run from any thread. Cross-thread
dispatch, coroutine confinement, UI-thread handoff, and framework scheduling
belong in adapters above this layer. A binding may add a small opt-in
owner-thread helper when the language scheduler separates logical execution from
native thread identity—limited to generic owner-thread execution, runtime
pumping, and event draining.

Status-returning C calls either complete normally or report errors through the
language's ordinary mechanism: exceptions, error values, typed results, or error
unions. Map each C status category to a stable, idiomatic public error
representation. When a native call returns a non-OK status, read the C
thread-local diagnostic immediately on the same thread and include it in the
reported error—another C call on that thread may replace the diagnostic. The C
API validates native arguments, state, numeric ranges, enum-domain semantics,
and MapLibre-specific rules. The binding validates language-owned state:
released wrappers, active callback-scoped borrows, threading promises, and
one-shot resource request completion.

## Type Mapping

C option structs become language-owned descriptor objects. Mutating descriptor
methods use explicit setters, builders, or the language's equivalent and update
any corresponding field mask. Field-mask structs use an empty/default
constructor plus explicit ways to mark fields present or absent. The binding
initializes `size` fields and masks internally—callers set semantic fields, not
ABI bookkeeping. A type uses one update style consistently: mutable descriptor
types use setters, immutable value types use copy, withers, builders, or the
language's equivalent.

Most input descriptors store language fields and materialize native structs at
the call boundary. Object-owned native memory is used only when the language
object represents storage that C fills or later consumes.

C enum domains become language enums when the domain is closed and type-safe.
Map values explicitly—language enum ordinals are not ABI values. C bit masks
become enum sets, option sets, purpose-built value types, or the language's
idiomatic mask wrapper. Output values that may grow across C API versions use
stable unknown-value representations; copied native output may also preserve the
raw native value alongside the mapped value for diagnostics.

A `NativePointer`-style value represents an opaque backend-native address that
the binding does not own. It is a value object. Use it for C `void*` fields that
represent host-owned backend objects: Metal devices and textures, Vulkan
devices, queues, and images, Android native windows, hardware buffers, native
surfaces. Passing it transfers no ownership and grants no memory access. Public
APIs accept or return opaque native-pointer values only where the C API already
accepts or returns an opaque backend-native handle.

Strings encode as standard UTF-8 at the C boundary. The binding rejects strings
containing embedded `NUL` for null-terminated `const char*` inputs, because C
would see a truncated string. Explicit-length `mln_string_view` inputs store
UTF-8 bytes plus a byte length.

## Data Ownership

Native storage falls into four lifetime categories:

```text
per-call temporary storage
  Option structs, selectors, temporary UTF-8 strings, out parameters, and
  scratch buffers.

object-owned native storage
  Reusable transparent structs, explicit native buffers, or native state that
  belongs to a public object.

large explicit buffers
  Caller-owned CPU texture readback buffers or other storage reused across
  frames.

scope-owned callback state
  Upcall stubs, function pointers, global references, StableRef values,
  delegates, closures, or adapter objects whose lifetime matches the C callback
  owner scope.
```

Temporary pointers live only until their allocation scope exits—pass them only
to C calls that consume or copy them before returning. Use explicit ownership
for storage whose address must outlive one call, and release it exactly once.

Borrowed C data becomes copied language data unless it is exposed through a
callback-scoped borrow. Copy event payloads, messages, and strings before their
C storage window ends. Copy C strings and string views before their borrow
window ends—use the byte length from the C API when available, and null
termination only for fields documented as `const char*`.

Native snapshot, result, and list handles are short-lived implementation
details. Public objects own copied language data. Internal readers release
native handles after copying, even when copying fails. A lazy view tied to a
native snapshot handle owns and releases that handle and never exposes
free-floating borrowed views.

A scoped borrow exposes native data only during a binding-controlled lifetime.
The binding acquires the borrow, exposes a scoped view, and releases it when the
scope ends. Session-owned texture frames use explicit frame handles: the frame
view is valid only while the handle is live, unsafe accessors check that active
state, and the native session rejects nested acquisition, render updates,
resize, detach, and destroy while a frame is acquired.

Runtime event polling returns copied language values. Public event objects are
independent of the next native poll. A runtime wrapper may keep a registry of
live map wrappers keyed by native map pointer—map-originated events can attach
the matching live `MapHandle` or carry copied source metadata for diagnostics.
The C API discards queued events before their source handle becomes invalid, so
pointer lookup does not match a later object that reused the same native
address.

## Callbacks

Callback lifetimes are explicit and tied to the C API owner scope. A language
callback is a function, closure, interface implementation, delegate, or callable
object stored by the binding. Native code calls a C-compatible adapter that
copies or wraps callback arguments, invokes the language callback, and converts
the result back to C data.

Store callbacks strongly for the native lifetime that can invoke them:

- Process-global logging callbacks live outside any `RuntimeHandle`. Their state
  remains live until replaced or cleared.
- Runtime-scoped resource transforms and resource providers live with their
  `RuntimeHandle` and outlive all native requests that can invoke them.
- Map/style-scoped custom geometry source callbacks live until the source is
  removed, the style is replaced, or the map is closed, and until any in-flight
  invocation has returned.
- Handled resource request objects own the provider's reference to the C request
  handle until they complete or release it exactly once.

Callback state is thread-safe because callbacks may arrive on MapLibre worker,
network, logging, or render-related threads. Adapters do not assume the runtime
owner thread.

Callbacks catch language exceptions, panics, and errors and convert them to the
documented C callback behavior. They do not unwind through native code.

Synchronous callbacks must finish before native code continues. Bindings whose
runtimes cannot safely execute user code on arbitrary native threads keep those
callbacks internal, use a native shim, or expose only the callback shapes they
can honor without blocking MapLibre worker, network, logging, or render threads.

Resource provider callbacks may run on worker or network threads—implementations
return quickly and avoid map or runtime methods from the callback. Custom
geometry source callbacks marshal work to the map owner thread before calling
thread-affine map APIs. Borrowed request fields are copied before the callback
returns when the binding needs them later.

Resource-provider bindings use native-owned routing rules before crossing into
language code. Non-matching requests return pass-through immediately. Matching
requests copy request data, own the provider's request handle, and complete
inline or later. A handled resource request enforces one-shot completion and
exactly-once release; completion and cancellation checks may run from any thread
when the C API allows it.

## Rendering

`RenderSessionHandle` represents one attached render target for one map. Current
C attachment APIs make the session owner thread the map owner thread. The type
remains distinct because render sessions have separate lifecycle and render
target state.

Attach methods return a `RenderSessionHandle`. Surface descriptors and
caller-owned texture descriptors contain backend-native handles. The binding
treats those handles as borrowed—the caller keeps backend objects valid and
synchronized for the lifetime documented by the C API.

Session-owned texture targets expose rendered backend objects through explicit
frame handles. CPU readback APIs copy into language arrays, byte buffers, or
explicit native buffers and return copied `TextureImageInfo` metadata.

Backend interop requires raw native handles in specific render-target APIs.
Unsafe accessors are limited to those APIs, marked with the target language's
unsafe convention, and document the scope in which the returned native handle is
valid. They transfer no ownership and expose no general native memory access.

## Testing

Focus binding tests on the language adaptation layer. C ABI tests prove native
behavior. Binding tests prove that wrappers, ownership, copying, callbacks,
threading errors, and error mapping preserve that behavior at the language
boundary.

Prefer small adaptation tests around real C calls. When a C ABI test already
covers native validation, the binding test only needs to show that the
corresponding language function propagates the native status, diagnostic, or
copied output correctly.

Add regression tests when the binding owns a lifetime or threading invariant
that raw C declarations cannot express, such as invalidating a texture frame
after its scope closes, preserving parent validity while child handles are live,
or releasing callback references exactly once.
