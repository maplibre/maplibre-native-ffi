// Snippets for the docs site's Render images page. The test runs
// them, with every request answered by the test instead of the
// network.
package org.maplibre.nativeffi.docsnippets;

import static org.junit.Assert.assertEquals;

import java.util.Map;
import org.junit.Test;
import org.maplibre.nativeffi.CameraOptions;
import org.maplibre.nativeffi.CameraUpdate;
import org.maplibre.nativeffi.CameraUpdateMode;
import org.maplibre.nativeffi.CanonicalTileId;
import org.maplibre.nativeffi.LatLng;
import org.maplibre.nativeffi.LogicalExtent;
import org.maplibre.nativeffi.LogicalSize;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MapMode;
import org.maplibre.nativeffi.MapOptions;
import org.maplibre.nativeffi.MaplibreNative;
import org.maplibre.nativeffi.OffscreenDescriptor;
import org.maplibre.nativeffi.RenderDriverKind;
import org.maplibre.nativeffi.RenderSessionAttachOptions;
import org.maplibre.nativeffi.RenderSessionHandle;
import org.maplibre.nativeffi.RuntimeHandle;
import org.maplibre.nativeffi.TextureReadbackResult;

public class ImagesTest {
  private int copiedRows;

  private void copyPixels(byte[] data, int stride) {
    copiedRows = data.length / stride;
  }

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void images() {
    RuntimeHandle runtime = RuntimeHandle.create(null);
    try {
      // The demo style is empty here.
      SnippetFixture.serve(
          runtime,
          Map.of(
              SnippetFixture.DEMO_STYLE_URL,
              SnippetFixture.EMPTY_STYLE_JSON));

      // #region map
      MapHandle map =
          runtime.createMap(
              MapOptions.of(LogicalExtent.of(800, 600, 2.0))
                  .withMapMode(MapMode.STATIC));
      map.getReady().join();
      // #endregion map

      // #region style
      map.getDiscarding()
          .setStyleUrl("https://demotiles.maplibre.org/style.json");
      // #endregion style

      // #region attach
      RenderSessionHandle session =
          map.attachOffscreen(
              new OffscreenDescriptor(LogicalSize.of(800, 600)),
              // The library renders on its own thread.
              RenderSessionAttachOptions.of(
                  RenderDriverKind.CORE_WORKER),
              null);
      session.getReady().join();
      // #endregion attach

      try {
        // #region image
        map.getDiscarding()
            .updateCamera(
                CameraUpdate.of()
                    .withMode(CameraUpdateMode.JUMP)
                    .withCamera(
                        CameraOptions.of()
                            .withCenter(new LatLng(48.8566, 2.3522))
                            .withZoom(4.0)));

        map.requestStillImage().join();
        // #endregion image

        // #region read
        TextureReadbackResult image = session.readTexture().join();
        copyPixels(image.getData(), image.getInfo().getStride());
        // #endregion read
        // 600 logical pixels at a scale factor of 2.
        assertEquals(1200, copiedRows);
      } finally {
        session.release().join();
        map.release().join();
      }

      imagesTile(runtime, 3, 4, 2);
    } finally {
      runtime.release().join();
    }
  }

  private static void imagesTile(
      RuntimeHandle runtime, int z, int x, int y) {
    // #region tile
    MapHandle map =
        runtime.createMap(
            MapOptions.of(LogicalExtent.of(512, 512, 1.0))
                .withMapMode(MapMode.TILE));
    map.getReady().join();

    map.getDiscarding()
        .updateCamera(
            CameraUpdate.of()
                .withMode(CameraUpdateMode.JUMP)
                .withCamera(
                    MaplibreNative.cameraForTile(
                        CanonicalTileId.of(z, x, y),
                        LogicalSize.of(512, 512))));
    // #endregion tile

    try {
      assertEquals(
          z, map.getCamera().join().getCamera().getZoom(), 0.01);
    } finally {
      map.release().join();
    }
  }
}
