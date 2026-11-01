// Snippets for the docs site's Network page. They compile with the tests,
// which never run them.

using System.Collections.Immutable;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class Network
{
    private static string WithApiKey(string url) =>
        $"{url}{(url.Contains('?') ? '&' : '?')}key=my-api-key";

    private static ImmutableArray<byte>? FindTile(string url) => null;

    private static string GeoJsonForTile(CanonicalTileId tile) =>
        """{"type":"FeatureCollection","features":[]}""";

    internal static void Run(RuntimeHandle runtime, MapHandle map)
    {
        // #region headers
        runtime.Discarding.SetHttpHeaderTransform(
            new HttpHeaderTransform
            {
                Rules =
                [
                    new HttpHeaderRule
                    {
                        Flags = UrlMatchFlag.Glob,
                        Url = "https://tiles.example.com/**",
                        Headers = [new HttpHeader("Authorization", "Bearer my-token")],
                    },
                ],
            }
        );
        // #endregion headers

        // #region rewrite
        // Runs on a library thread.
        runtime.Discarding.SetResourceTransform(
            new ResourceTransform((_, url, request) => request.Complete(WithApiKey(url)))
        );
        // #endregion rewrite

        // #region provider
        runtime.Discarding.SetResourceProvider(
            new ResourceProvider
            {
                Routes =
                [
                    new ResourceRoute
                    {
                        Kinds = ResourceKindFlag.Tile,
                        Flags = UrlMatchFlag.Glob,
                        Url = "app://tiles/**",
                    },
                ],
                // Runs on a library thread.
                Callback = (request, handle) =>
                    handle.Complete(
                        FindTile(request.RequestedUrl) is { } tile
                            ? new ResourceResponse
                            {
                                Status = ResourceResponseStatus.Ok,
                                Bytes = tile,
                            }
                            : new ResourceResponse { Status = ResourceResponseStatus.NoContent }
                    ),
            }
        );
        // #endregion provider

        // #region custom-source
        map.Discarding.AddCustomGeometrySource(
            "app-data",
            new CustomGeometrySourceOptions
            {
                // Runs on a library thread.
                FetchTile = tile =>
                    map.Discarding.SetCustomGeometrySourceTileData(
                        "app-data",
                        tile,
                        GeoJsonForTile(tile)
                    ),
            }
        );
        // #endregion custom-source

        // #region status
        MaplibreNative.SetNetworkStatus(NetworkStatus.Offline);

        // Later, when the connection returns:
        MaplibreNative.SetNetworkStatus(NetworkStatus.Online);
        // #endregion status
    }
}
