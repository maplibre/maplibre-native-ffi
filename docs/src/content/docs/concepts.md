---
title: Concepts
description: The parts of MapLibre Native FFI that every integration uses.
# Scope: the few ideas every integration needs, in terms every binding shares. No
# feature tours, no C-specific details, no task steps, and no contracts; those
# belong in guides and the API reference.
---

MapLibre Native FFI is the MapLibre Native map engine without a user interface.
It loads styles and data, keeps the state of each map, and renders it. Your
application provides everything else, such as the window and input of an
interactive map, or the request handling of a tile server.
[Projects](/maplibre-native-ffi/#projects) lists some that use it.

## C API and bindings

Use the library from C, or through the binding for your language: Dart, Go,
Kotlin, .NET, Python, Rust, Swift, TypeScript, or Zig. Every binding has the
same objects and functions as the C API, in its language's own types, errors,
asynchronous results, and cleanup. Its API reference shows them.

## Runtime, map, and render session

A **runtime** runs the library's background work: a thread that applies changes
to maps, plus networking and the tile cache. Most applications create one
runtime and share it among their maps.

A **map** holds one map's style, camera, and data. It draws nothing on its own,
so you can create and set up a map before any graphics exist.

A **render session** draws one map into one target: either a surface that
presents to a window or view, or a texture that your application composites or
reads. Closing a session doesn't change the map. You can attach the map to a new
target later, such as when your application returns from the background.

## Styles

A style describes what a map shows. Styles follow the
[MapLibre style spec](https://maplibre.org/maplibre-style-spec/): sources supply
data, and layers draw that data in order. You can load a whole style, or add and
change sources and layers in the loaded one. Loading another style replaces
everything, including what you added.

## Changes are asynchronous

A change to a map, such as moving the camera or adding a layer, is a request.
The library accepts it right away, applies it later in the order you sent it,
and reports the result. You can wait for the result or ignore it.

Reading the camera and other basic state doesn't wait: it comes from a snapshot
of the map that any thread can read. The library reports what you didn't ask
for, such as a style finishing loading or a tile failing to download, as events.

## Threads and callbacks

The library calls your code to deliver results and events, and to ask you for
things, such as a resource that your application serves. It either calls your
code on one of its own threads, or queues the call for your application to run
on a thread it chooses. Code on a library thread must return quickly and hand UI
work to your event loop. A binding for a language that can't run your code on
the library's threads queues every call. Where a callback has to answer right
away, such as one that rewrites HTTP request headers, that binding takes fixed
rules instead.

## Rendering

You decide when to render. Ask the render session for a frame, usually once per
display refresh. It renders if the map has changed, and reports the result.
Depending on the target and graphics backend, the library runs graphics work on
its own thread, or your application runs it on its graphics thread when the
library asks. Keep your graphics objects, such as the window surface or the
device, alive until the session closes.

## Closing

Close what you create, in reverse order: render sessions, then maps, then the
runtime. Closing can be asynchronous, and the resources are freed when it
finishes. In a garbage-collected language, close them yourself; if the collector
finds one still open, the binding reports a leak.

## Errors

A call can fail in two ways. The library rejects it right away when an argument
or the object's state is wrong, or it accepts the call and reports a failure in
the result. Each binding uses its language's own errors and asynchronous results
for these. Failures not tied to a call, such as a tile that can't load, arrive
as events. Every failure has a message that describes it.
