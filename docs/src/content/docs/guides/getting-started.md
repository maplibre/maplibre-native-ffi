---
title: Getting started
description: Render a map and read its pixels.
sidebar:
  order: 1
# Scope: one short program, start to finish, that renders a map and reads its
# pixels, without a window. Each step names the choice it makes and links the
# page that covers the alternatives.
---

Outline:

- Intro: this program renders a map of the world and reads its pixels. It needs
  no window, so it works on every platform.
  [Rendering](/maplibre-native-ffi/guides/rendering/) covers windows and other
  targets.

## Create a runtime

- Create one runtime with default options.

## Create a map

- Create a map in static mode, 512 × 512 at scale 1.
- Static mode renders one image per request. Rendering covers the other modes.

## Load a style

- Set the style URL to MapLibre's demo tiles style.
- Don't wait for it to load; the image waits for what it shows.

## Attach a texture

- Create a graphics device for the library's backend, so the program needs no
  graphics code of its own.
- Attach a texture from that device to the map, the same size as the map. This
  returns a render session.

## Render an image

- Request a still image, and wait for it.

## Read the pixels

- Read the texture's pixels back as RGBA. What the program does with them is up
  to it.

## Close

- Close the render session, the graphics device, the map, and the runtime, in
  that order.
- Wait for each to finish closing where the language makes closing asynchronous.

## Next steps

- [Rendering](/maplibre-native-ffi/guides/rendering/): pick the map mode,
  target, and frame timing for your use case.
- The feature pages: Camera, Style, Data, Query, Events and logs, Network.
