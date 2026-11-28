// Snippets for the docs site's Camera page. The test runs them, with
// every request answered by the test instead of the network.
package org.maplibre.nativeffi.docsnippets;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import java.util.concurrent.CompletableFuture;
import org.junit.Test;
import org.maplibre.nativeffi.AnimationOptions;
import org.maplibre.nativeffi.BoundOptions;
import org.maplibre.nativeffi.Camera;
import org.maplibre.nativeffi.CameraFitUpdate;
import org.maplibre.nativeffi.CameraOptions;
import org.maplibre.nativeffi.CameraQueryResult;
import org.maplibre.nativeffi.CameraTransitionHandler;
import org.maplibre.nativeffi.CameraTransitionOutcome;
import org.maplibre.nativeffi.CameraUpdate;
import org.maplibre.nativeffi.CameraUpdateMode;
import org.maplibre.nativeffi.GestureInput;
import org.maplibre.nativeffi.GesturePointer;
import org.maplibre.nativeffi.GesturePointerDown;
import org.maplibre.nativeffi.GestureRecognizerHandle;
import org.maplibre.nativeffi.GestureResult;
import org.maplibre.nativeffi.LatLng;
import org.maplibre.nativeffi.LatLngBounds;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MapProjectionHandle;
import org.maplibre.nativeffi.MapSnapshot;
import org.maplibre.nativeffi.PointerKind;
import org.maplibre.nativeffi.ScreenPoint;

public class CameraTest {
  private Camera camera;
  private final CompletableFuture<Boolean> flyCompleted =
      new CompletableFuture<>();

  private void useCamera(Camera camera) {
    this.camera = camera;
  }

  private void usePoint(ScreenPoint point) {}

  private void flyEnded(boolean completed) {
    flyCompleted.complete(completed);
  }

  // #region gesture
  private static GestureRecognizerHandle createRecognizer(
      MapHandle map) {
    return map.gestureRecognizerCreate(null);
  }

  private static void scheduleTick(GestureResult result) {
    // On the next frame, or at result.getNextTickMs().
  }

  private static void pointerDown(
      GestureRecognizerHandle recognizer,
      int id,
      ScreenPoint position,
      double timeMs) {
    GestureInput input =
        new GestureInput.PointerDown(
            GesturePointerDown.of()
                .withId(id)
                .withKind(PointerKind.TOUCH)
                .withPosition(position));
    scheduleTick(recognizer.handle(input, timeMs));
  }

  private static void pointerMove(
      GestureRecognizerHandle recognizer,
      int id,
      ScreenPoint position,
      double timeMs) {
    GestureInput input =
        new GestureInput.PointerMove(GesturePointer.of(id, position));
    scheduleTick(recognizer.handle(input, timeMs));
  }

  private static void tick(
      GestureRecognizerHandle recognizer, double timeMs) {
    scheduleTick(recognizer.tick(timeMs));
  }

  // #endregion gesture

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void camera() throws Exception {
    try (SnippetFixture fixture = SnippetFixture.withMap()) {
      MapHandle map = fixture.map;

      // #region jump
      map.getDiscarding()
          .updateCamera(
              CameraUpdate.of()
                  .withMode(CameraUpdateMode.JUMP)
                  .withCamera(
                      CameraOptions.of()
                          .withCenter(new LatLng(48.8566, 2.3522))
                          .withZoom(12.0)));
      // #endregion jump

      // #region read
      MapSnapshot snapshot = map.getSnapshot();
      useCamera(snapshot.getView().getCamera());

      CameraQueryResult result = map.getCamera().join();
      useCamera(result.getCamera());
      // #endregion read
      assertEquals(12.0, camera.getZoom(), 0.0);

      // #region animate
      // Runs after the animation ends, on a library thread.
      CameraTransitionHandler onEnd =
          new CameraTransitionHandler(
              end -> {
                CameraTransitionOutcome outcome = end.getOutcome();
                flyEnded(
                    outcome.equals(
                        CameraTransitionOutcome.COMPLETED));
              });
      // An ID to cancel this animation by.
      long flight = 1;
      map.getDiscarding()
          .updateCamera(
              CameraUpdate.of()
                  .withMode(CameraUpdateMode.FLY)
                  .withCamera(
                      CameraOptions.of()
                          .withCenter(new LatLng(40.7128, -74.006))
                          .withZoom(10.0))
                  .withAnimation(
                      AnimationOptions.of()
                          .withDurationMs(3000.0)
                          .withTransitionId(flight)
                          .withEndHandler(onEnd)));

      // Later, stop the flight where it is.
      map.getDiscarding().cancelCameraTransitionsWithId(flight);
      // #endregion animate
      assertFalse(SnippetFixture.within(flyCompleted));

      // #region fit
      LatLngBounds bounds =
          new LatLngBounds(
              new LatLng(45.8, 5.9), new LatLng(47.8, 10.5));
      map.getDiscarding()
          .fitCameraToLatLngBounds(
              bounds,
              CameraFitUpdate.of()
                  .withMode(CameraUpdateMode.EASE)
                  .withAnimation(
                      AnimationOptions.of().withDurationMs(500.0)));
      // #endregion fit

      // #region limit
      map.getDiscarding()
          .setBounds(
              BoundOptions.of()
                  .withBounds(bounds)
                  .withMinZoom(5.0)
                  .withMaxZoom(16.0));
      // #endregion limit

      try (GestureRecognizerHandle recognizer =
          createRecognizer(map)) {
        pointerDown(
            recognizer, 1, new ScreenPoint(100.0, 100.0), 0.0);
        pointerMove(
            recognizer, 1, new ScreenPoint(112.0, 96.0), 16.0);
        tick(recognizer, 32.0);
      }

      // #region convert
      ScreenPoint point;
      try (MapProjectionHandle projection =
          map.projectionCreate(null)) {
        LatLng coordinate =
            projection.latLngForPixel(new ScreenPoint(100.0, 200.0));
        point = projection.pixelForLatLng(coordinate);
      }
      // #endregion convert
      usePoint(point);
    }
  }
}
