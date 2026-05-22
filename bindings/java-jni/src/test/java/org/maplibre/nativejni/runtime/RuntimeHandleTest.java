package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.offline.OfflineRegionDefinition;
import org.maplibre.nativejni.offline.OfflineRegionDownloadState;
import org.maplibre.nativejni.test.NativeTestSupport;

class RuntimeHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void createRunOnceAndCloseRuntime() {
    var runtime = RuntimeHandle.create();

    assertFalse(runtime.isClosed());
    runtime.runOnce();
    runtime.close();
    assertTrue(runtime.isClosed());

    runtime.close();
    assertTrue(runtime.isClosed());
    assertThrows(InvalidStateException.class, runtime::runOnce);
  }

  @Test
  void startsAndDiscardsAmbientCacheOperation() {
    try (var runtime = RuntimeHandle.create()) {
      var operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.CLEAR);

      assertFalse(operation.isClosed());
      assertTrue(operation.id() != 0);
      assertTrue(operation.kind() == OfflineOperationKind.AMBIENT_CACHE);
      assertTrue(operation.resultKind() == OfflineOperationResultKind.NONE);

      runtime.discardOfflineOperation(operation);
      assertTrue(operation.isClosed());
      runtime.discardOfflineOperation(operation);
    }
  }

  @Test
  void startsAndDiscardsCreateOfflineRegionOperation() {
    try (var runtime = RuntimeHandle.create()) {
      var operation =
          runtime.startCreateOfflineRegion(
              new OfflineRegionDefinition.TilePyramid(
                  "https://example.com/style.json",
                  new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                  0.0,
                  1.0,
                  1.0f,
                  true),
              new byte[] {1, 2, 3});

      assertTrue(operation.kind() == OfflineOperationKind.REGION_CREATE);
      assertTrue(operation.resultKind() == OfflineOperationResultKind.REGION);
      runtime.discardOfflineOperation(operation);
    }
  }

  @Test
  void startsAndDiscardsOfflineRegionControlOperations() {
    try (var runtime = RuntimeHandle.create()) {
      var get = runtime.startOfflineRegion(123);
      assertTrue(get.kind() == OfflineOperationKind.REGION_GET);
      assertTrue(get.resultKind() == OfflineOperationResultKind.OPTIONAL_REGION);
      runtime.discardOfflineOperation(get);

      var list = runtime.startOfflineRegions();
      assertTrue(list.kind() == OfflineOperationKind.REGIONS_LIST);
      assertTrue(list.resultKind() == OfflineOperationResultKind.REGION_LIST);
      runtime.discardOfflineOperation(list);

      var status = runtime.startOfflineRegionStatus(123);
      assertTrue(status.kind() == OfflineOperationKind.REGION_GET_STATUS);
      assertTrue(status.resultKind() == OfflineOperationResultKind.REGION_STATUS);
      assertThrows(
          InvalidStateException.class, () -> runtime.takeOfflineRegionStatusResult(status));
      runtime.discardOfflineOperation(status);

      runtime.discardOfflineOperation(runtime.startSetOfflineRegionObserved(123, false));
      runtime.discardOfflineOperation(
          runtime.startSetOfflineRegionDownloadState(123, OfflineRegionDownloadState.INACTIVE));
      runtime.discardOfflineOperation(runtime.startInvalidateOfflineRegion(123));
      runtime.discardOfflineOperation(runtime.startDeleteOfflineRegion(123));
    }
  }

  @Test
  void pollEventReturnsEmptyWhenNativeQueueIsEmpty() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.runOnce();
      assertTrue(runtime.pollEvent().isEmpty());
    }
  }
}
