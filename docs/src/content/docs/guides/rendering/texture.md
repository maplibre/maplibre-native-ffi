---
title: Render to a texture
description: Share rendered frames with your own renderer.
# Scope: owned and borrowed textures, acquiring frames, and GPU sync. Reading
# pixels back belongs in Render images. Running driver work on your graphics
# thread belongs in Render to a window.
---

Outline:

- Intro: this guide renders a continuous map into textures that your own
  renderer draws, such as in a UI framework, a game engine, or a compositor. It
  assumes the frame loop from
  [Render to a window](/maplibre-native-ffi/guides/rendering/window/#render-frames).

## Attach a texture ring

- The session renders into a ring of textures, so your renderer can draw one
  frame while the session renders the next.
- Let the session create the ring on your graphics device, unless your framework
  hands you the textures to render into. Then attach those textures as a
  borrowed ring.
- Pass your graphics context, such as your device and queue. On Vulkan, if your
  renderer also submits to that queue, pass a lock that you take around your own
  calls on it.
- Pass the GPU sync kinds that your renderer can wait on. The session picks one,
  and the session's capabilities report which.
- Snippet: `attach-owned` — an owned texture descriptor on the host's device,
  the attach options with sync kinds, and the attach call.
- Snippet: `attach-borrowed` — a borrowed descriptor with the host's textures,
  and the attach call.

## Draw each frame

- Render frames with the same loop as a window, without presenting.
- When a frame renders, acquire it, and get its texture for your backend.
- Wait on the frame's producer sync before your renderer reads the texture.
- Release the frame after your reads. Pass a consumer sync that signals when
  they finish, or none if they finished already. The session renders into that
  texture again only after it signals.
- Keep at most one frame acquired while the next renders. If every texture in
  the ring is acquired, the next frame waits.
- If your renderer already runs a loop every frame, as a game engine does, you
  can drain the frame results there instead of using the frame wake.
- Snippet: `acquire` — acquire, get the texture, wait on the producer sync,
  draw, and release with a consumer sync.

## Resize the textures

- Release every acquired frame first.
- For an owned ring, resize the session, as for a window.
- For a borrowed ring, create textures at the new size, and set them as the
  session's target. Destroy the old textures after that finishes.
- Snippet: `resize-borrowed` — new textures, set target, then destroy the old
  ones.

## Close the session

- Release every acquired frame. Then detach and dispose the session, as for a
  window.
- Destroy a borrowed ring's textures after the dispose finishes.
