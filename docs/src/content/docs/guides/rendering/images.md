---
title: Render images
description: Render still images and tiles, and read their pixels.
# Scope: static and tile modes, still image requests, and reading pixels back.
---

Outline:

- Intro: this guide renders images of any view, or map tiles, and reads their
  pixels. It builds on
  [Getting started](/maplibre-native-ffi/guides/getting-started/), which renders
  one image.

## Create the map

- Create a static map for images of any view. Create a tile map for map tiles:
  it renders one tile, and labels don't cross the tile's edges.
- Make a tile map the size of one tile, such as 512 by 512. For high-density
  tiles, raise the scale factor instead of the size.
- Attach a graphics device texture, as in Getting started.
- Snippet: `map` — map options with the tile mode.

## Render an image

- Set the view for the image: jump the camera there. For a tile, jump to the
  tile's center at the tile's zoom. See
  [Camera](/maplibre-native-ffi/guides/camera/).
- To change the image size, resize the session. It resizes the map too.
- Request a still image, and wait for it. The library renders once the map has
  loaded what the image shows. You don't need to wait for the camera or the
  resize first.
- A map renders one image at a time. Reuse it for the next image after this one
  finishes.
- Snippet: `image` — jump the camera, request a still image, and wait.

## Read the pixels

- Read the session's texture after the image finishes. The result has RGBA
  pixels, row by row, with the stride as the bytes in each row.
- To use the images on the GPU instead, render to a texture; see
  [Render to a texture](/maplibre-native-ffi/guides/rendering/texture/).
- Snippet: `read` — read the texture, and access the pixels and stride.
