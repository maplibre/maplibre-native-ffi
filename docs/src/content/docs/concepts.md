---
title: Concepts
description: Core mental models for using MapLibre Native FFI.
---

MapLibre Native FFI exposes MapLibre Native concepts directly. Applications use
it through a language binding or call the C API themselves, and higher-level
adapters build on the same model.

Three objects form the core API: the runtime, the map, and the render session.
Events and bindings connect those objects to host code.

## Runtime

The runtime owns scheduler state and event storage for one host owner thread.
Host code creates a runtime on the thread that will pump it, and runtime work
and events flow through that thread.

Each owner thread carries at most one live runtime. MapLibre Native makes
progress when the host pumps that runtime, which also collects completed work.

The host sets the pace. A display-paced host pumps once per frame. A host that
owns its pump thread parks that thread until the runtime has work, and wakes it
from its own threads through a wake source.

## Map

A map belongs to a runtime. It owns style documents, sources, layers, images,
camera state, feature state, observer events, and render invalidation.

A map is independent of any particular render target. Host code creates,
configures, queries, and observes a map that has yet to draw a frame.

Sources and layers arrive as style-spec JSON. The API therefore stays in step
with the style specification across every layer type. Typed entry points exist
where a layer needs a surface beyond construction, such as source-type
validation or typed per-frame setters.

## Render session

A render session renders one map to one render target. A map carries at most one
live render session.

Render targets come in three kinds:

| Render target           | Owned by | Renders                              |
| ----------------------- | -------- | ------------------------------------ |
| native surface          | caller   | To a window or view, and presents    |
| owned texture target    | session  | Offscreen, into a session allocation |
| borrowed texture target | caller   | Offscreen, into a caller allocation  |

Keeping render sessions separate from maps lets host code manage graphics
backend lifecycle outside the map object.

A render session records its own owner thread, which is the thread that attached
it, and that thread stays fixed for the session's lifetime. Attaching requires
only a live map, so the attaching thread can differ from the map's owner thread.
A host therefore attaches on the thread that owns its graphics context and draws
the frames, while it pumps the runtime and map on another thread. A session call
from any other thread reports the owner-thread status.

## Events

Events preserve MapLibre Native's observer-driven model across the FFI boundary.
The runtime copies events into host-visible storage, and host code drains those
events from the runtime.

Events report map lifecycle, rendering progress, resource activity, diagnostics,
and asynchronous failures.

Rendering observer events reach the runtime queue through the map's run loop, so
a later pump drains the events that a frame produced rather than the render call
that produced them.

Queued events belong to their source. Destroying a map discards that map's
queued events immediately, so host state mirrored from events stays only as
current as the last drain before teardown. A host that needs such state at
teardown reads it synchronously while the map is live.

## Failures

Every call reports whether it succeeded, and each binding surfaces that in its
own idiom: an exception, a result type, or an error return. Failures that
MapLibre detects at the call, such as a wrong thread or an argument it cannot
use, arrive that way.

A failure that happens later has no call to return to and arrives as an event
instead: a style that never loaded, a resource that could not be fetched, a
still image that failed. A host that checks only return values sees those as
silence.

## Language bindings

Language bindings preserve the runtime, map, render session, and event model in
the target language. They sit directly above the C API and expose the same
objects and relationships, adding language-appropriate safety around handles,
lifetimes, errors, and event draining.
