---
title: Choose a render target
description: Pick a map mode, a render target, and a driver.
# Scope: how to pick a map mode, render target, driver, and frame timing for a
# use case, and the graphics backends. How to use each target belongs on its
# own page.
---

Every integration picks a map mode, a render target, and a driver. This page
picks them for a use case, then links to the page that does the work.

## Find your use case

| Use case                                 | Map mode       | Target                    | When to render                       | Page                                                                  |
| ---------------------------------------- | -------------- | ------------------------- | ------------------------------------ | --------------------------------------------------------------------- |
| Your own window or view                  | Continuous     | Surface                   | A frame loop                         | [Render to a window](/maplibre-native-ffi/guides/rendering/window/)   |
| A map inside another renderer            | Continuous     | Owned or borrowed texture | A frame loop at that renderer's rate | [Render to a texture](/maplibre-native-ffi/guides/rendering/texture/) |
| Map images or tiles, such as on a server | Static or tile | Graphics device texture   | One image per request                | [Render images](/maplibre-native-ffi/guides/rendering/images/)        |

Another renderer can be a UI framework, a game engine, or a compositor. For a
use case that the table lacks, the sections below explain each choice.

## Pick a map mode

Set the mode when you create the map. You can't change it later.

Use continuous mode for an interactive map. It updates as data loads and the
camera moves. You request frames in a loop, usually once per display refresh.

Use static mode for images of any view, and tile mode for map tiles. You request
one still image at a time.

## Pick a render target

A surface presents to a window or view that you own.

A texture holds frames for your own renderer to draw. The session owns its
textures, or borrows textures that you create.

Both need your graphics objects, such as a device or a context. If you only need
pixels and have no graphics code, attach a graphics device texture instead. The
library creates the device and owns the texture, and you read its pixels.

The library builds with one graphics backend, chosen when you install it; see
[Install](/maplibre-native-ffi/install/#choose-a-backend). Surfaces and textures
have an attach function for each backend. Use the one for your build's backend.

## Pick a driver

The driver is where the session's graphics work runs: on a library thread, or on
your graphics thread when the library asks.

Choose the library thread unless your target requires your graphics thread.
Targets that use your OpenGL context require it, and so do WebGPU targets in a
browser. A graphics device texture always uses the library thread.

To run that work on your graphics thread, see
[Render to a window](/maplibre-native-ffi/guides/rendering/window/#render-on-your-graphics-thread).
