---
title: Concepts
description: How runtimes, maps, and render sessions fit into a host application.
---

MapLibre Native FFI provides map state and rendering through a C API and
language bindings. The host supplies its UI, graphics resources, and frame
cadence. Three objects form the core API: the runtime, the map, and the render
session.

## Runtime

A runtime owns a native scheduler thread, an event queue, and shared resource
services such as networking and the cache database. One runtime can serve
multiple maps. Host threads submit work, and the scheduler applies it in order.

Callbacks may run on native threads or on the submitting thread. Host code
schedules UI work on its own event loop. A wake callback announces work that is
ready; it must return promptly without blocking or waiting for the receiver.
Each callback's API reference states its threading and reentry rules.

The [runtime reference](/maplibre-native-ffi/reference/c/runtime_8h.html) covers
runtime options and shutdown. The
[creation guide](/maplibre-native-ffi/guides/create-a-map/) shows the first
steps.

## Map

A map belongs to a runtime and holds the style, sources, layers, camera, and
feature state. It exists independently of a render target, so the host can
configure it before attaching graphics resources.

A style describes what to draw. Sources supply data, and layers choose its
appearance and drawing order. Sources and layers use MapLibre style-spec JSON.
Loading another style replaces the style's contents; host additions must be
applied again.

Map mutations are asynchronous commands. Acceptance means that native code
copied and queued the input. A completion reports the command's outcome; the
host observes that outcome even when it continues without waiting. Cancelling a
binding's wait ends the wait, while native work continues.

Published snapshots give UI and display threads a synchronous copy of map state.
Their generations connect commands, events, and rendered frames when a host
needs to know which state it is reading. A command group holds render updates
until the group ends, allowing related changes to appear together in a frame.

The [map reference](/maplibre-native-ffi/reference/c/map_8h.html) covers
commands, snapshots, and groups.
[Load a style](/maplibre-native-ffi/guides/load-a-style/) introduces the source
and layer workflow.

## Render session

A render session draws one map into one render target. A map has at most one
live session. Keeping the session separate lets the host replace graphics
resources while retaining map state.

A native surface presents to a window, view, or canvas. An owned texture target
uses textures allocated by the session for offscreen rendering. A borrowed
texture target uses textures allocated by the host for compositing into its own
scene.

The host also chooses where graphics work runs. A core-worker driver uses a
native graphics thread. A caller-graphics-thread driver requires the host to
service queued work on its graphics thread, including while display callbacks
are paused. The backend and target determine which choice is available.

The host requests frames at its own cadence and drains frame results when they
are ready. A rendered result refers to the map state that the session drew;
resource loads and animated camera changes may require later frames. Texture
frames require producer synchronization before host reads and consumer
synchronization before the session reuses them.

The host keeps graphics resources alive through attachment and teardown. Normal
shutdown detaches the session before destroying those resources. Abandon handles
a target that can no longer be serviced and may retain graphics resources until
process exit.

[Attach a render target](/maplibre-native-ffi/guides/attach-a-render-target/)
covers target and driver choices.
[Run the render loop](/maplibre-native-ffi/guides/run-the-render-loop/) connects
frame demand to host scheduling. The
[session reference](/maplibre-native-ffi/reference/c/render__session_8h.html)
states synchronization and teardown contracts.

## Events

The runtime queues events from its maps and resource services. Events report
style loads, render updates, download progress, and resource failures. A
subscription selects which event types enter the queue, and a wake tells the
host when to drain it.

A drain transfers queued events into an owned batch. The host routes events by
source ID, copies any values that it keeps, and releases the batch. Events
report changes; completions report individual submissions, and frame results
report individual frame demands.

[Handle events](/maplibre-native-ffi/guides/handle-events/) shows the drain
path. The [runtime reference](/maplibre-native-ffi/reference/c/runtime_8h.html)
describes event payloads and generation ordering.

## Failures

Immediate calls report validation or inspection failures. A submission can fail
at acceptance or later in its completion. Resource failures also appear as
events. Bindings expose these channels through their language's error and
asynchronous result types.

The host handles both acceptance and completion failures. Diagnostic text
explains the failure; the
[completion reference](/maplibre-native-ffi/reference/c/completion_8h.html)
defines command outcomes and borrowed completion results.

## Layer plugins

Native plugins add style layer types, with properties and shaders that styles
use alongside built-in layers. Plugin code runs on MapLibre's threads and stays
loaded for the process lifetime. Plugin authoring uses the native contract;
language bindings expose registration for host integrations.

The [plugin reference](/maplibre-native-ffi/reference/c/plugin_8h.html) covers
registration and backend support.

## Language bindings

Bindings preserve the runtime, map, render session, and event model. They add
language-appropriate safety around handles, borrowed values, callbacks, and
errors.

The host closes owned handles explicitly using its language's cleanup idiom and
keeps borrowed values only for their documented scope. Garbage-collected
bindings report handles reclaimed while still open as leaks. Automatic cleanup
is a fallback; explicit teardown lets the host release graphics resources in
order.

[Install](/maplibre-native-ffi/install/) covers packages for each language. The
API references describe their generated shapes, and the
[examples](https://github.com/maplibre/maplibre-native-ffi/tree/main/examples)
show complete integrations.
