// Snippets for the docs site's Render to a window page. They compile
// with the tests, which never run them, since they need your Wayland
// window.
package org.maplibre.nativeffi.docsnippets;

import java.util.List;
import org.maplibre.nativeffi.FrameDemand;
import org.maplibre.nativeffi.LogicalExtent;
import org.maplibre.nativeffi.LogicalSize;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.NativePointer;
import org.maplibre.nativeffi.RenderDriverKind;
import org.maplibre.nativeffi.RenderFrameResult;
import org.maplibre.nativeffi.RenderResult;
import org.maplibre.nativeffi.RenderSessionAttachOptions;
import org.maplibre.nativeffi.RenderSessionHandle;
import org.maplibre.nativeffi.RenderTarget;
import org.maplibre.nativeffi.Wake;
import org.maplibre.nativeffi.WaylandSurfaceTarget;

public class WindowTest {
  // Stand-ins for the host's event loop, display timer, and graphics
  // thread.
  private static void runOnEventLoop(Runnable task) {}

  private static void runAtNextRefresh(Runnable task) {}

  private static void runOnGraphicsThread(Runnable task) {}

  private void replaceSession() {}

  private void destroySurface() {}

  private void useOptions(RenderSessionAttachOptions options) {}

  // #region attach
  private RenderSessionHandle session;

  // #endregion attach

  // #region frame-loop
  // Draws the map if it changed since the last frame.
  private void demandFrame() {
    session.demandFrame(FrameDemand.of());
  }

  private void drainFrameResults() {
    List<RenderFrameResult> results = session.drainFrameResults();
    if (results.isEmpty()) return;

    if (results.stream()
        .anyMatch(
            result ->
                result
                    .getDisposition()
                    .equals(RenderResult.FAILED))) {
      replaceSession(); // Release the session and attach a new one.
    }
  }

  // #endregion frame-loop

  // #region frame-wake
  // Both run on a library thread.
  private void onRedrawWake() {
    runAtNextRefresh(this::demandFrame);
  }

  private void onFrameWake() {
    runOnEventLoop(this::drainFrameResults);
  }

  // #endregion frame-wake

  // #region resize
  // Runs when the window changes size, or moves to a display with
  // another scale factor.
  private void onWindowResized(
      int width, int height, double scaleFactor) {
    // The window's new size in logical pixels, and its scale factor.
    session
        .getDiscarding()
        .resize(LogicalExtent.of(width, height, scaleFactor));
  }

  // #endregion resize

  // #region service
  // Your graphics context is current on this thread.
  private void serviceSession() {
    session.serviceDriverWork();
  }

  // Runs on a library thread.
  private void onDriverWorkWake() {
    runOnGraphicsThread(this::serviceSession);
  }

  // #endregion service

  private void surfaceLost() {
    // #region lost-surface
    session.releaseTarget().join();
    // #endregion lost-surface
  }

  private void surfaceRestored(
      NativePointer display, NativePointer newSurface) {
    // #region lost-surface
    session
        .setTarget(
            new RenderTarget.WaylandSurface(
                new WaylandSurfaceTarget(
                    LogicalSize.of(800, 600), display, newSurface)))
        .join();
    // #endregion lost-surface
  }

  private void renderToWindow(
      MapHandle map,
      NativePointer display,
      NativePointer windowSurface) {
    // #region attach
    // With no device, the session creates one on the platform's own
    // backend.
    session =
        map.attach(
            new RenderTarget.WaylandSurface(
                new WaylandSurfaceTarget(
                    // The window's size in logical pixels. The map's
                    // scale factor turns it into pixels.
                    LogicalSize.of(800, 600),
                    // Your wl_display and wl_surface.
                    display,
                    windowSurface)),
            null,
            RenderSessionAttachOptions.of(
                    RenderDriverKind.CORE_WORKER)
                .withRedrawWake(new Wake(this::onRedrawWake))
                .withFrameWake(new Wake(this::onFrameWake)));
    session.getReady().join();
    // #endregion attach

    onWindowResized(1024, 768, 2.0);

    // #region service
    RenderSessionAttachOptions graphicsThreadOptions =
        RenderSessionAttachOptions.of(
                RenderDriverKind.CALLER_GRAPHICS_THREAD)
            .withRedrawWake(new Wake(this::onRedrawWake))
            .withFrameWake(new Wake(this::onFrameWake))
            .withDriverWorkWake(new Wake(this::onDriverWorkWake));
    // #endregion service
    useOptions(graphicsThreadOptions);

    session.release().join();
    destroySurface(); // For example, with vkDestroySurfaceKHR.
  }
}
