package org.maplibre.nativeffi.runtime;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.geo.Geometry;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.LatLngBounds;
import org.maplibre.nativeffi.offline.OfflineRegionDefinition;
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState;
import org.maplibre.nativeffi.offline.OfflineRegionInfo;
import org.maplibre.nativeffi.offline.OfflineRegionStatus;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class RuntimeOfflineTest {
  @TempDir Path temporaryDirectory;

  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @AfterEach
  void restoreProcessState() {
    Maplibre.clearLogCallback();
  }

  @Test
  void createsListsUpdatesAndDeletesOfflineRegions() {
    var runtime = runtime("offline-cache.db");
    try {
      var definition = tileDefinition();
      var created = createOfflineRegion(runtime, definition, new byte[] {1, 2, 3});
      assertTrue(created.id() > 0);
      assertEquals(definition, created.definition());
      assertArrayEquals(new byte[] {1, 2, 3}, created.metadata());

      assertEquals(created, offlineRegion(runtime, created.id()).orElseThrow());
      assertEquals(List.of(created), offlineRegions(runtime));

      var updated = updateOfflineRegionMetadata(runtime, created.id(), new byte[] {4, 5});
      assertEquals(created.id(), updated.id());
      assertArrayEquals(new byte[] {4, 5}, updated.metadata());

      var status = offlineRegionStatus(runtime, created.id());
      assertEquals(OfflineRegionDownloadState.INACTIVE, status.downloadState());
      completeVoidOperation(runtime, runtime.startSetOfflineRegionObserved(created.id(), true));
      completeVoidOperation(runtime, runtime.startSetOfflineRegionObserved(created.id(), false));
      completeVoidOperation(
          runtime,
          runtime.startSetOfflineRegionDownloadState(
              created.id(), OfflineRegionDownloadState.INACTIVE));
      var unknownStateError =
          assertThrows(
              InvalidArgumentException.class,
              () ->
                  runtime.startSetOfflineRegionDownloadState(
                      created.id(), OfflineRegionDownloadState.UNKNOWN));
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, unknownStateError.status());
      completeVoidOperation(runtime, runtime.startInvalidateOfflineRegion(created.id()));

      completeVoidOperation(runtime, runtime.startDeleteOfflineRegion(created.id()));
      assertFalse(offlineRegion(runtime, created.id()).isPresent());
      assertTrue(offlineRegions(runtime).isEmpty());
    } finally {
      runtime.close();
    }
  }

  @Test
  void mergeOfflineRegionsDatabaseCopiesMergedRegionList() {
    var sideCache = temporaryDirectory.resolve("side-offline-cache.db");
    var sideRuntime = RuntimeHandle.create(new RuntimeOptions().cachePath(sideCache.toString()));
    try {
      createOfflineRegion(sideRuntime, tileDefinition(), new byte[] {5, 4, 3});
    } finally {
      sideRuntime.close();
    }

    var mainRuntime = runtime("main-offline-cache.db");
    try {
      var merged = mergeOfflineRegionsDatabase(mainRuntime, sideCache);
      assertEquals(1, merged.size());
      assertEquals(tileDefinition(), merged.getFirst().definition());
      assertArrayEquals(new byte[] {5, 4, 3}, merged.getFirst().metadata());
      assertEquals(merged, offlineRegions(mainRuntime));
    } finally {
      mainRuntime.close();
    }
  }

  @Test
  void geometryRegionDefinitionsRoundTripThroughSnapshots() {
    var runtime = runtime("geometry-offline-cache.db");
    try {
      var definition =
          new OfflineRegionDefinition.GeometryRegion(
              "asset://geometry-style.json",
              Geometry.point(new LatLng(37.7749, -122.4194)),
              0.0,
              12.0,
              2.0f,
              false);
      var created = createOfflineRegion(runtime, definition, new byte[] {});
      var roundTripped =
          assertInstanceOf(OfflineRegionDefinition.GeometryRegion.class, created.definition());
      assertEquals(definition.styleUrl(), roundTripped.styleUrl());
      assertEquals(definition.geometry(), roundTripped.geometry());
      assertArrayEquals(new byte[] {}, created.metadata());
    } finally {
      runtime.close();
    }
  }

  @Test
  void offlineRegionValidationPreservesNativeStatusDiagnostics() {
    var runtime = runtime("invalid-offline-cache.db");
    try {
      var invalid =
          new OfflineRegionDefinition.TilePyramid(
              "asset://style.json",
              new LatLngBounds(new LatLng(10.0, 0.0), new LatLng(0.0, 1.0)),
              0.0,
              1.0,
              1.0f,
              true);
      var exception =
          assertThrows(
              InvalidArgumentException.class,
              () -> runtime.startCreateOfflineRegion(invalid, new byte[0]));
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status());
      assertTrue(exception.diagnostic().contains("offline region bounds are invalid"));
    } finally {
      runtime.close();
    }
  }

  private static RuntimeEventPayload.OfflineOperationCompleted waitForOperation(
      RuntimeHandle runtime, OfflineOperationHandle<?> operation) {
    for (var attempt = 0; attempt < 1_000; attempt++) {
      runtime.runOnce();
      Optional<RuntimeEvent> event;
      while ((event = runtime.pollEvent()).isPresent()) {
        if (!(event.get().payload()
            instanceof RuntimeEventPayload.OfflineOperationCompleted completed)) {
          continue;
        }
        if (completed.operationId() != operation.id()) {
          continue;
        }
        assertEquals(operation.kind(), completed.operationKind());
        assertEquals(operation.kind().nativeValue(), completed.rawOperationKind());
        assertEquals(operation.resultKind(), completed.resultKind());
        assertEquals(operation.resultKind().nativeValue(), completed.rawResultKind());
        assertEquals(MaplibreStatus.OK.nativeCode(), completed.resultStatus());
        return completed;
      }
      try {
        Thread.sleep(1);
      } catch (InterruptedException exception) {
        Thread.currentThread().interrupt();
        throw new AssertionError(exception);
      }
    }
    throw new AssertionError("offline operation did not complete: " + operation.id());
  }

  private static void completeVoidOperation(
      RuntimeHandle runtime, OfflineOperationHandle<Void> operation) {
    waitForOperation(runtime, operation);
    operation.close();
  }

  private static OfflineRegionInfo createOfflineRegion(
      RuntimeHandle runtime, OfflineRegionDefinition definition, byte[] metadata) {
    var operation = runtime.startCreateOfflineRegion(definition, metadata);
    waitForOperation(runtime, operation);
    return runtime.takeCreateOfflineRegionResult(operation);
  }

  private static Optional<OfflineRegionInfo> offlineRegion(RuntimeHandle runtime, long id) {
    var operation = runtime.startOfflineRegion(id);
    waitForOperation(runtime, operation);
    return runtime.takeOfflineRegionResult(operation);
  }

  private static List<OfflineRegionInfo> offlineRegions(RuntimeHandle runtime) {
    var operation = runtime.startOfflineRegions();
    waitForOperation(runtime, operation);
    return runtime.takeOfflineRegionsResult(operation);
  }

  private static List<OfflineRegionInfo> mergeOfflineRegionsDatabase(
      RuntimeHandle runtime, Path path) {
    var operation = runtime.startMergeOfflineRegionsDatabase(path);
    waitForOperation(runtime, operation);
    return runtime.takeMergeOfflineRegionsDatabaseResult(operation);
  }

  private static OfflineRegionInfo updateOfflineRegionMetadata(
      RuntimeHandle runtime, long id, byte[] metadata) {
    var operation = runtime.startUpdateOfflineRegionMetadata(id, metadata);
    waitForOperation(runtime, operation);
    return runtime.takeUpdateOfflineRegionMetadataResult(operation);
  }

  private static OfflineRegionStatus offlineRegionStatus(RuntimeHandle runtime, long id) {
    var operation = runtime.startOfflineRegionStatus(id);
    waitForOperation(runtime, operation);
    return runtime.takeOfflineRegionStatusResult(operation);
  }

  private RuntimeHandle runtime(String fileName) {
    return RuntimeHandle.create(
        new RuntimeOptions().cachePath(temporaryDirectory.resolve(fileName).toString()));
  }

  private static OfflineRegionDefinition.TilePyramid tileDefinition() {
    return new OfflineRegionDefinition.TilePyramid(
        "asset://style.json",
        new LatLngBounds(new LatLng(0.0, 0.0), new LatLng(1.0, 1.0)),
        0.0,
        1.0,
        1.0f,
        true);
  }
}
