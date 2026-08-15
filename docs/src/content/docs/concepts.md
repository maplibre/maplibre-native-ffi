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
runtime close completes.

Any host thread can submit runtime and map work. Submissions wake the native run
loop, so progress never depends on a display callback or a host pump. One
runtime may own multiple maps; their commands, operations, barriers, and close
requests share one ordered submission stream.

Use a runtime barrier when later work must wait for every preceding submission
to reach a terminal disposition. Use the runtime's receiver-scoped notification
source to wait for events and operation completions without polling.

## Map

A map belongs to a runtime. It owns style documents, sources, layers, images,
camera state, observer events, and render invalidation.

A map is independent of a render target. The host can create, configure, query,
and observe a map before the first frame.

Sources and layers use style-spec JSON. This representation keeps the API
aligned with the style specification across every layer type. Typed entry points
cover behavior beyond construction, such as source-type validation and per-frame
property updates.

Map mutations are commands. A command copies its input before returning
acceptance, receives an ID, and later reports a terminal disposition through the
runtime event queue. Ordered queries and lifecycle transitions are operations.
Bindings expose those operations through their normal future, task, suspension,
or explicit-wait idiom.

Published snapshots provide synchronous copies of state needed by UI and display
threads. Snapshot reads never call into mutable MapLibre map state. Each
committed command reports the snapshot generation that its commit published, so
a host that holds a command's terminal event can fence a snapshot read on it.

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
a native serial graphics worker for transferable Metal, Vulkan, or transferred
`OffscreenCanvas` WebGL state. A caller-graphics-thread session stores typed
work until the host services it where the graphics context is usable. WGL, EGL,
existing WebGL contexts, and browser WebGPU use the caller driver.

Session control is separate from graphics execution. Any host thread may request
a frame, read a snapshot, start an operation, abandon a target, or destroy a
detached session. The first successful caller-driver service fixes its graphics
thread identity. Later service calls and thread-current backend accessors remain
affine to that thread. The host services ready work even while presentation
callbacks are paused.

A frame demand carries a host token, presentation time, optional deadline, and
coalescing boundary. Every accepted demand produces one terminal result. Result
records identify the token and the map-update, extent, and frame generations
that the driver used. A notification source remains ready until the host drains
all frame results, so coalesced notifications do not lose results.

Owned texture targets negotiate a ring of one to three slots. Acquiring a frame
leases one slot and returns producer-completion synchronization. Releasing the
frame starts an operation and supplies consumer-completion synchronization when
the host submitted GPU reads. The driver reuses the slot only after the host
released the handle and those reads completed.

### OpenGL context ownership

OpenGL binds a context to a thread, so an OpenGL render target names how the
session and the host divide the thread's context. This policy is independent of
the caller-graphics-thread driver.

A shared session leaves the thread as it found it. Each driver-service call
makes the session's context current and restores whatever was current before,
and that context joins the host share group. Texture targets and WebGL use this
mode.

A dedicated session owns the thread's context. It creates a context from the
display or device that the host presents through, joins no host share group, and
keeps that context current between service calls. Choose it for a surface target
on a thread that exists to draw one map, such as an Android host rendering into
a `SurfaceView`.

## Events

Events preserve MapLibre Native's observer-driven model across the FFI boundary.
The runtime copies events into host-visible storage, and host code drains those
events from the runtime.

Events report map lifecycle, rendering progress, resource activity, diagnostics,
and asynchronous failures.

Rendering observer events reach the runtime queue asynchronously. A
receiver-scoped notification source reports that the queue is ready to drain.

Each map and each runtime carries a subscription: the set of event types it
queues. Default options select every event type the library reports, and a host
narrows a subscription by naming the types it reads. An unselected event is
never built, never queued, and never makes the notification source readable.

One drain creates an owned batch: every queued event in order, plus the message
text that those events carry. A batch remains readable across later drains and
runtime close. Copy values that must outlive the batch, then release it.

Queued events belong to their source. Closing a map discards that map's queued
events. Read any teardown state from an owned batch or published snapshot.

## Failures

The status returned by an Immediate call reports validation or inspection
failure. The status returned by a Command reports whether native code accepted
and copied the submission; its terminal event reports an asynchronous
application failure. An Operation stores its terminal status and a copied
diagnostic for inspection from any thread.

Each binding surfaces these channels in its own idiom: an exception, a result
type, an asynchronous result, or an event stream. Render-driver calls continue
to report their graphics-thread failures directly.

## Language bindings

Language bindings preserve the runtime, map, render session, and event model in
the target language. They sit directly above the C API and expose the same
objects and relationships, adding language-appropriate safety around handles,
lifetimes, errors, and event draining.
