// Snippets for the docs site's Create a map page. The test runs them,
// with every request answered by the test instead of the network.
package org.maplibre.nativeffi.docsnippets;

import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.util.concurrent.CompletionException;
import org.junit.Test;
import org.maplibre.nativeffi.CameraOptions;
import org.maplibre.nativeffi.CameraUpdate;
import org.maplibre.nativeffi.CameraUpdateMode;
import org.maplibre.nativeffi.LogicalExtent;
import org.maplibre.nativeffi.LogicalSize;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MapMode;
import org.maplibre.nativeffi.MapOptions;
import org.maplibre.nativeffi.MaplibreException;
import org.maplibre.nativeffi.OffscreenTarget;
import org.maplibre.nativeffi.RenderDriverKind;
import org.maplibre.nativeffi.RenderSessionAttachOptions;
import org.maplibre.nativeffi.RenderSessionHandle;
import org.maplibre.nativeffi.RenderTarget;
import org.maplibre.nativeffi.RuntimeHandle;

public class CreateAMapTest {
  private Throwable failure;

  private void reportFailure(Throwable error) {
    failure = error;
  }

  // Attaches a render target, as the render target guides show.
  private static RenderSessionHandle attachTarget(MapHandle map) {
    RenderSessionHandle session =
        map.attach(
            new RenderTarget.Offscreen(
                new OffscreenTarget(LogicalSize.of(800, 600))),
            null,
            RenderSessionAttachOptions.of(
                RenderDriverKind.CORE_WORKER));
    session.getReady().join();
    return session;
  }

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void createAMap() {
    // #region runtime
    RuntimeHandle runtime = RuntimeHandle.create(null);
    // #endregion runtime
    SnippetFixture.keepOffTheNetwork(runtime);

    // #region map
    MapHandle map =
        runtime.createMap(
            MapOptions.of(LogicalExtent.of(800, 600, 2.0))
                .withMapMode(MapMode.CONTINUOUS));
    map.getReady().join();
    // #endregion map

    // #region style
    map.getDiscarding()
        .setStyleUrl("https://demotiles.maplibre.org/style.json");
    // #endregion style

    CameraUpdate update =
        CameraUpdate.of()
            .withMode(CameraUpdateMode.JUMP)
            .withCamera(CameraOptions.of().withZoom(3.0));

    // #region wait
    // join() blocks until the result arrives. To continue without
    // blocking, chain a stage such as thenRun() instead.
    map.updateCamera(update).join();

    // Use the discarding variant to ignore the result.
    map.getDiscarding().updateCamera(update);
    // #endregion wait

    // #region errors
    try {
      map.updateCamera(update).join();
    } catch (MaplibreException error) {
      // The call failed right away. Its message describes the
      // failure.
      reportFailure(error);
    } catch (CompletionException error) {
      // The result failed, with a MaplibreException as the cause.
      reportFailure(error.getCause());
    }
    // #endregion errors
    assertNull(failure);

    RenderSessionHandle session = attachTarget(map);

    // #region close
    try {
      // Render frames and change the map.
    } finally {
      // Release each one, and wait for it, in this order.
      session.release().join();
      map.release().join();
      runtime.release().join();
    }
    // #endregion close
    assertTrue(runtime.isClosed());
  }
}
