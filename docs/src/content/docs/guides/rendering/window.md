---
title: Render to a window
description: Present a map to a window or view.
# Scope: surfaces, the frame loop, resizing, losing and restoring the surface,
# such as when an app goes to the background, and running driver work on your
# graphics thread.
---

Outline:

- Intro: this guide presents a continuous map to a window or view that your
  application creates, and keeps it updated as the map changes. It assumes a
  runtime and a continuous map from
  [Getting started](/maplibre-native-ffi/guides/getting-started/).

## Attach a surface

- Create the window with your platform or windowing library, and get its surface
  handle for your backend. Link the upstream docs for each: `CAMetalLayer` for
  Metal, `VkSurfaceKHR` for Vulkan, an EGL window surface for OpenGL, and a
  canvas for WebGPU. The snippet starts from that handle.
- Attach the surface to the map at the window's size and scale factor. Pass a
  frame wake; the next section uses it. Wait for the attach to finish.
- Use the library thread as the driver unless your backend needs your graphics
  thread; see
  [Choose a render target](/maplibre-native-ffi/guides/rendering/choose-a-target/#pick-a-driver).
- Snippet: `attach` — the surface descriptor, attach options with the driver and
  a frame wake, and the attach call.

## Render frames

- Ask for one frame at a time. Ask the frame to present, and to wait for the
  map's next update, so an idle map renders nothing.
- The frame wake runs on a library thread. Have it schedule a drain of the frame
  results on your event loop.
- When the result arrives, ask for the next frame at the next display refresh.
  This renders at most once per refresh, and shows every change made in between.
- When the target isn't ready, ask again after the next refresh. When a frame
  fails, dispose the session and attach a new one.
- Snippet: `frame-wake` — the wake callback, which schedules a drain.
- Snippet: `frame-loop` — the drain, the check of each result, and the next
  frame request.

## Resize the window

- When the window's size changes, resize the session to the new width and
  height. The session resizes the map too.
- Keep asking for frames meanwhile. Frames render at the new size once the map
  has applied it.
- If the platform gives you a new surface for the new size, replace the
  session's surface instead.
- Snippet: `resize` — the session resize call from a window resize handler.

## Handle a lost surface

- When the platform takes the surface away, such as when an app goes to the
  background, stop asking for frames. Release the session's target, and wait for
  the release before the surface is destroyed.
- The session keeps its tiles and other renderer state, so the map shows again
  quickly.
- When the platform gives you a surface again, set it as the session's target,
  then ask for a frame.
- Snippet: `lost-surface` — the target release, and setting the new surface.

## Render on your graphics thread

- With the graphics-thread driver, the session's graphics work runs only when
  you service it on your graphics thread, with the target's context current.
- Pass a driver work wake when you attach. When it fires, schedule a service of
  the session on your graphics thread.
- Service whenever the wake fires, even while you ask for no frames. Attaching,
  detaching, and releasing the target finish only through service.
- Snippet: `service` — the driver work wake, and the service call on the
  graphics thread.

## Close the session

- Stop asking for frames. Detach the session, wait for the detach, then dispose
  it. With the graphics-thread driver, keep servicing until the detach finishes.
- Destroy the surface after the dispose finishes. Then close the map and the
  runtime, as in
  [Getting started](/maplibre-native-ffi/guides/getting-started/#close).
- Snippet: `close` — detach, dispose, and destroying the surface.
