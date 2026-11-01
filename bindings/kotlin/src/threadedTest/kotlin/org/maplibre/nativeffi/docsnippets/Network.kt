// Snippets for the docs site's Network page. They compile with the tests, which never run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.CanonicalTileId
import org.maplibre.nativeffi.CustomGeometrySourceOptions
import org.maplibre.nativeffi.HttpHeader
import org.maplibre.nativeffi.HttpHeaderRule
import org.maplibre.nativeffi.HttpHeaderTransform
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.NetworkStatus
import org.maplibre.nativeffi.ResourceKindFlag
import org.maplibre.nativeffi.ResourceProvider
import org.maplibre.nativeffi.ResourceResponse
import org.maplibre.nativeffi.ResourceResponseStatus
import org.maplibre.nativeffi.ResourceRoute
import org.maplibre.nativeffi.ResourceTransform
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.UrlMatchFlag
import org.maplibre.nativeffi.setHttpHeaderTransform
import org.maplibre.nativeffi.setNetworkStatus

private fun withApiKey(url: String): String =
  url + (if ('?' in url) "&" else "?") + "key=my-api-key"

private fun findTile(url: String): ByteArray? = null

private fun geojsonForTile(tile: CanonicalTileId): String =
  """{"type": "FeatureCollection", "features": []}"""

private fun network(runtime: RuntimeHandle, map: MapHandle) {
  // #region headers
  runtime.discarding.setHttpHeaderTransform(
    HttpHeaderTransform(
      rules =
        listOf(
          HttpHeaderRule(
            flags = UrlMatchFlag.GLOB,
            url = "https://tiles.example.com/**",
            headers = listOf(HttpHeader(name = "Authorization", value = "Bearer my-token")),
          ),
        ),
    ),
  )
  // #endregion headers

  // #region rewrite
  runtime.discarding.setResourceTransform(
    ResourceTransform { _, url, request -> request.complete(withApiKey(url)) },
  )
  // #endregion rewrite

  // #region provider
  runtime.discarding.setResourceProvider(
    ResourceProvider(
      routes =
        listOf(
          ResourceRoute(
            kinds = ResourceKindFlag.TILE,
            flags = UrlMatchFlag.GLOB,
            url = "app://tiles/**",
          ),
        ),
    ) { request, handle ->
      val tile = findTile(request.requestedUrl)
      handle.complete(
        if (tile == null) {
          ResourceResponse(status = ResourceResponseStatus.NO_CONTENT)
        } else {
          ResourceResponse(status = ResourceResponseStatus.OK, bytes = tile)
        },
      )
    },
  )
  // #endregion provider

  // #region custom-source
  map.discarding.addCustomGeometrySource(
    "app-data",
    CustomGeometrySourceOptions(
      fetchTile = { tile ->
        map.discarding.setCustomGeometrySourceTileData("app-data", tile, geojsonForTile(tile))
      },
    ),
  )
  // #endregion custom-source

  // #region status
  setNetworkStatus(NetworkStatus.OFFLINE)

  // Later, when the connection returns:
  setNetworkStatus(NetworkStatus.ONLINE)
  // #endregion status
}
