---
title: Concepts
description: Core mental models for using MapLibre Native FFI.
---

MapLibre Native FFI exposes MapLibre Native concepts directly. A host uses a
language binding or calls the C API, and higher-level adapters build on the same
model.

Three objects form the core API: the runtime, the map, and the render session.
Events and bindings connect those objects to host code.

## Runtime

The runtime owns one native scheduler thread and its event storage. Runtime
creation starts that thread, which keeps MapLibre Native's run loop active until
native teardown finishes after runtime release.

Any host thread can submit runtime and map work. A submission wakes the native
run loop, and the runtime's own thread carries the work forward. One runtime may
own multiple maps; their commands, queries, barriers, and release work share one
ordered submission stream.

Use a runtime barrier when later work must wait for every preceding submission
to reach a terminal disposition. The runtime's direct event wake callback tells
the host when its event queue is ready to drain.

A process may exit while runtimes and maps are live. Native threads keep running
until the operating system ends the process, and nothing that they use is
destroyed at exit. Once exit begins, the library dispatches no further host
callback, and a completion that is still pending never runs. A binding whose
language runtime shuts down before exit begins stops native callbacks into it
first: the Python binding releases every runtime before the interpreter
finalizes.

On Windows, exit begins only when the operating system ends the process's other
threads, after the host's exit handlers and static destructors have run. A host
exit handler or static destructor there that tears down state its callbacks use
releases each runtime and waits for its release completion first.

A render session renders through the host's graphics driver, and some drivers
tear down their own state at exit before the library's exit handler runs. So
before the process exits, end the graphics calls of every render session:
abandon the session, or detach it and wait for the detach completion. For a
session that a host graphics thread drives, stop driver service first. Abandon
is synchronous, so an exit path can use it on a session that is mid-frame.

## Map

A map belongs to a runtime. It owns style documents, sources, layers, images,
camera state, feature state, observer events, and render invalidation.

Releasing a map consumes its public handle synchronously; the release accepts a
completion that runs after native retirement. That completion runs after earlier
map work is terminal and map-owned callback state has been destroyed. Backend
worker and graphics resource cleanup can continue after that completion. Await
it when later host work depends on cleanup; a runtime release also remains
ordered after it.

A map is independent of a render target. The host can create, configure, query,
and observe a map before the first frame.

Sources and layers use style-spec JSON. This representation keeps the API
aligned with the style specification across every layer type. Typed entry points
cover behavior beyond construction, such as source-type validation and per-frame
property updates.

Map mutations are commands. A command copies its input before returning
acceptance and later invokes one completion with its terminal disposition.
Ordered queries and lifecycle transitions use typed completions. Bindings expose
one-shot work through their normal future, promise, task, suspension, or
explicit async idiom.

Cancelling or timing out a binding's wait ends only that wait. The native work
continues to its terminal disposition, and its completion still runs. When the
host cancels the wait in the language's idiom, such as task cancellation, a
cancellation token, cancelling the future, or dropping it, the binding releases
a created handle that arrives afterward. A timeout that leaves the future
uncancelled keeps the handle in the future for its other listeners. If the host
then drops that future, the collector reclaims the handle and reports a leak
where the binding reports leaks. A Go context ends one wait, and the collector
retires the handle of a dropped Go future. Dart futures have no cancellation, so
a Dart wait can only time out, and the future still delivers its result.

Published snapshots provide synchronous copies of state needed by UI and display
threads. Snapshot reads never call into mutable MapLibre map state. Each
committed command completion reports the snapshot generation that its commit
published, so a host can fence a snapshot read on it.

## Render session

A render session renders one map to one render target. A map carries at most one
live render session. Feature state belongs to the map; a session pushes the
map's store into its renderer on the next render update, and the session's
queries read the last frame the session drew.

Render targets come in three kinds:

| Render target           | Owned by | Renders                                    |
| ----------------------- | -------- | ------------------------------------------ |
| native surface          | caller   | To a window, view, or canvas, and presents |
| owned texture target    | session  | Offscreen, into a session allocation       |
| borrowed texture target | caller   | Offscreen, into a caller allocation        |

Keeping render sessions separate from maps lets the host manage the graphics
backend lifecycle independently.

The host supplies the graphics implementation. A render target names a context
and a surface that the host created, so the library binds its graphics entry
points to the implementation already in the process rather than to a copy of its
own. A process then holds a single implementation, and handles that it mints
stay valid everywhere they are passed.

Most platforms provide that implementation. Apple provides neither EGL nor
Vulkan, so a macOS host loads an EGL implementation such as ANGLE for the OpenGL
backend, or MoltenVK for the Vulkan backend. That implementation brings the
headers to build against.

Execution placement is fixed when attachment starts. A core-worker session owns
a native serial graphics worker. A caller-graphics-thread session stores typed
work until the host services it where the graphics context is usable. The target
decides which drivers it accepts:

| Render target                                               | Driver                 |
| ----------------------------------------------------------- | ---------------------- |
| Metal surface or texture                                    | either                 |
| Vulkan surface or texture                                   | either                 |
| OpenGL surface on WGL, EGL, or an existing WebGL context    | caller graphics thread |
| OpenGL surface on a transferred `OffscreenCanvas`           | core worker            |
| OpenGL owned texture on a shared WGL, EGL, or WebGL context | caller graphics thread |
| OpenGL owned texture on a private EGL context               | core worker            |
| OpenGL borrowed texture                                     | caller graphics thread |
| WebGPU surface or texture                                   | caller graphics thread |

Session control is separate from graphics execution. Any host thread may request
a frame, read a snapshot, start an asynchronous call, abandon a target, or
destroy a detached session. The first successful caller-driver service fixes its
graphics thread identity. Later service calls and thread-current backend
accessors remain affine to that thread. The host services ready work even while
presentation callbacks are paused.

A frame demand carries a host token, an optional timeout, and a coalescing
boundary. Every accepted demand produces one terminal result. Result records
identify the token and the map-update, extent, and frame generations that the
driver used. A direct frame-result wake callback remains armed until the host
drains all frame results, so coalesced wakeups do not lose results.

Host-acquirable owned texture targets negotiate a ring of one to three slots.
Acquiring a frame leases one slot and returns producer-completion
synchronization. Releasing the frame supplies consumer-completion
synchronization when the host submitted GPU reads. The driver reuses the slot
only after the host released the handle and those reads completed. A private
OpenGL owned texture target fixes its ring depth at one and exposes CPU readback
instead of frame acquisition.

### OpenGL context ownership

OpenGL binds a context to a thread, so an OpenGL render target names how the
session and the host divide driver-thread context and graphics-object ownership.

A shared session leaves the thread as it found it. Each driver-service call
makes the session's context current and restores whatever was current before,
and that context joins the host share group. Host-acquirable texture targets and
existing WebGL contexts use this mode.

A dedicated session owns its driver thread's context. It creates a context from
the supplied display or device, joins no host share group, and keeps that
context current between renders. A surface target can use a caller thread that
exists to draw one map, such as an Android host rendering into a `SurfaceView`.
A private EGL owned texture target uses a core worker and exposes CPU readback.

## Events

Events preserve MapLibre Native's observer-driven model across the FFI boundary.
The runtime copies events into host-visible storage, and host code drains those
events from the runtime.

Events report map lifecycle, rendering progress, resource activity, diagnostics,
and asynchronous failures.

Rendering observer events reach the runtime queue asynchronously. The runtime's
direct event wake callback reports that the queue is ready to drain.

Each map and each runtime carries a subscription: the set of event types it
queues. Default options select every event type the library reports, and a host
narrows a subscription by naming the types it reads. An unselected event is
never built, never queued, and never invokes the event wake callback.

One drain transfers the queued event records and their message storage into an
owned batch. A batch remains readable across later drains and runtime close.
Copy values that must outlive the batch, then release it.

Releasing a map or disabling offline-region observation prevents future events
from that source and leaves queued events unchanged. Each queued event keeps a
copied source ID that remains meaningful after the source handle closes.

## Failures

The status returned by an immediate call reports validation or inspection
failure. The status returned by a one-shot submission reports whether native
code accepted and copied it. Either call writes its failure message into the
caller's `mln_diagnostic`, the last parameter of every status-returning
function. A submission's completion reports an asynchronous application failure
and a borrowed diagnostic that the binding copies before returning.

Each binding surfaces these channels in its own idiom: an exception, a result
type, an asynchronous result, or an event stream. Render-driver calls report
their graphics-thread failures in their returned status.

A style entity that doesn't exist, such as a source, layer, or image ID, is
reported through the completion. A command that targets it completes with
`MLN_STATUS_NOT_FOUND`. A query that reads a whole entity succeeds with no
value. A query that reads one attribute of a missing layer completes with
`MLN_STATUS_NOT_FOUND`.

## Layer plugins

MapLibre Native's plugin API lets native code add style layer types. A plugin
declares paint properties and shaders for each render backend, lays out tile
features into vertex data on tile workers, and fills uniform blocks on the
render thread. The renderer owns the GPU resources and draws the result like any
other layer, so a style names the layer type and sets its paint properties the
same way it does for built-in layers.

This library builds plugin support into its core and exports the registration
entry point. A plugin registers once per process, before a style that uses its
layer types loads. Registration retains the plugin's callbacks for the rest of
the process, and those callbacks run on MapLibre's threads, so a plugin is
native code. Language bindings expose the registration function for plugin
integrations; plugin authoring uses the raw C contract. The plugin API declares
shaders for OpenGL, Vulkan, and Metal; a WebGPU build registers a plugin but has
no shader path for its layers.

Plugin integrations obtain the host's registration function through
`mln_plugin_get_register_function_v1()` and pass it to their own registration
entry point. The integration loads the plugin and keeps its code loaded for the
process lifetime. The plugin registers its descriptors through the supplied
function, into the host's copy of MapLibre Native.

## Language bindings

Language bindings preserve the runtime, map, render session, and event model in
the target language. They sit directly above the C API and expose the same
objects and relationships, adding language-appropriate safety around handles,
lifetimes, errors, and event draining.

Each binding tracks the lifecycle of the handles that it owns and reports misuse
before any native call. A call on a closed handle, a call while the handle's
close is in progress, and a close while a call or borrowed view still holds the
handle each raise the binding's invalid-state error. That error carries no
native status, and its message names the handle type and its state: closed,
closing, or in use. Closing a handle that is already closed does nothing.
