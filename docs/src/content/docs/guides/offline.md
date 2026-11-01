---
title: Offline
description:
  Set a cache file, manage the ambient cache, and download offline regions.
# Scope: the cache file, the ambient cache, and offline regions.
---

Outline:

- Intro: the library keeps resources in a cache, so maps load faster and work
  without a network. The ambient cache keeps what maps have loaded recently. An
  offline region keeps a whole area that you download on purpose.

## Set a cache file

- Set a cache file path when you create the runtime. Without one, the cache
  lives in memory, and offline regions are lost when the runtime closes.
- Snippet: `cache-file` — runtime options with a cache path.

## Manage the ambient cache

- Set the ambient cache's maximum size to bound its disk use. The library
  removes old resources to fit. Offline regions don't count toward it.
- Clear the ambient cache to free its space. Invalidate it to keep the data but
  check each resource with the server before its next use, such as after your
  tiles change.
- Snippet: `ambient` — set the maximum size, and clear the cache.

## Download a region

- Define the region: the style URL, the bounds, the zoom range, and the pixel
  ratio. Use a GeoJSON geometry instead of bounds to follow a shape. Each zoom
  level has four times the tiles of the one before.
- Store your own bytes with the region as metadata.
- Create the region, then start its download.
- Maps on the same runtime use the region's resources when they load the same
  style URL. They need no other setup.
- Snippet: `download` — define and create a region, then set its download
  active.

## Track download progress

- Observe the region to receive its progress as events (link Events and logs).
- Each status event counts the completed and required resources, and says
  whether the download is complete. An error event names a resource that failed.
- Snippet: `progress` — observe the region, and read status events in the event
  handler.

## Update and delete regions

- List the regions to find those from earlier launches. A download stops when
  the runtime closes. Start it again to resume.
- To update a region's data, invalidate the region, then start its download
  again.
- Delete a region you no longer need. Its resources that no other region uses
  move to the ambient cache.
- Snippet: `manage` — list regions, invalidate and restart one, delete one.
