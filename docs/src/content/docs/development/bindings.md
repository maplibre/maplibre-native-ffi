---
title: Binding Conventions
description: Shared design rules for low-level language bindings over the C API.
sidebar:
  order: 3
---

Language bindings sit directly above the public C API. They preserve the C API's
core model while adding language-appropriate ownership, memory, error, and
threading rules.

See the language-specific binding conventions in this section for implementation
choices in each target language.

Code fragments in this page are representative pseudocode. They show API shape
while leaving spelling and syntax to each language.

## Design Priorities

Bindings balance cross-language consistency with target-language conventions.
Use these priorities:

1. Protect host programs according to target-language safety conventions. Public
   APIs manage ownership, validate binding-owned state, and translate C statuses
   and diagnostics into language-appropriate errors.
2. Preserve the C API model. Bindings expose the same core concepts and
   operations so docs and examples transfer across languages.
3. Keep wrappers regular. Prefer generated or mechanically derived code from the
   public headers and docs. Handwritten code follows simple, repeatable
   patterns.
4. Respect language idioms at the boundary. Use idiomatic naming, resource
   management, errors, callbacks, and memory tools where they preserve the
   shared low-level model.

## API Shape

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

## Binding Layers

Separate raw C access from the public binding.

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

Keep C binding-generator types internal. Public APIs expose target-language
values, descriptors, errors, and handle wrappers rather than raw ABI structs or
generated layout classes.

Rust-based native-extension bindings may share internal Rust crates. A generated
`-sys` crate mirrors the C ABI and contains no binding policy. A support crate
holds shared implementation glue such as status checking, diagnostic copying,
string and descriptor helpers, memory guards, callback-boundary utilities, and
native-pointer utilities. The support crate is not the public safety layer;
public Rust, Python, and Node packages keep their own language-facing ownership,
errors, callbacks, and packaging.

## Status And Diagnostics

Status-returning C calls either complete normally or report errors through the
language's ordinary mechanism: exceptions, error values, typed results, or error
unions.

Map each C status category to a stable, idiomatic public error representation.
Use target-language spelling and mechanics, but keep the category visible to
callers. When a native call returns a non-OK status, read the C thread-local
diagnostic immediately on the same thread and include it in the reported error.
Another C call on that thread may replace the diagnostic.

Let the C API validate native arguments and native state. The binding validates
language-owned state such as released wrappers, active callback-scoped borrows,
threading promises made by the binding, and one-shot resource request
completion.

## Owned Handles

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
language's memory model, this may use strong references, ownership or lifetimes,
ARC retention, or a documented destruction order.

`MapProjectionHandle` is created from a `MapHandle`, owns a standalone transform
snapshot, and releases with `mln_map_projection_destroy()`. Later map camera or
projection changes do not update it. It does not depend on the source
`MapHandle` for native validity after creation.

## Owner Threads

Mirror the C API owner-thread model in documentation and errors.

Runtime creation records the runtime owner thread in native code. Map creation
currently runs on the runtime owner thread and makes that same thread the map
owner thread. Projection helper creation runs on the map owner thread and
returns a projection owned by that thread. Surface and texture attachment
currently create render sessions whose session owner thread is the map owner
thread.

Bindings can rely on native owner-thread validation for ordinary calls. Native
`MLN_STATUS_WRONG_THREAD` results become the language's wrong-thread error.

Keep public type boundaries aligned with C owner concepts:

```text
RuntimeHandle         runtime owner thread in C
MapHandle             map owner thread in C
MapProjectionHandle   projection owner thread in C
RenderSessionHandle   session owner thread in C
```

This leaves room for future C APIs that expose render sessions owned by a render
thread distinct from the runtime owner thread. If a binding needs to inspect
owner threads directly, add a C getter.

Resource provider request completion follows the C API and may run from any
thread. Cross-thread dispatch, coroutine confinement, and UI-thread handoff
belong in adapters above this layer unless a language binding explicitly owns a
small safety helper.

## Options And Transparent Structs

Model C option structs as language-owned descriptor objects. Mutating descriptor
methods use explicit setters, builders, or the language's equivalent and update
any corresponding field mask. Field-mask structs use an empty/default
constructor plus explicit ways to mark fields present or absent.

The binding initializes `size` fields and masks internally. Callers set semantic
fields, not ABI bookkeeping fields.

A type uses one update style consistently. Mutable descriptor types use setters.
Immutable value types use copy, withers, builders, or the language's equivalent.

Most input descriptors store language fields and materialize native structs at
the call boundary. Use object-owned native memory only when the language object
represents storage that C fills or later consumes.

## Native Memory

Choose native storage by lifetime:

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

Temporary pointers live only until their allocation scope exits. Pass them only
to C calls that consume or copy them before returning. Use explicit ownership
for storage whose address must outlive one call, and release it exactly once
from the owning object.

## Strings

Encode target-language strings as standard UTF-8 at the C boundary.

The binding rejects strings containing embedded `NUL` for null-terminated
`const char*` inputs, because C would see a truncated string. Explicit-length
`mln_string_view` inputs store UTF-8 bytes plus a byte length and keep that
storage alive for the call or callback scope that borrows it.

Copy C strings and string views into target-language strings or byte values
before their borrow window ends. Use the byte length supplied by the C API when
one is available; use null termination only for C fields documented as
`const char*`.

## Native Pointers

A `NativePointer`-style value represents an opaque backend-native address that
the binding does not own. It is a value object, not a memory view.

Use this value for C `void*` fields that represent host-owned backend objects,
such as Metal devices, Metal textures, Vulkan devices, Vulkan queues, Vulkan
images, Android native windows, hardware buffers, and native surfaces. Passing
it transfers no ownership and grants no memory access.

Public APIs accept or return opaque native-pointer values only where the C API
already accepts or returns an opaque backend-native handle. The internal C layer
converts that value to the pointer representation required by the language's FFI
mechanism.

## Callback-Scoped Borrows

A callback-scoped borrow is native data exposed only during a language callback.
The binding acquires the native borrow before invoking the callback and releases
it with the language's cleanup mechanism after the callback returns or fails.

Owned texture frames use callback-scoped access. The callback receives a frame
view whose native handles are valid only during that callback. The frame type is
not publicly closeable. Unsafe native accessors check that the frame is active.

The native session rejects nested acquisition, render updates, resize, detach,
and destroy while a frame is acquired. The binding relies on those native checks
and always releases the frame after the callback scope ends.

## Borrowed Data

Borrowed C data becomes copied language data unless it is exposed through a
callback-scoped borrow.

Copy event payloads, messages, and strings before their C event storage window
ends.

Native snapshot, result, and list handles are short-lived implementation details
by default. Public snapshot and result objects own copied language data. If a
binding exposes a lazy view tied to a native snapshot handle, that view owns and
releases the native handle and never exposes free-floating borrowed views.

Backend-native handles returned from acquired texture frames are callback-scoped
borrows represented as opaque native-pointer values.

## Events

Expose runtime event polling as copied language values. A drain helper may exist
when it has the same semantics. Events remain runtime-owned in the C API. Public
event objects are independent of the next native poll.

A runtime wrapper may keep a registry of live map wrappers keyed by native map
pointer. Map-originated events can attach the matching live `MapHandle` or carry
copied source metadata for diagnostics. The C API discards queued events before
their source handle becomes invalid, so pointer lookup does not match a later
object that reused the same native address.

The low-level binding preserves event names and payload categories close to the
C API. Translating events into listeners, flows, promises, coroutines, or UI
state belongs to adapters above this layer.

## Native Callbacks

Keep callback lifetimes explicit and tied to the C API owner scope.

A language callback is a function, closure, interface implementation, delegate,
or callable object stored by the binding. Native code calls a C-compatible
adapter. The adapter copies or wraps callback arguments according to this
document, invokes the language callback, and converts the result back to C data.

Store callbacks strongly for the native lifetime that can invoke them:

- Process-global logging callbacks live outside any `RuntimeHandle`. Their
  callback state remains live until the callback is replaced or cleared.
- Runtime-scoped resource transforms and resource providers live with their
  `RuntimeHandle` and outlive all native requests that can invoke them.
- Map/style-scoped custom geometry source callbacks live until the source is
  removed, the style is replaced, or the map is closed, and until any in-flight
  callback invocation has returned.
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

Callback documentation carries forward C callback restrictions that remain
visible to users. Resource provider callbacks may run on worker or network
threads, so implementations return quickly and avoid map or runtime methods from
the callback. Custom geometry source callbacks marshal work to the map owner
thread before calling thread-affine map APIs. Borrowed request fields are copied
before the callback returns when the binding needs them later.

Resource-provider bindings use native-owned routing rules before crossing into
language code. Non-matching requests return pass-through immediately. Matching
requests copy request data, own the provider's request handle, and complete
inline or later according to the language's callback model.

A handled resource request owns the provider's reference to the C request
handle. It enforces one-shot completion and exactly-once release. Completion and
cancellation checks may run from any thread when the C API allows it.

## Render Sessions And Render Targets

`RenderSessionHandle` represents one attached render target for one map. Current
C attachment APIs make the session owner thread the map owner thread. The type
remains distinct because render sessions have separate lifecycle and render
target state.

Attach methods return a `RenderSessionHandle`. Surface descriptors and
caller-owned texture descriptors contain backend-native handles. The binding
treats those handles as borrowed. The caller keeps backend objects valid and
synchronized for the lifetime documented by the C API.

Session-owned texture targets expose rendered backend objects through
callback-scoped frame access. CPU readback APIs copy into language arrays, byte
buffers, or explicit native buffers and return copied `TextureImageInfo`
metadata.

## Unsafe Escape Hatches

Backend interop requires raw native handles in specific render-target APIs.
Unsafe accessors are limited to those APIs.

Mark unsafe accessors visibly with the target language's unsafe convention.
Unsafe accessors document the scope in which the returned native handle is
valid. They transfer no ownership and expose no general native memory access.

## Constants And Enums

Expose C enum domains as language enums when the domain is closed and type-safe.
Map values explicitly. Language enum ordinals are not ABI values.

Expose C bit masks as enum sets, option sets, purpose-built value types, or the
language's idiomatic mask wrapper. Keep raw integer constants internal where the
language can represent the domain safely.

Output values that may grow across C API versions use stable unknown-value
representations where forward compatibility matters.

## Testing

Focus binding tests on the language adaptation layer. C ABI tests prove native
behavior. Binding tests prove generated or handwritten wrappers, ownership,
copying, callbacks, threading errors, and error mapping preserve that behavior
at the language boundary.

Prefer small adaptation tests around real C calls. When a C ABI test already
covers native validation, the binding test only needs to show that the
corresponding language function propagates the native status, diagnostic, or
copied output correctly.

Add regression tests when the binding owns a lifetime or threading invariant
that raw C declarations cannot express, such as releasing a texture frame after
a failing callback, preserving parent validity while child handles are live, or
releasing callback references exactly once.
