// Snippets for the docs site's Network page. They compile with the
// tests,
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
  try runtime.discarding
    .setHttpHeaderTransform(HttpHeaderTransform(rules: [
      HttpHeaderRule(
        flags: .glob,
        url: "https://tiles.example.com/**",
        headers: [HttpHeader(
          name: "Authorization",
          value: "Bearer my-token"
        )]
      ),
    ]))
  // #endregion headers

  // #region rewrite
  let transform = ResourceTransform { _, url, request in
    // The answer ends the request whatever it returns. A failed one
    // fails
    // the request.
    try? request.complete(url: withApiKey(url))
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
      try? handle.complete(response: response)
    },
    routes: [ResourceRoute(
      kinds: .tile,
      flags: .glob,
      url: "app://tiles/**"
    )]
  ))
  // #endregion provider

  // #region custom-source
  try map.discarding.addCustomGeometrySource(
    "app-data",
    options: CustomGeometrySourceOptions(
      fetchTile: { sourceMap, sourceId, tile in
        try? sourceMap.discarding.setCustomGeometrySourceTileData(
          sourceId,
          tileId: tile,
          data: geojsonForTile(tile)
        )
      }
    )
  )
  // #endregion custom-source

  // #region status
  try setNetworkStatus(.offline)

  // Later, when the connection returns:
  try setNetworkStatus(.online)
  // #endregion status
}
