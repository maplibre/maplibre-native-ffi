package org.maplibre.nativeffi.render;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.util.EnumSet;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.error.UnsupportedFeatureException;
import org.maplibre.nativeffi.error.WrongThreadException;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.map.MapOptions;
import org.maplibre.nativeffi.runtime.RuntimeEventType;
import org.maplibre.nativeffi.runtime.RuntimeHandle;
import org.maplibre.nativeffi.test.NativeTestSupport;
import org.maplibre.nativeffi.test.RenderTargetTestSupport;

final class RenderSessionHandleTest {
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
    NativeTestSupport.loadNativeLibrary();
  }

  @AfterEach
  void restoreProcessState() {
    Maplibre.clearLogCallback();
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void ownedTextureSessionRendersReadsBackAndDetaches() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOwnedTextureTarget(map, new RenderTargetExtent(32, 16, 1.0))) {
      var activeSession = target.session();
      assertSame(map, activeSession.map());
      assertThrows(InvalidStateException.class, activeSession::textureImageInfo);
      assertThrows(InvalidStateException.class, map::close);

      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();

      var info = activeSession.textureImageInfo();
      assertEquals(32, info.width());
      assertEquals(16, info.height());
      assertEquals(32 * 4, info.stride());
      assertEquals((long) info.stride() * info.height(), info.byteLength());

      try (var small = NativeBuffer.allocate(4)) {
        assertThrows(
            InvalidArgumentException.class, () -> activeSession.readPremultipliedRgba8(small));
      }
      try (var buffer = NativeBuffer.allocate(info.byteLength())) {
        assertEquals(info, activeSession.readPremultipliedRgba8(buffer));
        assertEquals(info.byteLength(), buffer.toByteArray().length);
      }

      activeSession.reduceMemoryUse();
      activeSession.clearData();
      activeSession.dumpDebugLogs();
      activeSession.detach();
      assertThrows(InvalidStateException.class, activeSession::renderUpdate);
      activeSession.close();
      assertTrue(activeSession.isClosed());
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void metalOwnedTextureFrameHandleStaysActiveUntilClosed() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeMetalOwnedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();

      MetalOwnedTextureFrame frame;
      try (var frameHandle = activeSession.acquireMetalOwnedTextureFrame()) {
        frame = frameHandle.frame();
        assertEquals(32, frame.width());
        assertFalse(frameHandle.isClosed());
        assertThrows(InvalidStateException.class, activeSession::renderUpdate);
      }
      assertThrows(IllegalStateException.class, frame::width);
      activeSession.renderUpdate();
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void vulkanOwnedTextureFrameHandleStaysActiveUntilClosed() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeVulkanOwnedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();

      VulkanOwnedTextureFrame frame;
      try (var frameHandle = activeSession.acquireVulkanOwnedTextureFrame()) {
        frame = frameHandle.frame();
        assertEquals(32, frame.width());
        assertFalse(frameHandle.isClosed());
        assertThrows(InvalidStateException.class, activeSession::renderUpdate);
      }
      assertThrows(IllegalStateException.class, frame::width);
      activeSession.renderUpdate();
    } finally {
      map.close();
      runtime.close();
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
        assertNotNull(runOnOtherThread(frameHandle::close));
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
      try (var buffer = NativeBuffer.allocate(4)) {
        assertThrows(
            UnsupportedFeatureException.class, () -> activeSession.readPremultipliedRgba8(buffer));
      }
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
      try (var buffer = NativeBuffer.allocate(4)) {
        assertThrows(
            UnsupportedFeatureException.class, () -> activeSession.readPremultipliedRgba8(buffer));
      }
      assertTrue(hasNonZeroByte(target.readOpenGLSurfaceRgba(128, 128)));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void renderTargetDescriptorsTrackOptionalFields() {
    var vulkanBorrowed = new VulkanBorrowedTextureDescriptor();
    assertFalse(vulkanBorrowed.hasFinalLayout());
    assertNull(vulkanBorrowed.finalLayout());
    vulkanBorrowed.finalLayout(0);
    assertTrue(vulkanBorrowed.hasFinalLayout());
    assertEquals(0, vulkanBorrowed.finalLayout());
    vulkanBorrowed.clearFinalLayout();
    assertFalse(vulkanBorrowed.hasFinalLayout());

    var openglBorrowed = new OpenGLBorrowedTextureDescriptor().texture(12).target(0x0de1);
    assertEquals(12, openglBorrowed.texture());
    assertEquals(0x0de1, openglBorrowed.target());
  }

  @Test
  void scopedFrameAccessorsRejectEscapedNativePointers() {
    var scope = new FrameScope();
    var frame =
        new MetalOwnedTextureFrame(
            scope,
            1,
            2,
            3,
            1.0,
            4,
            NativePointer.scoped(0x10, scope),
            NativePointer.scoped(0x20, scope),
            80);
    var escaped = frame.texture();
    assertEquals(NativePointer.ofAddress(0x10), escaped);
    scope.close();
    assertThrows(IllegalStateException.class, escaped::address);
    assertThrows(IllegalStateException.class, frame::texture);
    assertThrows(IllegalStateException.class, frame::width);

    var openglFrame =
        new OpenGLOwnedTextureFrame(scope, 1, 2, 3, 1.0, 4, 5, 0x0de1, 0x8058, 0x1908, 0x1401);
    assertThrows(IllegalStateException.class, openglFrame::texture);
  }

  @Test
  void openglAttachMethodsReportUnsupportedWhenBackendUnavailable() {
    Assumptions.assumeFalse(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL native build exercises positive attach paths");

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    var context = fakeWglContext();
    try {
      assertThrows(
          UnsupportedFeatureException.class,
          () ->
              map.attachOpenGLOwnedTexture(
                  new OpenGLOwnedTextureDescriptor()
                      .extent(new RenderTargetExtent(32, 16, 1.0))
                      .context(context)));
      assertThrows(
          UnsupportedFeatureException.class,
          () ->
              map.attachOpenGLBorrowedTexture(
                  new OpenGLBorrowedTextureDescriptor()
                      .extent(new RenderTargetExtent(32, 16, 1.0))
                      .context(context)
                      .texture(1)
                      .target(0x0de1)));
      assertThrows(
          UnsupportedFeatureException.class,
          () ->
              map.attachOpenGLSurface(
                  new OpenGLSurfaceDescriptor()
                      .extent(new RenderTargetExtent(32, 16, 1.0))
                      .context(context)
                      .surface(NativePointer.ofAddress(1))));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void openglAttachMethodsReportNativeValidationErrors() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    var context = fakeSupportedOpenGLContext();
    try {
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
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void wrongThreadSessionCallAndCloseLeaveHandleLive() throws Exception {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOwnedTextureTarget(map, new RenderTargetExtent(64, 64, 1.0))) {
      var session = target.session();
      assertWrongThread(runOnOtherThread(session::renderUpdate));
      assertWrongThread(runOnOtherThread(session::close));
      assertFalse(session.isClosed());
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void nativeBufferOwnsOffHeapBytesUntilClosed() {
    var bytes = new byte[8];
    try (var buffer = NativeBuffer.allocate(bytes.length)) {
      assertEquals(bytes.length, buffer.byteLength());
      assertArrayEquals(bytes, buffer.toByteArray());
    }
    var closed = NativeBuffer.allocate(1);
    closed.close();
    assertThrows(IllegalStateException.class, closed::byteLength);
    assertEquals(0, NativePointer.NULL.address());
  }

  private static void assertWrongThread(Throwable thrown) {
    assertTrue(thrown instanceof WrongThreadException, () -> String.valueOf(thrown));
    var error = (WrongThreadException) thrown;
    assertEquals(MaplibreStatus.WRONG_THREAD, error.status());
    assertFalse(error.diagnostic().isBlank());
  }

  private static RenderTargetTestSupport assumeMetalOwnedTextureTarget(MapHandle map) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.METAL),
        "Metal owned texture unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachMetalOwnedTexture(
          map, new RenderTargetExtent(32, 16, 1.0));
    } catch (MaplibreException error) {
      return fail("Metal owned texture unavailable: " + error.getMessage(), error);
    }
  }

  private static RenderTargetTestSupport assumeOwnedTextureTarget(
      MapHandle map, RenderTargetExtent extent) {
    try {
      return RenderTargetTestSupport.attachOwnedTexture(map, extent);
    } catch (IllegalStateException error) {
      return fail("Owned texture target unavailable: " + error.getMessage(), error);
    }
  }

  private static RenderTargetTestSupport assumeVulkanOwnedTextureTarget(MapHandle map) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.VULKAN),
        "Vulkan owned texture unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachVulkanOwnedTexture(
          map, new RenderTargetExtent(32, 16, 1.0));
    } catch (MaplibreException error) {
      return fail("Vulkan owned texture unavailable: " + error.getMessage(), error);
    }
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
