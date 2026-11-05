// Snippets for the docs site's Query page. The test runs them, with
// every request answered by the test instead of the network.
package org.maplibre.nativeffi.docsnippets;

import static org.junit.Assert.assertEquals;

import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.maplibre.nativeffi.MapMode;
import org.maplibre.nativeffi.RenderSessionHandle;
import org.maplibre.nativeffi.RenderedFeature;
import org.maplibre.nativeffi.RenderedFeatureQueryOptions;
import org.maplibre.nativeffi.RenderedQueryGeometry;
import org.maplibre.nativeffi.ScreenBox;
import org.maplibre.nativeffi.ScreenPoint;
import org.maplibre.nativeffi.SourceFeature;
import org.maplibre.nativeffi.SourceFeatureQueryOptions;

public class QueryTest {
  private final List<String> used = new ArrayList<>();

  private void useFeature(String feature) {
    used.add(feature);
  }

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void query() {
    try (SnippetFixture fixture =
        SnippetFixture.withMap(MapMode.STATIC)) {
      fixture
          .map
          .setStyleJson(SnippetFixture.EMPTY_STYLE_JSON)
          .join();
      RenderSessionHandle session = fixture.attachOffscreen();
      try {
        // A session can query once it has rendered the map.
        fixture.map.requestStillImage().join();
        ScreenPoint point = new ScreenPoint(32.0, 32.0);

        // #region rendered
        List<RenderedFeature> hits =
            session
                .queryRenderedFeatures(
                    new RenderedQueryGeometry.Box(
                        new ScreenBox(
                            new ScreenPoint(
                                point.getX() - 5, point.getY() - 5),
                            new ScreenPoint(
                                point.getX() + 5, point.getY() + 5))),
                    RenderedFeatureQueryOptions.of()
                        .withLayerIds(List.of("poi")))
                .join();
        for (RenderedFeature hit : hits) {
          // hit.getFeature() is the GeoJSON feature, as JSON.
          useFeature(hit.getFeature());
        }
        // #endregion rendered

        // #region source
        List<SourceFeature> features =
            session
                .querySourceFeatures(
                    "openmaptiles",
                    SourceFeatureQueryOptions.of()
                        .withSourceLayerIds(List.of("transportation"))
                        // A filter from the MapLibre style spec.
                        .withFilter(
                            """
                            ["==", ["get", "class"], "motorway"]
                            """))
                .join();
        for (SourceFeature feature : features) {
          useFeature(feature.getFeature());
        }
        // #endregion source

        // The empty style has no features to find.
        assertEquals(List.of(), used);
      } finally {
        session.release().join();
      }
    }
  }
}
