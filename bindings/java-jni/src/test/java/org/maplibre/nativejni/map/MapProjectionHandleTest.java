package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.test.NativeTestSupport;

class MapProjectionHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void projectionOwnsStandaloneNativeSnapshot() {
    try (var runtime = RuntimeHandle.create()) {
      var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
      var projection = map.createProjection();

      assertFalse(projection.isClosed());
      map.close();
      assertTrue(map.isClosed());

      projection.close();
      assertTrue(projection.isClosed());
      projection.close();
    }
  }
}
