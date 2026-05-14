package org.maplibre.nativeffi.render;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
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
import org.maplibre.nativeffi.error.WrongThreadException;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.map.MapOptions;
import org.maplibre.nativeffi.runtime.RuntimeEventType;
import org.maplibre.nativeffi.runtime.RuntimeHandle;
import org.maplibre.nativeffi.test.NativeTestSupport;

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
    RenderSessionHandle session = null;
    try {
      session =
          map.attachOwnedTexture(
              new OwnedTextureDescriptor().extent(new RenderTargetExtent(32, 16, 1.0)));
      var activeSession = session;
      assertSame(map, activeSession.map());
      assertThrows(InvalidStateException.class, activeSession::textureImageInfo);
      assertThrows(InvalidStateException.class, map::close);

      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      session.renderUpdate();

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

      var image = activeSession.readPremultipliedRgba8();
      assertEquals(info.width(), image.width());
      assertEquals(info.height(), image.height());
      assertEquals(info.stride(), image.stride());
      assertEquals(info.byteLength(), image.pixels().length);

      activeSession.reduceMemoryUse();
      activeSession.clearData();
      activeSession.dumpDebugLogs();
      activeSession.detach();
      assertThrows(InvalidStateException.class, activeSession::renderUpdate);
      activeSession.close();
      assertTrue(activeSession.isClosed());
      session = null;
    } finally {
      if (session != null) {
        session.close();
      }
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
    RenderSessionHandle session = null;
    try {
      session = assumeMetalOwnedTextureSession(map);
      var activeSession = session;
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
      if (session != null) {
        session.close();
      }
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
    RenderSessionHandle session = null;
    try {
      session = assumeVulkanOwnedTextureSession(map);
      var activeSession = session;
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
      if (session != null) {
        session.close();
      }
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
  }

  @Test
  void wrongThreadSessionCallAndCloseLeaveHandleLive() throws Exception {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    var session =
        map.attachOwnedTexture(
            new OwnedTextureDescriptor().extent(new RenderTargetExtent(64, 64, 1.0)));
    try {
      assertWrongThread(runOnOtherThread(session::renderUpdate));
      assertWrongThread(runOnOtherThread(session::close));
      assertFalse(session.isClosed());
    } finally {
      session.close();
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

  private static RenderSessionHandle assumeMetalOwnedTextureSession(MapHandle map) {
    try {
      return map.attachMetalOwnedTexture(
          new MetalOwnedTextureDescriptor().extent(new RenderTargetExtent(32, 16, 1.0)));
    } catch (MaplibreException error) {
      Assumptions.assumeTrue(false, "Metal owned texture unavailable: " + error.getMessage());
      throw new AssertionError("unreachable");
    }
  }

  private static RenderSessionHandle assumeVulkanOwnedTextureSession(MapHandle map) {
    try {
      return map.attachVulkanOwnedTexture(
          new VulkanOwnedTextureDescriptor().extent(new RenderTargetExtent(32, 16, 1.0)));
    } catch (MaplibreException error) {
      Assumptions.assumeTrue(false, "Vulkan owned texture unavailable: " + error.getMessage());
      throw new AssertionError("unreachable");
    }
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
