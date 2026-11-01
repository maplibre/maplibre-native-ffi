// Snippets for the docs site's Network page. They are type-checked with the
// tests, which never run them.

import {
  type CanonicalTileId,
  type MapHandle,
  NetworkStatus,
  ResourceKindFlag,
  ResourceResponseStatus,
  type RuntimeHandle,
  setNetworkStatus,
  UrlMatchFlag,
} from "../../src/index.ts";

function withApiKey(url: string): string {
  return `${url}${url.includes("?") ? "&" : "?"}key=my-api-key`;
}

function findTile(_url: string): Uint8Array | undefined {
  return undefined;
}

function geojsonForTile(_tile: CanonicalTileId): unknown {
  return { type: "FeatureCollection", features: [] };
}

export function network(runtime: RuntimeHandle, map: MapHandle): void {
  // #region headers
  // This binding takes rules only, because it can't answer on a library thread.
  runtime.discarding.setHttpHeaderTransform({
    rules: [
      {
        flags: UrlMatchFlag.glob,
        url: "https://tiles.example.com/**",
        headers: [{ name: "Authorization", value: "Bearer my-token" }],
      },
    ],
  });
  // #endregion headers

  // #region rewrite
  runtime.discarding.setResourceTransform((_kind, url, request) => {
    request.complete(withApiKey(url));
  });
  // #endregion rewrite

  // #region provider
  runtime.discarding.setResourceProvider({
    routes: [
      {
        kinds: ResourceKindFlag.tile,
        flags: UrlMatchFlag.glob,
        url: "app://tiles/**",
      },
    ],
    callback: (request, handle) => {
      const tile = findTile(request.requestedUrl);
      handle.complete(
        tile === undefined
          ? { status: ResourceResponseStatus.noContent }
          : { status: ResourceResponseStatus.ok, bytes: tile },
      );
    },
  });
  // #endregion provider

  // #region custom-source
  map.discarding.addCustomGeometrySource("app-data", {
    fetchTile: (tile) => {
      map.discarding.setCustomGeometrySourceTileData(
        "app-data",
        tile,
        geojsonForTile(tile),
      );
    },
  });
  // #endregion custom-source

  // #region status
  setNetworkStatus(NetworkStatus.offline);

  // Later, when the connection returns:
  setNetworkStatus(NetworkStatus.online);
  // #endregion status
}
