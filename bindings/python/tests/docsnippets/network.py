# Snippets for the docs site's Network page. They are type-checked with the
# tests, which never run them.

from typing import Any

import maplibre_native_ffi as mln


def with_api_key(url: str) -> str:
    return f"{url}{'&' if '?' in url else '?'}key=my-api-key"


def find_tile(url: str) -> bytes | None:
    return None


def geojson_for_tile(tile: mln.CanonicalTileId) -> Any:
    return {"type": "FeatureCollection", "features": []}


def network(runtime: mln.RuntimeHandle, map_handle: mln.MapHandle) -> None:
    # #region headers
    # This binding takes rules only, because it can't answer on a library thread.
    runtime.discarding.set_http_header_transform(
        mln.HttpHeaderTransform(
            rules=[
                mln.HttpHeaderRule(
                    flags=mln.UrlMatchFlag.GLOB,
                    url="https://tiles.example.com/**",
                    headers=[
                        mln.HttpHeader(name="Authorization", value="Bearer my-token")
                    ],
                )
            ]
        )
    )
    # #endregion headers

    # #region rewrite
    def add_api_key(
        kind: mln.ResourceKind, url: str, request: mln.ResourceTransformRequestHandle
    ) -> None:
        request.complete(with_api_key(url))

    runtime.discarding.set_resource_transform(mln.ResourceTransform(add_api_key))
    # #endregion rewrite

    # #region provider
    def serve_tile(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        tile = find_tile(request.requested_url)
        if tile is None:
            handle.complete(
                mln.ResourceResponse(status=mln.ResourceResponseStatus.NO_CONTENT)
            )
        else:
            handle.complete(
                mln.ResourceResponse(status=mln.ResourceResponseStatus.OK, bytes=tile)
            )

    runtime.discarding.set_resource_provider(
        mln.ResourceProvider(
            serve_tile,
            routes=[
                mln.ResourceRoute(
                    kinds=mln.ResourceKindFlag.TILE,
                    flags=mln.UrlMatchFlag.GLOB,
                    url="app://tiles/**",
                )
            ],
        )
    )
    # #endregion provider

    # #region custom-source
    def fetch_tile(tile: mln.CanonicalTileId) -> None:
        map_handle.discarding.set_custom_geometry_source_tile_data(
            "app-data", tile, geojson_for_tile(tile)
        )

    map_handle.discarding.add_custom_geometry_source(
        "app-data", mln.CustomGeometrySourceOptions(fetch_tile=fetch_tile)
    )
    # #endregion custom-source

    # #region status
    mln.set_network_status(mln.NetworkStatus.OFFLINE)

    # Later, when the connection returns:
    mln.set_network_status(mln.NetworkStatus.ONLINE)
    # #endregion status
