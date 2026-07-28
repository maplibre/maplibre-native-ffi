---
title: Concepts
description: Core mental models for using MapLibre Native FFI.
---

## Mental Model

MapLibre Native FFI exposes MapLibre Native concepts directly. Applications can
use it directly or through language bindings, and higher-level adapters can
build on the same model. It provides a common portable API surface for native
map integration.

Three concepts form the core API: the runtime, the map, and the render session.
Events and bindings connect those concepts to host code.

## Runtime

The runtime owns scheduler state and event storage for one host owner thread.
Host code creates a runtime on the thread that will pump it. Runtime work and
events flow through that owner thread.

Each owner thread may have one live runtime. The host pumps that runtime to let
MapLibre Native make progress and to collect completed work.

A host paces the pump itself. Display-paced hosts pump once per frame. A host
that owns its pump thread parks that thread until the runtime has work, and
wakes it from its own threads through a wake source.

## Map

A map belongs to a runtime. It owns map state: style documents, sources, layers,
images, camera state, feature state, observer events, and render invalidation.

A map is independent of any particular render target. Host code can create,
configure, query, and observe the map without tying that map state to a window,
surface, or texture.

Sources and layers are added as style-spec JSON, which keeps the API in step
with the style specification across every layer type. Typed entry points exist
where a layer needs a surface beyond construction, such as source-type
validation or typed per-frame setters.

## Render Session

A render session renders one map to one render target. Render targets are
surfaces or textures.

Surface sessions render and present through caller-provided native surfaces.
Texture sessions render offscreen into session-owned backend targets or
caller-owned borrowed backend targets.

A map may have one live render session at a time. Keeping render sessions
separate from maps lets host code manage graphics backend lifecycle outside the
map object itself.

## Events

Events preserve MapLibre Native's observer-driven model across the FFI boundary.
The runtime copies events into host-visible storage, and host code drains those
events from the runtime.

Events report map lifecycle, rendering progress, resource activity, diagnostics,
and asynchronous failures.

Queued events belong to their source. Destroying a map discards that map's
queued events without a flush or a terminal event, so host state mirrored from
events is only as current as the last drain before teardown. Snapshot the state
a host needs synchronously while the map is live, and let teardown run to
completion rather than waiting on an event from the map being destroyed.

## Language Bindings

Language bindings preserve the same runtime, map, render session, and event
model in the target language. They keep the API portable while matching the
target language's handle and error conventions.

Bindings sit directly above the C API and stay close to its shape. They expose
the same core objects and relationships with language-appropriate safety around
handles, lifetimes, errors, and event draining.
