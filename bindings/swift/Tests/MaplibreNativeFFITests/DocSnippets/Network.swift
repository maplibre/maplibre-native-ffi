// Snippets for the docs site's Network page. They compile with the tests,
// which never run them.

import Foundation
import MaplibreNativeFFI

private func withApiKey(_ url: String) -> String {
  url + (url.contains("?") ? "&" : "?") + "key=my-api-key"
}

private func findTile(_: String) -> Data? {
  nil
}

private func geojsonForTile(_: CanonicalTileId) -> String {
  #"{"type": "FeatureCollection", "features": []}"#
}

private func network(runtime: RuntimeHandle, map: MapHandle) throws {
  // #region headers
  try runtime.discarding.setHttpHeaderTransform(HttpHeaderTransform(rules: [
    HttpHeaderRule(
      flags: .glob,
      url: "https://tiles.example.com/**",
      headers: [HttpHeader(name: "Authorization", value: "Bearer my-token")]
    ),
  ]))
  // #endregion headers

  // #region rewrite
  let transform = ResourceTransform { _, url, request in
    do {
      try request.complete(url: withApiKey(url))
    } catch {
      // Releasing the request without an answer keeps the old URL.
      request.close()
    }
  }

  try runtime.discarding.setResourceTransform(transform)
  // #endregion rewrite

  // #region provider
  try runtime.discarding.setResourceProvider(ResourceProvider(
    callback: { request, handle in
      let response = if let tile = findTile(request.requestedUrl) {
        ResourceResponse(status: .ok, bytes: tile)
      } else {
        ResourceResponse(status: .noContent)
      }
      do {
        try handle.complete(response: response)
      } catch {
        handle.close()
      }
    },
    routes: [ResourceRoute(kinds: .tile, flags: .glob, url: "app://tiles/**")]
  ))
  // #endregion provider

  // #region custom-source
  try map.discarding.addCustomGeometrySource(
    "app-data",
    options: CustomGeometrySourceOptions(fetchTile: { tile in
      try? map.discarding.setCustomGeometrySourceTileData(
        "app-data",
        tileId: tile,
        data: geojsonForTile(tile)
      )
    })
  )
  // #endregion custom-source

  // #region status
  try setNetworkStatus(.offline)

  // Later, when the connection returns:
  try setNetworkStatus(.online)
  // #endregion status
}
