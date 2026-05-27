package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.util.EnumSet;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.error.UnsupportedFeatureException;
import org.maplibre.nativejni.error.WrongThreadException;
import org.maplibre.nativejni.log.LogSeverity;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.runtime.RuntimeEventType;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.test.NativeTestSupport;
import org.maplibre.nativejni.test.RenderTargetTestSupport;

class RenderSessionHandleTest {
  private static final String STYLE_JSON =
      """
      {
        "version": 8,
        "sources": {},
        "layers": [
          {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
        ]
      }
      """;

  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @AfterEach
  void restoreProcessState() {
    Maplibre.clearLogCallback();
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void rejectsNegativeAttachDimensionsBeforeNativeCast() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        var descriptor =
            new MetalOwnedTextureDescriptor().extent(new RenderTargetExtent(-1, 64, 1.0));
        assertThrows(InvalidArgumentException.class, () -> map.attachMetalOwnedTexture(descriptor));
        if (Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL)) {
          var openGLDescriptor =
              new OpenGLOwnedTextureDescriptor()
                  .extent(new RenderTargetExtent(-1, 64, 1.0))
                  .context(fakeSupportedOpenGLContext());
          assertThrows(
              InvalidArgumentException.class, () -> map.attachOpenGLOwnedTexture(openGLDescriptor));
        }
      }
    }
  }

  @Test
  void openGLAttachMethodsReportUnsupportedWhenBackendUnavailable() {
    Assumptions.assumeFalse(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL native build exercises positive attach paths");
    var context = fakeWglContext();
    var extent = new RenderTargetExtent(64, 64, 1.0);
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            UnsupportedFeatureException.class,
            () ->
                map.attachOpenGLOwnedTexture(
                    new OpenGLOwnedTextureDescriptor().extent(extent).context(context)));
        assertThrows(
            UnsupportedFeatureException.class,
            () ->
                map.attachOpenGLBorrowedTexture(
                    new OpenGLBorrowedTextureDescriptor()
                        .extent(extent)
                        .context(context)
                        .texture(12)
                        .target(0x0de1)));
        assertThrows(
            UnsupportedFeatureException.class,
            () ->
                map.attachOpenGLSurface(
                    new OpenGLSurfaceDescriptor()
                        .extent(extent)
                        .context(context)
                        .surface(NativePointer.ofAddress(3))));
      }
    }
  }

  @Test
  void openglAttachMethodsReportNativeValidationErrors() {
    var context = fakeSupportedOpenGLContext();
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.attachOpenGLOwnedTexture(
                    new OpenGLOwnedTextureDescriptor()
                        .extent(new RenderTargetExtent(0, 16, 1.0))
                        .context(context)));
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.attachOpenGLBorrowedTexture(
                    new OpenGLBorrowedTextureDescriptor()
                        .extent(new RenderTargetExtent(32, 16, 1.0))
                        .context(context)
                        .texture(0)
                        .target(0x0de1)));
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.attachOpenGLSurface(
                    new OpenGLSurfaceDescriptor()
                        .extent(new RenderTargetExtent(32, 16, 1.0))
                        .context(context)
                        .surface(NativePointer.NULL)));
      }
    }
  }

  @Test
  void attachAttemptsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            MaplibreException.class,
            () -> map.attachMetalOwnedTexture(new MetalOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachMetalBorrowedTexture(new MetalBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachVulkanOwnedTexture(new VulkanOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachVulkanBorrowedTexture(new VulkanBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachOpenGLOwnedTexture(new OpenGLOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachOpenGLBorrowedTexture(new OpenGLBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachMetalSurface(new MetalSurfaceDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachVulkanSurface(new VulkanSurfaceDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachOpenGLSurface(new OpenGLSurfaceDescriptor()));
      }
    }
  }

  @Test
  void openglOwnedTextureFrameHandleStaysActiveUntilClosed() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();

      OpenGLOwnedTextureFrame frame;
      try (var frameHandle = activeSession.acquireOpenGLOwnedTextureFrame()) {
        frame = frameHandle.frame();
        assertEquals(32, frame.width());
        assertEquals(0x0de1, frame.target());
        assertEquals(0x8058, frame.internalFormat());
        assertEquals(0x1908, frame.format());
        assertEquals(0x1401, frame.type());
        assertTrue(frame.texture() != 0);
        assertFalse(frameHandle.isClosed());
        assertThrows(InvalidStateException.class, activeSession::renderUpdate);
      }
      assertThrows(IllegalStateException.class, frame::texture);
      activeSession.renderUpdate();
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void openglOwnedTextureFrameCloseFailureLeavesHandleRetryable() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();

      var frameHandle = activeSession.acquireOpenGLOwnedTextureFrame();
      var frame = frameHandle.frame();
      try {
        assertWrongThread(runOnOtherThread(frameHandle::close));
        assertFalse(frameHandle.isClosed());
        assertTrue(frame.texture() != 0);
        assertThrows(InvalidStateException.class, activeSession::renderUpdate);
      } finally {
        frameHandle.close();
      }
      assertTrue(frameHandle.isClosed());
      assertThrows(IllegalStateException.class, frame::texture);
      activeSession.renderUpdate();
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void openglBorrowedTextureSessionRendersThroughPublicBinding() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try (var target = assumeOpenGLBorrowedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();

      assertThrows(
          UnsupportedFeatureException.class, activeSession::acquireOpenGLOwnedTextureFrame);
      assertThrows(UnsupportedFeatureException.class, activeSession::textureImageInfo);
      assertThrows(UnsupportedFeatureException.class, activeSession::readPremultipliedRgba8);
      assertThrows(UnsupportedFeatureException.class, () -> activeSession.resize(128, 128, 1.0));
      assertTrue(hasNonZeroByte(target.readOpenGLBorrowedTextureRgba()));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void openglSurfaceSessionRendersThroughPublicBinding() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try (var target = assumeOpenGLSurfaceTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();

      assertThrows(
          UnsupportedFeatureException.class, activeSession::acquireOpenGLOwnedTextureFrame);
      assertThrows(UnsupportedFeatureException.class, activeSession::textureImageInfo);
      assertThrows(UnsupportedFeatureException.class, activeSession::readPremultipliedRgba8);
      assertTrue(hasNonZeroByte(target.readOpenGLSurfaceRgba(128, 128)));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void openGLDescriptorAccessorsRoundTrip() {
    var extent = new RenderTargetExtent(32, 16, 2.0);
    var eglContext =
        new EglContextDescriptor(
                NativePointer.ofAddress(0x10),
                NativePointer.ofAddress(0x20),
                NativePointer.ofAddress(0x30))
            .getProcAddress(NativePointer.ofAddress(0x40));
    var wglContext =
        new WglContextDescriptor(NativePointer.ofAddress(0x50), NativePointer.ofAddress(0x60))
            .getProcAddress(NativePointer.ofAddress(0x70));

    var owned = new OpenGLOwnedTextureDescriptor().extent(extent).context(eglContext);
    assertEquals(extent, owned.extent());
    assertEquals(eglContext, owned.context());

    var borrowed =
        new OpenGLBorrowedTextureDescriptor()
            .extent(extent)
            .context(wglContext)
            .texture(12)
            .target(0x0de1);
    assertEquals(extent, borrowed.extent());
    assertEquals(wglContext, borrowed.context());
    assertEquals(12, borrowed.texture());
    assertEquals(0x0de1, borrowed.target());

    var surface =
        new OpenGLSurfaceDescriptor()
            .extent(extent)
            .context(wglContext)
            .surface(NativePointer.ofAddress(0x80));
    assertEquals(extent, surface.extent());
    assertEquals(wglContext, surface.context());
    assertEquals(NativePointer.ofAddress(0x80), surface.surface());
  }

  @Test
  void openGLOwnedTextureFrameValuesExpireWithScope() {
    var scope = new FrameScope();
    var frame =
        new OpenGLOwnedTextureFrame(scope, 1, 2, 3, 1.0, 4, 5, 0x0de1, 0x8058, 0x1908, 0x1401);

    assertEquals(5, frame.texture());
    scope.close();
    assertThrows(IllegalStateException.class, frame::texture);
  }

  @Test
  void supportedOpenGLContextProviderMaskMapsFlags() {
    assertTrue(OpenGLContextProvider.fromMask(0).isEmpty());
    assertEquals(
        java.util.EnumSet.of(OpenGLContextProvider.WGL), OpenGLContextProvider.fromMask(1));
    assertEquals(
        java.util.EnumSet.of(OpenGLContextProvider.WGL, OpenGLContextProvider.EGL),
        OpenGLContextProvider.fromMask(3));
  }

  private static RenderTargetTestSupport assumeOpenGLOwnedTextureTarget(MapHandle map) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL owned texture unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachOpenGLOwnedTexture(
          map, new RenderTargetExtent(32, 16, 1.0));
    } catch (MaplibreException | IllegalStateException error) {
      return fail("OpenGL owned texture unavailable: " + error.getMessage(), error);
    }
  }

  private static RenderTargetTestSupport assumeOpenGLBorrowedTextureTarget(MapHandle map) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL borrowed texture unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachOpenGLBorrowedTexture(
          map, new RenderTargetExtent(128, 128, 1.0));
    } catch (MaplibreException | IllegalStateException error) {
      return fail("OpenGL borrowed texture unavailable: " + error.getMessage(), error);
    }
  }

  private static RenderTargetTestSupport assumeOpenGLSurfaceTarget(MapHandle map) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL surface unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachOpenGLSurface(
          map, new RenderTargetExtent(128, 128, 1.0));
    } catch (MaplibreException | IllegalStateException error) {
      return fail("OpenGL surface unavailable: " + error.getMessage(), error);
    }
  }

  private static WglContextDescriptor fakeWglContext() {
    var fake = NativePointer.ofAddress(1);
    return new WglContextDescriptor(fake, fake);
  }

  private static OpenGLContextDescriptor fakeSupportedOpenGLContext() {
    var fake = NativePointer.ofAddress(1);
    var providers = Maplibre.supportedOpenGLContextProviders();
    if (providers.contains(OpenGLContextProvider.EGL)) {
      return new EglContextDescriptor(fake, fake, fake);
    }
    if (providers.contains(OpenGLContextProvider.WGL)) {
      return new WglContextDescriptor(fake, fake);
    }
    return fakeWglContext();
  }

  private static boolean hasNonZeroByte(byte[] bytes) {
    for (var value : bytes) {
      if (value != 0) {
        return true;
      }
    }
    return false;
  }

  private static void assertWrongThread(Throwable thrown) {
    assertTrue(thrown instanceof WrongThreadException, () -> String.valueOf(thrown));
    var error = (WrongThreadException) thrown;
    assertEquals(MaplibreStatus.WRONG_THREAD, error.status());
    assertFalse(error.diagnostic().isBlank());
  }

  private static Throwable runOnOtherThread(ThrowingRunnable action) throws InterruptedException {
    var thrown = new AtomicReference<Throwable>();
    var thread =
        new Thread(
            () -> {
              try {
                action.run();
              } catch (Throwable error) {
                thrown.set(error);
              }
            });
    thread.start();
    thread.join();
    return thrown.get();
  }

  private static void waitForMapEvent(RuntimeHandle runtime, MapHandle map, RuntimeEventType type)
      throws InterruptedException {
    for (var attempt = 0; attempt < 1000; attempt++) {
      runtime.runOnce();
      while (true) {
        var event = runtime.pollEvent();
        if (event.isEmpty()) {
          break;
        }
        var value = event.get();
        if (value.type() == type && value.mapSource().filter(source -> source == map).isPresent()) {
          return;
        }
      }
      Thread.sleep(1);
    }
    fail("Timed out waiting for " + type);
  }

  @FunctionalInterface
  private interface ThrowingRunnable {
    void run() throws Exception;
  }
}
