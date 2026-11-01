---
title: Query
description: Find the features on the map or in a source.
# Scope: querying rendered features and source features.
---

Outline:

- Intro: find the features that the map draws, or the features that a source has
  loaded.
- Queries go through the render session, so the map needs an attached one. A
  rendered query reads the last frame. Render after a camera or style change
  before you query.
- With a driver on your graphics thread, keep servicing driver work while you
  wait, or the query doesn't finish. See
  [Render to a texture](/maplibre-native-ffi/guides/rendering/texture/).

## Query rendered features

- Query rendered features at a screen point or in a box.
  [Camera](/maplibre-native-ffi/guides/camera/#convert-between-pixels-and-coordinates)
  covers screen points.
- Name the layers to query. Without them, the query covers every layer.
- Each result is a GeoJSON feature with the layer that drew it. A feature that
  several layers draw appears once per layer.
- To keep only features with certain properties, pass a filter.
- Snippet: `rendered` — query a box around a point, in one layer, and read each
  feature's JSON.

## Query source features

- Query a source's features by its ID, whether or not a layer draws them. For a
  vector source, name the source layers.
- The result has only the features in tiles the source loaded for the current
  view. A source loads tiles only while a visible layer uses it.
- Snippet: `source` — query a vector source's features in one source layer, with
  a filter.
