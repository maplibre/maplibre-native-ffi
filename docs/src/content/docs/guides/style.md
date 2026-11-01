---
title: Style
description: Load a style, and add and change its sources, layers, and images.
# Scope: loading a style, and adding and changing its sources, layers, and
# images. The style spec itself belongs in its own docs.
---

Outline:

- Intro: this guide loads a style and changes its sources, layers, and images.
  Sources and layers use the JSON of the
  [MapLibre style spec](https://maplibre.org/maplibre-style-spec/), so this
  guide shows the calls and links the spec for the JSON. See
  [Concepts](/maplibre-native-ffi/concepts/#styles) for what a style is.

## Load a style

- Set the style by URL or by JSON.
- Loading a style removes the sources, layers, and images you added. Changes you
  send before the new style finishes loading are lost too. The style-loaded
  event reports when it has.
  [Events and logs](/maplibre-native-ffi/guides/events-and-logs/) covers
  receiving events.
- Snippet: `load` — set the style URL.

## Add a source and a layer

- Add a source with its ID and the JSON object that a style puts under
  `sources`. Link the spec's sources page.
- For GeoJSON data in memory, prepare it on any thread, then add a GeoJSON
  source with the prepared data. To update the source, prepare new data with the
  same options and set it.
- Add a layer with its JSON object. Give it the ID of another layer to draw
  under that layer, or no ID to draw on top. Link the spec's layers page.
- To remove them, remove the layer first, then the source. A source that a layer
  still uses can't be removed.
- Snippet: `add` — add a raster tile source and a raster layer.
- Snippet: `geojson` — prepare GeoJSON, add a source with it, and update it.

## Change a layer

- Set a layer property by its style-spec name and a JSON value. The same call
  sets `visibility` and `filter`. A JSON null restores the default.
- A value can be an expression. Link the spec's expressions page.
- Each change can show in its own frame. To show several changes in the same
  frame, send them between the start and end of a command group.
- Snippet: `set-property` — hide a layer, then set `line-color`.

## Add an image

- Set a style image with an ID and premultiplied RGBA pixels. A symbol layer
  draws it by that ID in `icon-image`.
- When a layer asks for an image that the style lacks, the map sends a
  missing-image event with the image's ID. Set the image when the event arrives.
- Snippet: `image` — set a style image and add a symbol layer that uses it.
- Snippet: `missing-image` — on the missing-image event, set an image with the
  event's ID.
