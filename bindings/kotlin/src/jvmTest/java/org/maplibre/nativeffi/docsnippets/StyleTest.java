// Snippets for the docs site's Style page. The test runs them, with
// every request answered by the test instead of the network.
package org.maplibre.nativeffi.docsnippets;

import static org.junit.Assert.assertEquals;

import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import org.junit.Test;
import org.maplibre.nativeffi.GeoJsonSourceDataHandle;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MapMode;
import org.maplibre.nativeffi.PremultipliedRgba8Image;
import org.maplibre.nativeffi.RenderSessionHandle;
import org.maplibre.nativeffi.RuntimeEvent;
import org.maplibre.nativeffi.RuntimeEventHandler;
import org.maplibre.nativeffi.RuntimeEventPayload;
import org.maplibre.nativeffi.RuntimeHandle;
import org.maplibre.nativeffi.StyleLayerInfo;
import org.maplibre.nativeffi.StyleLayerVisibility;

public class StyleTest {
  /** The demo style here: the layers that the snippets name. */
  private static final String DEMO_STYLE_JSON =
      """
      {
        "version": 8,
        "sources": {
          "countries": {
            "type": "geojson",
            "data": {"type": "FeatureCollection", "features": []}
          }
        },
        "layers": [
          {
            "id": "countries-boundary",
            "type": "line",
            "source": "countries"
          },
          {
            "id": "countries-label",
            "type": "symbol",
            "source": "countries"
          }
        ]
      }
      """;

  private final CompletableFuture<String> drawn =
      new CompletableFuture<>();

  private PremultipliedRgba8Image drawImage(String imageId) {
    drawn.complete(imageId);
    return PremultipliedRgba8Image.of(
        1, 1, 4, new byte[] {-1, 0, 0, -1});
  }

  // pixels holds 16 x 16 premultiplied RGBA pixels.
  private static void style(MapHandle map, byte[] pixels) {
    // #region load
    map.setStyleUrl("https://demotiles.maplibre.org/style.json")
        .join();
    // #endregion load

    // #region add
    map.getDiscarding()
        .addStyleSourceJson(
            "satellite",
            """
            {
              "type": "raster",
              "tileSize": 256,
              "tiles": ["https://tiles.example.com/{z}/{x}/{y}.png"]
            }
            """);

    // Draw the layer under the "countries-label" layer.
    map.getDiscarding()
        .addStyleLayerJson(
            """
            {"id": "satellite", "type": "raster", "source": "satellite"}
            """,
            "countries-label");
    // #endregion add

    // #region geojson
    GeoJsonSourceDataHandle data =
        GeoJsonSourceDataHandle.create(
            """
            {"type": "Point", "coordinates": [-122.4, 37.8]}
            """,
            null);
    map.getDiscarding().addGeoJsonSourceData("points", data, null);
    // The source keeps its own reference to the data.
    data.release();

    // Later, prepare new data with the same options, and set it.
    GeoJsonSourceDataHandle next =
        GeoJsonSourceDataHandle.create(
            """
            {"type": "Point", "coordinates": [2.35, 48.86]}
            """,
            null);
    map.getDiscarding().setGeoJsonSourceData("points", next);
    next.release();
    // #endregion geojson

    // #region set-property
    // Show both changes in the same frame.
    map.getDiscarding().beginCommandGroup();
    map.getDiscarding()
        .setStyleLayerProperty("satellite", "visibility", "\"none\"");
    map.getDiscarding()
        .setStyleLayerProperty(
            "countries-boundary",
            "line-color",
            """
            ["interpolate", ["linear"], ["zoom"], 5, "#9cf", 12, "#06c"]
            """);
    map.getDiscarding().endCommandGroup();
    // #endregion set-property

    // #region image
    map.getDiscarding()
        .setStyleImage(
            "marker",
            PremultipliedRgba8Image.of(16, 16, 16 * 4, pixels),
            null);

    // A null layer ID draws the layer on top.
    map.getDiscarding()
        .addStyleLayerJson(
            """
            {
              "id": "markers",
              "type": "symbol",
              "source": "points",
              "layout": {"icon-image": "marker"}
            }
            """,
            null);
    // #endregion image
  }

  private void styleEvents(RuntimeHandle runtime, MapHandle map) {
    // #region missing-image
    // Runs on a library thread.
    runtime
        .getDiscarding()
        .setEventHandler(
            new RuntimeEventHandler(
                events -> {
                  for (RuntimeEvent event : events) {
                    if (event.getSource() == map.getId()
                        && event.getPayload()
                            instanceof
                            RuntimeEventPayload.StyleImage missing) {
                      String id = missing.getValue().getImageId();
                      map.getDiscarding()
                          .setStyleImage(id, drawImage(id), null);
                    }
                  }
                }));
    // #endregion missing-image
  }

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void style() throws Exception {
    try (SnippetFixture fixture =
        SnippetFixture.withMap(MapMode.STATIC)) {
      SnippetFixture.serve(
          fixture.runtime,
          Map.of(SnippetFixture.DEMO_STYLE_URL, DEMO_STYLE_JSON));
      MapHandle map = fixture.map;
      style(map, new byte[16 * 16 * 4]);

      assertEquals(
          List.of(
              "countries-boundary",
              "satellite",
              "countries-label",
              "markers"),
          map.listStyleLayers().join().stream()
              .map(StyleLayerInfo::getId)
              .toList());
      assertEquals(
          StyleLayerVisibility.NONE,
          map.getStyleLayer("satellite").join().getVisibility());

      // Rendering the marker without its image asks for it.
      styleEvents(fixture.runtime, map);
      map.removeStyleImage("marker").join();
      RenderSessionHandle session = fixture.attachOffscreen();
      try {
        map.requestStillImage().join();
        assertEquals("marker", SnippetFixture.within(drawn));
      } finally {
        session.release().join();
      }
    }
  }
}
