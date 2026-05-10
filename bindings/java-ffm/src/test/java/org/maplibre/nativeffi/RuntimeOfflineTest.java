package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.file.Path;
import java.util.List;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.maplibre.nativeffi.internal.NativeTestSupport;

final class RuntimeOfflineTest {
  @TempDir Path temporaryDirectory;

  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @AfterEach
  void restoreProcessState() {
    MapLibre.clearLogCallback();
  }

  @Test
  void createsListsUpdatesAndDeletesOfflineRegions() {
    var runtime = runtime("offline-cache.db");
    try {
      var definition = tileDefinition();
      var created = runtime.createOfflineRegion(definition, new byte[] {1, 2, 3});
      assertTrue(created.id() > 0);
      assertEquals(definition, created.definition());
      assertArrayEquals(new byte[] {1, 2, 3}, created.metadata());

      assertEquals(created, runtime.offlineRegion(created.id()).orElseThrow());
      assertEquals(List.of(created), runtime.offlineRegions());

      var updated = runtime.updateOfflineRegionMetadata(created.id(), new byte[] {4, 5});
      assertEquals(created.id(), updated.id());
      assertArrayEquals(new byte[] {4, 5}, updated.metadata());

      var status = runtime.offlineRegionStatus(created.id());
      assertEquals(OfflineRegionDownloadState.INACTIVE, status.downloadState());
      runtime.setOfflineRegionObserved(created.id(), true);
      runtime.setOfflineRegionObserved(created.id(), false);
      runtime.setOfflineRegionDownloadState(created.id(), OfflineRegionDownloadState.INACTIVE);
      assertThrows(
          IllegalArgumentException.class,
          () ->
              runtime.setOfflineRegionDownloadState(
                  created.id(), OfflineRegionDownloadState.UNKNOWN));
      runtime.invalidateOfflineRegion(created.id());

      runtime.deleteOfflineRegion(created.id());
      assertFalse(runtime.offlineRegion(created.id()).isPresent());
      assertTrue(runtime.offlineRegions().isEmpty());
    } finally {
      runtime.close();
    }
  }

  @Test
  void mergeOfflineRegionsDatabaseCopiesMergedRegionList() {
    var sideCache = temporaryDirectory.resolve("side-offline-cache.db");
    var sideRuntime = RuntimeHandle.create(new RuntimeOptions().setCachePath(sideCache.toString()));
    try {
      sideRuntime.createOfflineRegion(tileDefinition(), new byte[] {5, 4, 3});
    } finally {
      sideRuntime.close();
    }

    var mainRuntime = runtime("main-offline-cache.db");
    try {
      var merged = mainRuntime.mergeOfflineRegionsDatabase(sideCache);
      assertEquals(1, merged.size());
      assertEquals(tileDefinition(), merged.getFirst().definition());
      assertArrayEquals(new byte[] {5, 4, 3}, merged.getFirst().metadata());
      assertEquals(merged, mainRuntime.offlineRegions());
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
      var created = runtime.createOfflineRegion(definition, new byte[] {});
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
              () -> runtime.createOfflineRegion(invalid, new byte[0]));
      assertEquals(MapLibreStatus.INVALID_ARGUMENT, exception.status());
      assertTrue(exception.diagnostic().contains("offline region bounds are invalid"));
    } finally {
      runtime.close();
    }
  }

  private RuntimeHandle runtime(String fileName) {
    return RuntimeHandle.create(
        new RuntimeOptions().setCachePath(temporaryDirectory.resolve(fileName).toString()));
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
