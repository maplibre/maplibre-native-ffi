---
title: Network
description:
  Change requests, serve resources and tiles from your code, and set the network
  status.
# Scope: transforming and serving requests, custom sources whose tiles your code
# supplies, and the network status.
---

Outline:

- Intro: the library downloads styles, tiles, fonts, and icons from the URLs in
  the style. This guide changes those requests, serves resources from your own
  code, and tells the library when the device is offline. Each setting applies
  to every map on the runtime.

## Add headers

- Add headers, such as an `Authorization` header, with a header transform.
- For fixed values, give it rules: each rule matches a URL pattern, such as
  every URL on your tile server, and lists its headers. Match only your own
  server, so your token doesn't go to other servers in the style.
- For values that change, such as a token that expires, give it a callback
  instead. It sets headers for each request, and must return right away.
- The browser and OpenHarmony can't add headers. There, serve those requests
  with a resource provider instead (see below).
- Snippet: `headers` — a header transform with one rule for one host.

## Rewrite URLs

- Change a request's URL with a resource transform, for example to add an API
  key or to send requests to another server.
- The callback answers with the new URL, or keeps the old one. It can answer
  later, from any thread, such as after signing the URL.
- Snippet: `rewrite` — a transform that adds a query parameter.

## Serve resources from your code

- First check whether you need code. The library reads `file://` URLs, bundled
  files through `asset://` URLs and the runtime's asset path, and tile archives
  through `pmtiles://` and `mbtiles://` URLs. Point the style at those.
- For anything else, set a resource provider. Give your URLs a custom scheme in
  the style, such as `app://tiles/{z}/{x}/{y}.pbf`, and route that scheme to the
  provider. Other requests skip it.
- Answer each request with its bytes, or with no content for a tile that has no
  data. Answer later, from any thread, if loading takes time. Pass a request
  through to let the library download it.
- Snippet: `provider` — a provider with one route that answers from a lookup.

## Supply tiles from your code

- For tiles that have no URL, such as data your application computes for an
  area, add a custom source. It asks your code for each tile it needs, by zoom,
  x, and y.
- A custom geometry source takes GeoJSON for each tile. A custom vector source
  takes encoded vector tiles.
- Answer each request with the tile's data, or with an error. Answer later, from
  any thread, if it takes time. When your data for a tile changes, invalidate
  the tile, and the map asks for it again.
- Snippet: `custom-source` — add a custom geometry source whose callback answers
  each tile with GeoJSON.

## Set the network status

- When the device loses its connection, set the network status to offline. The
  library stops starting requests and loads from its cache (link Offline). Set
  it online when the connection returns.
- The status covers every runtime in the process.
- Snippet: `status` — set offline, then online.
