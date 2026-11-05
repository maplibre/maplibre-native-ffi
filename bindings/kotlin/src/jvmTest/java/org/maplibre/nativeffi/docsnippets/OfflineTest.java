// Snippets for the docs site's Offline page. The test runs them, with
// every request answered by the test instead of the network.
package org.maplibre.nativeffi.docsnippets;

import static org.junit.Assert.assertEquals;

import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.maplibre.nativeffi.CacheDatabaseOperation;
import org.maplibre.nativeffi.LatLng;
import org.maplibre.nativeffi.LatLngBounds;
import org.maplibre.nativeffi.OfflineRegionDefinition;
import org.maplibre.nativeffi.OfflineRegionDownloadState;
import org.maplibre.nativeffi.OfflineRegionInfo;
import org.maplibre.nativeffi.OfflineRegionStatus;
import org.maplibre.nativeffi.OfflineTilePyramidRegionDefinition;
import org.maplibre.nativeffi.RuntimeEvent;
import org.maplibre.nativeffi.RuntimeEventFlag;
import org.maplibre.nativeffi.RuntimeEventHandler;
import org.maplibre.nativeffi.RuntimeEventPayload;
import org.maplibre.nativeffi.RuntimeHandle;
import org.maplibre.nativeffi.RuntimeOptions;

public class OfflineTest {
  @Rule public final TemporaryFolder folder = new TemporaryFolder();

  private final List<String> regions = new ArrayList<>();

  private void recordProgress(
      long regionId,
      long completed,
      long required,
      boolean complete) {}

  private void recordError(long regionId, String message) {}

  private void recordRegion(long regionId, byte[] metadata) {
    regions.add(new String(metadata, StandardCharsets.UTF_8));
  }

  // #region progress
  private void handleEvents(List<RuntimeEvent> events) {
    for (RuntimeEvent event : events) {
      RuntimeEventPayload payload = event.getPayload();
      if (payload
          instanceof
          RuntimeEventPayload.OfflineRegionStatus changed) {
        OfflineRegionStatus status = changed.getValue().getStatus();
        recordProgress(
            changed.getValue().getRegionId(),
            status.getCompletedResourceCount(),
            status.getRequiredResourceCount(),
            status.getComplete());
      } else if (payload
          instanceof
          RuntimeEventPayload.OfflineRegionResponseError error) {
        recordError(
            error.getValue().getRegionId(), event.getMessage());
      }
    }
  }

  // #endregion progress

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void offline() {
    Path cacheDirectory = folder.getRoot().toPath();

    // #region cache-file
    RuntimeHandle runtime =
        RuntimeHandle.create(
                RuntimeOptions.of()
                    .withCachePath(
                        cacheDirectory
                            .resolve("maplibre-cache.db")
                            .toString()))
            .join();
    // #endregion cache-file
    try {
      SnippetFixture.keepOffTheNetwork(runtime);

      // #region progress
      // Runs on a library thread.
      runtime
          .getDiscarding()
          .setEventHandler(
              new RuntimeEventHandler(this::handleEvents));
      // #endregion progress

      // #region ambient
      // 50 MiB
      runtime.setMaximumAmbientCacheSize(50L * 1024 * 1024).join();
      runtime
          .runCacheDatabaseOperation(CacheDatabaseOperation.CLEAR)
          .join();
      // #endregion ambient

      // #region download
      OfflineRegionInfo region =
          runtime
              .createOfflineRegion(
                  new OfflineRegionDefinition.TilePyramid(
                      OfflineTilePyramidRegionDefinition.of()
                          .withStyleUrl(
                              "https://demotiles.maplibre.org/style.json")
                          .withBounds(
                              new LatLngBounds(
                                  new LatLng(47.5, -122.5),
                                  new LatLng(47.8, -122.2)))
                          .withMinZoom(0.0)
                          .withMaxZoom(6.0)
                          .withPixelRatio(1f)),
                  "Seattle".getBytes(StandardCharsets.UTF_8))
              .join();

      runtime
          .setOfflineRegionDownloadState(
              region.getId(), OfflineRegionDownloadState.ACTIVE)
          .join();
      // #endregion download

      // #region progress
      runtime
          .setOfflineRegionObserved(
              region.getId(), RuntimeEventFlag.ALL_RUNTIME_EVENTS)
          .join();
      // #endregion progress

      // #region manage
      for (OfflineRegionInfo stored :
          runtime.listOfflineRegions().join()) {
        recordRegion(stored.getId(), stored.getMetadata());
      }

      // Resume a download that stopped when the runtime was released.
      runtime
          .setOfflineRegionDownloadState(
              region.getId(), OfflineRegionDownloadState.ACTIVE)
          .join();

      runtime.invalidateOfflineRegion(region.getId()).join();

      runtime.deleteOfflineRegion(region.getId()).join();
      // #endregion manage
      assertEquals(List.of("Seattle"), regions);
    } finally {
      runtime.release().join();
    }
  }
}
