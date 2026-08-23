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

Any host thread can submit runtime and map work. Submissions wake the native run
loop, so progress never depends on a display callback or a host pump. One
runtime may own multiple maps; their commands, queries, barriers, and release
work share one ordered submission stream.

Use a runtime barrier when later work must wait for every preceding submission
to reach a terminal disposition. The runtime's direct event wake callback tells
the host when its event queue is ready to drain.

## Map

A map belongs to a runtime. It owns style documents, sources, layers, images,
camera state, observer events, and render invalidation.

Closing a map consumes its public handle synchronously and returns a completion
for native retirement. That completion runs after earlier map work is terminal
and map-owned callback state has been destroyed. Await it when later host work
depends on cleanup; a runtime close also remains ordered after it.

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

Published snapshots provide synchronous copies of state needed by UI and display
threads. Snapshot reads never call into mutable MapLibre map state. Each
committed command completion reports the snapshot generation that its commit
published, so a host can fence a snapshot read on it.

## Render session

A render session renders one map to one render target. A map carries at most one
live render session. Feature state belongs to the render session, because
MapLibre stores it in the session's render state: the session's feature-state
and query operations read and mutate it in session order.

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
a native serial graphics worker for transferable Metal or Vulkan state, private
EGL owned texture targets, or transferred `OffscreenCanvas` WebGL state. A
caller-graphics-thread session stores typed work until the host services it
where the graphics context is usable. WGL targets, EGL surfaces, shared EGL
textures, existing WebGL contexts, and browser WebGPU use the caller driver.

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

Closing a map or disabling offline-region observation prevents future events
from that source and leaves queued events unchanged. Each queued event keeps a
copied source ID that remains meaningful after the source handle closes.

## Failures

The status returned by an immediate call reports validation or inspection
failure. The status returned by a one-shot submission reports whether native
code accepted and copied it. Its completion reports an asynchronous application
failure and a borrowed diagnostic that the binding copies before returning.

Each binding surfaces these channels in its own idiom: an exception, a result
type, an asynchronous result, or an event stream. Render-driver calls continue
to report their graphics-thread failures directly.

## Language bindings

Language bindings preserve the runtime, map, render session, and event model in
the target language. They sit directly above the C API and expose the same
objects and relationships, adding language-appropriate safety around handles,
lifetimes, errors, and event draining.
