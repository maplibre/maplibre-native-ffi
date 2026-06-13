package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.Optional;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.offline.OfflineRegionDefinition;
import org.maplibre.nativejni.offline.OfflineRegionDownloadState;
import org.maplibre.nativejni.resource.ResourceProviderDecision;

class RuntimeHandleTest {
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
  void createRuntimeUsesSuppliedOptions() {
    try (var runtime = RuntimeHandle.create(new RuntimeOptions().maximumCacheSize(42))) {
      runtime.runOnce();
    }
    assertThrows(
        InvalidArgumentException.class,
        () -> RuntimeHandle.create(new RuntimeOptions().assetPath("asset\0path")));
    assertThrows(
        InvalidArgumentException.class,
        () -> RuntimeHandle.create(new RuntimeOptions().maximumCacheSize(-1)));
  }

  @Test
  void startsAndDiscardsAmbientCacheOperation() {
    try (var runtime = RuntimeHandle.create()) {
      var operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.CLEAR);
      assertFalse(operation.isClosed());
      assertTrue(operation.id() != 0);
      assertTrue(operation.kind() == OfflineOperationKind.AMBIENT_CACHE);
      assertTrue(operation.resultKind() == OfflineOperationResultKind.NONE);
      operation.close();
      assertTrue(operation.isClosed());
      operation.close();
    }
  }

  @Test
  void discardAfterRuntimeCloseConsumesOperationAndRethrows() {
    var runtime = RuntimeHandle.create();
    var operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.CLEAR);
    runtime.close();
    assertThrows(InvalidStateException.class, operation::close);
    assertTrue(operation.isClosed());
    operation.close();
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
      try {
        runtime.takeCreateOfflineRegionResult(operation);
      } catch (InvalidStateException expectedIfStillRunning) {
        operation.close();
      }
      var geometryOperation =
          runtime.startCreateOfflineRegion(
              new OfflineRegionDefinition.GeometryRegion(
                  "https://example.com/style.json",
                  Geometry.point(new LatLng(0.5, 0.5)),
                  0.0,
                  1.0,
                  1.0f,
                  true),
              new byte[] {1, 2, 3});
      assertTrue(geometryOperation.kind() == OfflineOperationKind.REGION_CREATE);
      assertTrue(geometryOperation.resultKind() == OfflineOperationResultKind.REGION);
      geometryOperation.close();
    }
  }

  @Test
  void startsAndDiscardsOfflineRegionControlOperations() {
    try (var runtime = RuntimeHandle.create()) {
      var get = runtime.startOfflineRegion(123);
      assertTrue(get.kind() == OfflineOperationKind.REGION_GET);
      assertTrue(get.resultKind() == OfflineOperationResultKind.OPTIONAL_REGION);
      get.close();
      var list = runtime.startOfflineRegions();
      assertTrue(list.kind() == OfflineOperationKind.REGIONS_LIST);
      assertTrue(list.resultKind() == OfflineOperationResultKind.REGION_LIST);
      list.close();
      var update = runtime.startUpdateOfflineRegionMetadata(123, new byte[] {4, 5, 6});
      assertTrue(update.kind() == OfflineOperationKind.REGION_UPDATE_METADATA);
      assertTrue(update.resultKind() == OfflineOperationResultKind.REGION);
      assertThrows(
          InvalidStateException.class, () -> runtime.takeUpdateOfflineRegionMetadataResult(update));
      update.close();
      var status = runtime.startOfflineRegionStatus(123);
      assertTrue(status.kind() == OfflineOperationKind.REGION_GET_STATUS);
      assertTrue(status.resultKind() == OfflineOperationResultKind.REGION_STATUS);
      assertThrows(
          InvalidStateException.class, () -> runtime.takeOfflineRegionStatusResult(status));
      status.close();
      runtime.startSetOfflineRegionObserved(123, false).close();
      runtime.startSetOfflineRegionDownloadState(123, OfflineRegionDownloadState.INACTIVE).close();
      runtime.startInvalidateOfflineRegion(123).close();
      runtime.startDeleteOfflineRegion(123).close();
    }
  }

  @Test
  void resourceProviderLifecycleCrossesNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
    }
  }

  @Test
  void resourceTransformLifecycleCrossesNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceTransform(request -> Optional.empty());
      runtime.setResourceTransform(request -> Optional.of(request.url() + "?rewritten=true"));
      runtime.clearResourceTransform();
      runtime.clearResourceTransform();
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
