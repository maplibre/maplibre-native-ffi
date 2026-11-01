---
title: Camera
description:
  Move, animate, and fit the camera, apply gestures, and convert between pixels
  and coordinates.
# Scope: reading and changing the camera, animations, fitting bounds, limits,
# applying gestures, and converting between screen pixels and coordinates.
---

Outline:

- Intro: change the map's camera, from your code and from gestures. A camera has
  a center, zoom, bearing, and pitch.

## Read the camera

- Read the camera from the map's snapshot. It returns right away, but it can lag
  behind changes you just sent.
- To read the camera after your own changes have applied, query the camera
  instead, and wait for the result.
  [Concepts](/maplibre-native-ffi/concepts/#changes-are-asynchronous) explains
  why.
- Snippet: `read` — read the snapshot's camera, and query the camera.

## Move the camera

- Send a camera update in jump mode. Set only the fields you want to change. The
  others keep their values.
- Snippet: `jump` — jump to a center and zoom.

## Animate the camera

- Send a camera update in ease or fly mode, with a duration.
- Ease moves straight to the target. Fly zooms out, travels, and zooms back in.
- The update's completion reports that the animation started. To act when it
  ends, set an end handler. The handler reports whether the animation finished
  or was cancelled.
- A new camera change replaces the animation of the fields it sets. To stop an
  animation where it is, cancel the map's camera transitions.
- Snippet: `animate` — fly to a center and zoom, with an end handler.

## Fit the camera to an area

- Query the camera that fits a bounding box. Then jump or animate to that
  camera.
- To fit a list of coordinates or a GeoJSON geometry instead, use the matching
  query.
- Snippet: `fit` — query the camera for bounds, then ease to it.

## Limit where the camera can go

- Set the map's bounds. They keep the center inside an area, and the zoom and
  pitch inside ranges. Limits you leave out keep their current values.
- The limits apply to gestures and to camera changes from your code.
- Snippet: `limit` — set bounds and a zoom range.

## Apply gestures

- Your code recognizes the gesture from its input events. Screen points are in
  logical pixels from the map's top-left corner.
- When a gesture starts, cancel the camera's transitions, then send a camera
  delta with the begin phase.
- For each movement, send a delta with the update phase. Pass the change since
  the last delta: a drag's movement as the offset, a pinch's scale with its
  centroid as the anchor.
- When the gesture ends, send the end phase, or the cancel phase if the platform
  cancels it.
- Snippet: `gesture` — begin, a drag and a pinch update, end.

## Convert between pixels and coordinates

- Create a projection from the snapshot's view. Convert screen points to
  coordinates, and coordinates to screen points.
- A projection converts for the view it was created from. Create a new one after
  the camera moves.
- Snippet: `convert` — create a projection, convert both ways, close it.
