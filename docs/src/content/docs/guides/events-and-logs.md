---
title: Events and logs
description: Receive map events and library logs.
# Scope: choosing which events to receive, handling them, and capturing logs.
---

Outline:

- Intro: this guide receives map events and the library's logs, and runs your
  code for them on your application's thread. For what events are, link to
  [Concepts](/maplibre-native-ffi/concepts/#changes-are-asynchronous).

## Set an event handler

- Set one event handler on the runtime. It receives the events of every map on
  that runtime.
- Set it before you create a map, so it receives the map's first events.
- Each event names its type and the map that sent it. Compare that map with the
  maps you hold.
- Snippet: `handler` — set the handler, and switch on each event's type.

## Choose the events a map sends

- A map sends every event by default. Set an event mask in the map options to
  send only the events you act on. You can change the mask later.
- Events report style and resource loading, rendering, and camera changes. The
  API reference lists them.
- Snippet: `mask` — map options with an event mask.

## Handle events on your application's thread

- The handler runs on a library thread, where it must return quickly (link
  [Concepts](/maplibre-native-ffi/concepts/#threads-and-callbacks)).
- To run it on your thread instead, create a delivery queue whose wake posts a
  task to your event loop. Name the queue in the handler. The task dispatches
  the queue, which runs the handler.
- A binding that queues every call already does this.
- Snippet: `queue` — create the queue with a wake, set the handler with it, and
  dispatch from the event loop.

## Capture logs

- Set a log callback to send the library's logs to your own logger. It covers
  every runtime in the process.
- Return true to consume a record, or false to let the platform logger print it
  too.
- The callback can't call the library. Name a delivery queue, as for events, to
  run it on your thread.
- Snippet: `logs` — set a log callback that forwards severity and message.
