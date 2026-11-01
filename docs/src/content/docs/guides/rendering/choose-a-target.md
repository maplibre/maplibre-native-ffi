---
title: Choose a render target
description: Pick a map mode, a render target, and when to render.
# Scope: how to pick a map mode, render target, driver, and frame timing for a
# use case, and the graphics backends. How to use each target belongs on its
# own page.
---

Outline:

- Intro: every integration makes three choices: the map mode, the render target,
  and when to render. This page picks them for a use case, then links to the
  page that does the work.
- No snippets: each choice is one option or one function, which the linked page
  shows.

## Find your use case

- Table, one row per use case, columns: map mode, target, when to render, page.
  - Your own window or view: continuous, surface, a frame loop →
    [Render to a window](/maplibre-native-ffi/guides/rendering/window/).
  - A map inside another renderer's scene, such as a UI framework, a game
    engine, or a compositor: continuous, owned or borrowed texture, a frame loop
    at that renderer's frame rate →
    [Render to a texture](/maplibre-native-ffi/guides/rendering/texture/).
  - Map images or tiles, such as on a server: static or tile, graphics device
    texture, one image per request →
    [Render images](/maplibre-native-ffi/guides/rendering/images/).
- The sections below explain each column, for a use case that the table lacks.

## Pick a map mode

- Set the mode when you create the map. You can't change it later.
- Continuous for an interactive map. It updates as data loads and the camera
  moves. You ask for frames in a loop, usually once per display refresh.
- Static for images of any view. Tile for map tiles: it renders one tile, and
  labels don't cross the tile's edges. You request one still image at a time,
  and the library renders when the map has loaded what the image shows.

## Pick a render target

- A surface presents to a window or view that you own.
- A texture holds frames for your own renderer to draw, or for you to read back.
  The session owns its textures, or borrows textures that you create, such as
  ones your UI framework gives you.
- Both need your graphics objects, such as a device or a context. If you only
  need pixels and have no graphics code, attach a graphics device texture
  instead. The library creates the device and owns the texture, and you read it
  back.
- The library builds with one graphics backend, chosen when you install it; see
  [Install](/maplibre-native-ffi/install/#choose-a-backend). Each target has an
  attach function for each backend. Use the one for your build's backend.

## Pick a driver

- The driver is where the session's graphics work runs: on a library thread, or
  on your graphics thread when the library asks.
- Choose the library thread unless your target requires your graphics thread.
  OpenGL surfaces and shared OpenGL textures require it, and so does WebGPU in a
  browser. A graphics device texture has no graphics thread of yours, so it uses
  the library thread.
- For how to run that work on your graphics thread, see
  [Render to a window](/maplibre-native-ffi/guides/rendering/window/#render-on-your-graphics-thread).
