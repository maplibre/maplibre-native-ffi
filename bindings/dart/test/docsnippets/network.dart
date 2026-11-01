// Snippets for the docs site's Network page. They are analyzed with the tests,
// which never run them.

import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

String withApiKey(String url) =>
    '$url${url.contains('?') ? '&' : '?'}key=my-api-key';

Uint8List? findTile(String url) => null;

Object? geojsonForTile(CanonicalTileId tile) => {
  'type': 'FeatureCollection',
  'features': <Object?>[],
};

void network(RuntimeHandle runtime, MapHandle map) {
  // #region headers
  // This binding takes rules only, because it can't answer on a library thread.
  runtime.discarding.setHttpHeaderTransform(
    HttpHeaderTransform(
      rules: [
        HttpHeaderRule(
          flags: UrlMatchFlag.glob,
          url: 'https://tiles.example.com/**',
          headers: [
            HttpHeader(name: 'Authorization', value: 'Bearer my-token'),
          ],
        ),
      ],
    ),
  );
  // #endregion headers

  // #region rewrite
  runtime.discarding.setResourceTransform((_, url, request) {
    request.complete(withApiKey(url));
  });
  // #endregion rewrite

  // #region provider
  runtime.discarding.setResourceProvider(
    ResourceProvider(
      routes: [
        ResourceRoute(
          kinds: ResourceKindFlag.tile,
          flags: UrlMatchFlag.glob,
          url: 'app://tiles/**',
        ),
      ],
      callback: (request, handle) {
        final tile = findTile(request.requestedUrl);
        handle.complete(
          tile == null
              ? ResourceResponse(status: ResourceResponseStatus.noContent)
              : ResourceResponse(
                  status: ResourceResponseStatus.ok,
                  bytes: tile,
                ),
        );
      },
    ),
  );
  // #endregion provider

  // #region custom-source
  map.discarding.addCustomGeometrySource(
    'app-data',
    CustomGeometrySourceOptions(
      fetchTile: (tile) {
        map.discarding.setCustomGeometrySourceTileData(
          'app-data',
          tile,
          geojsonForTile(tile),
        );
      },
    ),
  );
  // #endregion custom-source

  // #region status
  setNetworkStatus(NetworkStatus.offline);

  // Later, when the connection returns:
  setNetworkStatus(NetworkStatus.online);
  // #endregion status
}
