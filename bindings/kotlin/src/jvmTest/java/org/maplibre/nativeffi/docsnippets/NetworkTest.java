// Snippets for the docs site's Network page. The test runs them, with
// every request answered by the test instead of the network.
package org.maplibre.nativeffi.docsnippets;

import static org.junit.Assert.assertEquals;

import java.util.List;
import org.junit.Test;
import org.maplibre.nativeffi.CanonicalTileId;
import org.maplibre.nativeffi.CustomGeometrySourceOptions;
import org.maplibre.nativeffi.HttpHeader;
import org.maplibre.nativeffi.HttpHeaderRule;
import org.maplibre.nativeffi.HttpHeaderTransform;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MaplibreNative;
import org.maplibre.nativeffi.NetworkStatus;
import org.maplibre.nativeffi.ResourceKindFlag;
import org.maplibre.nativeffi.ResourceProvider;
import org.maplibre.nativeffi.ResourceRequestHandle;
import org.maplibre.nativeffi.ResourceRequestInfo;
import org.maplibre.nativeffi.ResourceResponse;
import org.maplibre.nativeffi.ResourceResponseStatus;
import org.maplibre.nativeffi.ResourceRoute;
import org.maplibre.nativeffi.ResourceTransform;
import org.maplibre.nativeffi.ResourceTransformRule;
import org.maplibre.nativeffi.RuntimeHandle;
import org.maplibre.nativeffi.StyleSourceInfo;
import org.maplibre.nativeffi.UrlMatchFlag;
import org.maplibre.nativeffi.UrlQueryParameter;

public class NetworkTest {
  private String withApiKey(String url) {
    return url + (url.contains("?") ? "&" : "?") + "key=my-api-key";
  }

  private byte[] findTile(String url) {
    return null;
  }

  private String geojsonForTile(CanonicalTileId tile) {
    return "{\"type\": \"FeatureCollection\", \"features\": []}";
  }

  // #region provider
  // Runs on a library thread.
  private void serveTile(
      ResourceRequestInfo request, ResourceRequestHandle handle) {
    byte[] tile = findTile(request.getRequestedUrl());
    if (tile == null) {
      handle.complete(
          ResourceResponse.of()
              .withStatus(ResourceResponseStatus.NO_CONTENT));
    } else {
      handle.complete(
          ResourceResponse.of()
              .withStatus(ResourceResponseStatus.OK)
              .withBytes(tile));
    }
  }

  // #endregion provider

  private void network(RuntimeHandle runtime, MapHandle map) {
    // #region headers
    HttpHeaderRule authorization =
        HttpHeaderRule.of()
            .withFlags(UrlMatchFlag.GLOB)
            .withUrl("https://tiles.example.com/**")
            .withHeaders(
                List.of(
                    HttpHeader.of("Authorization")
                        .withValue("Bearer my-token")));
    MaplibreNative.setHttpHeaderTransform(
        runtime.getDiscarding(),
        HttpHeaderTransform.of().withRules(List.of(authorization)));
    // #endregion headers

    // #region query-rules
    ResourceTransformRule apiKey =
        ResourceTransformRule.of()
            .withFlags(UrlMatchFlag.GLOB)
            .withUrl("https://tiles.example.com/**")
            .withParameters(
                List.of(
                    UrlQueryParameter.of("key")
                        .withValue("my-api-key")));
    runtime
        .getDiscarding()
        .setResourceTransform(
            ResourceTransform.of().withRules(List.of(apiKey)));
    // #endregion query-rules

    // #region rewrite
    runtime
        .getDiscarding()
        .setResourceTransform(
            ResourceTransform.of()
                .withCallback(
                    (kind, url, request) ->
                        request.complete(withApiKey(url))));
    // #endregion rewrite

    // #region provider
    ResourceRoute tiles =
        ResourceRoute.of()
            .withKinds(ResourceKindFlag.TILE)
            .withFlags(UrlMatchFlag.GLOB)
            .withUrl("app://tiles/**");
    runtime
        .getDiscarding()
        .setResourceProvider(
            ResourceProvider.of(this::serveTile)
                .withRoutes(List.of(tiles)));
    // #endregion provider

    // #region custom-source
    map.getDiscarding()
        .addCustomGeometrySource(
            "app-data",
            CustomGeometrySourceOptions.of(
                (sourceMap, sourceId, tile) ->
                    sourceMap
                        .getDiscarding()
                        .setCustomGeometrySourceTileData(
                            sourceId, tile, geojsonForTile(tile))));
    // #endregion custom-source

    // #region status
    MaplibreNative.setNetworkStatus(NetworkStatus.OFFLINE);

    // Later, when the connection returns:
    MaplibreNative.setNetworkStatus(NetworkStatus.ONLINE);
    // #endregion status
  }

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void network() {
    try (SnippetFixture fixture = SnippetFixture.withMap()) {
      fixture
          .map
          .setStyleJson(SnippetFixture.EMPTY_STYLE_JSON)
          .join();
      // The map loads nothing, so the provider that replaces the
      // test's own never sends a request to the network.
      network(fixture.runtime, fixture.map);

      assertEquals(
          List.of("app-data"),
          fixture.map.listStyleSources().join().stream()
              .map(StyleSourceInfo::getId)
              .toList());
      assertEquals(
          NetworkStatus.ONLINE, MaplibreNative.getNetworkStatus());
    }
  }
}
