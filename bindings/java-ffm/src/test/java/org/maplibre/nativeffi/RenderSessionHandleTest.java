package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.util.EnumSet;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.NativeTestSupport;

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
    MapLibre.clearLogCallback();
    MapLibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void ownedTextureSessionRendersReadsBackAndDetaches() throws Exception {
    MapLibre.setLogCallback(record -> true);
    MapLibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(64, 64));
    RenderSessionHandle session = null;
    try {
      session =
          map.attachOwnedTexture(new OwnedTextureDescriptor().setSize(32, 16).setScaleFactor(1.0));
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
  void renderTargetDescriptorsValidateJavaOwnedState() {
    assertThrows(IllegalArgumentException.class, () -> new OwnedTextureDescriptor().setSize(0, 1));
    assertThrows(
        IllegalArgumentException.class, () -> new OwnedTextureDescriptor().setScaleFactor(0.0));
    assertThrows(
        IllegalArgumentException.class,
        () -> new MetalSurfaceDescriptor().setScaleFactor(Double.NaN));
    assertThrows(
        IllegalArgumentException.class,
        () -> new VulkanOwnedTextureDescriptor().setGraphicsQueueFamilyIndex(-1));
    assertThrows(
        IllegalArgumentException.class,
        () -> new VulkanBorrowedTextureDescriptor().setFinalLayout(0));
    var vulkanBorrowed = new VulkanBorrowedTextureDescriptor();
    assertFalse(vulkanBorrowed.hasFinalLayout());
    assertNull(vulkanBorrowed.finalLayout());
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
}
