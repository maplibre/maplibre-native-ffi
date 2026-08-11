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

The runtime owns scheduler state and event storage for one owner thread. The
host creates the runtime on the thread that will pump it. Runtime work and
events flow through that thread.

Each owner thread has at most one live runtime. Pumping advances MapLibre Native
and collects completed work.

The host sets the pace. A display-paced host pumps once per frame. A host with a
dedicated pump thread parks that thread until the runtime has work. Other host
threads wake it through a wake source.

## Map

A map belongs to a runtime. It owns style documents, sources, layers, images,
camera state, feature state, observer events, and render invalidation.

A map is independent of a render target. The host can create, configure, query,
and observe a map before the first frame.

Sources and layers use style-spec JSON. This representation keeps the API
aligned with the style specification across every layer type. Typed entry points
cover behavior beyond construction, such as source-type validation and per-frame
property updates.

## Render session

A render session renders one map to one render target. A map carries at most one
live render session.

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

The thread that attaches a render session becomes its owner thread for the
session's lifetime. The attaching thread can differ from the map's owner thread.
A host therefore attaches on the thread that owns its graphics context and draws
frames, while another thread pumps the runtime and map. A session call from any
other thread reports an owner-thread status.

### OpenGL context ownership

OpenGL binds a context to a thread, so an OpenGL render target names how the
session and the host divide the thread's context.

A shared session leaves the thread as it found it. Each render makes the
session's context current and restores whatever was current before, and that
context joins the host share group, so the host draws its own graphics on the
same thread and samples session textures from its own context. Texture targets
and WebGL work this way.

A dedicated session owns the thread's context. It creates a context of its own
from the display or device the host already presents through, joins no share
group, and keeps that context current between renders. Choose it for a surface
target on a thread that exists to draw one map, such as an Android host
rendering into a `SurfaceView`. The host then builds no context of its own, and
each frame saves and restores nothing.

## Events

Events preserve MapLibre Native's observer-driven model across the FFI boundary.
The runtime copies events into host-visible storage, and host code drains those
events from the runtime.

Events report map lifecycle, rendering progress, resource activity, diagnostics,
and asynchronous failures.

Rendering observer events reach the runtime queue through the map's run loop. A
pump after the render call makes those events available to drain.

Each map and each runtime carries a subscription: the set of event types it
queues. Default options select every event type the library reports, and a host
narrows a subscription by naming the types it reads. An unselected event is
never built, never queued, and never raises the wake flag that releases a parked
pump.

One drain reports a batch: every queued event in order, plus the message text
that those events carry. Copy any value you keep, because the next drain for
that runtime replaces the batch.

Queued events belong to their source. Destroying a map discards that map's
queued events immediately. Read any state that teardown needs synchronously
while the map is live.

## Failures

Status-returning calls report synchronous failures. Each binding surfaces them
in its own idiom: an exception, a result type, or an error return. Examples
include a call from the wrong thread and an invalid argument.

Events report asynchronous failures, such as a style load, resource request, or
still-image request that failed. Drain events in addition to checking call
results.

## Language bindings

Language bindings preserve the runtime, map, render session, and event model in
the target language. They sit directly above the C API and expose the same
objects and relationships, adding language-appropriate safety around handles,
lifetimes, errors, and event draining.
