package org.maplibre.nativejni.internal.strings;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.file.Files;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.runtime.OfflineOperationKind;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.runtime.RuntimeOptions;
import org.maplibre.nativejni.test.NativeTestSupport;

final class StandardUtf8Test {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void standardUtf8StringsReachNativeRuntimeOptions() {
    try (var runtime = RuntimeHandle.create(new RuntimeOptions().assetPath("assets/é"))) {
      runtime.runOnce();
    }
  }

  @Test
  void standardUtf8PathStringsReachOfflineMergeOperation() throws IOException {
    var database = Files.createTempFile("offline-\uD83D\uDDFC", ".db");
    try (var runtime = RuntimeHandle.create()) {
      var operation = runtime.startMergeOfflineRegionsDatabase(database.toString());
      assertEquals(OfflineOperationKind.REGIONS_MERGE_DATABASE, operation.kind());
      runtime.discardOfflineOperation(operation);
    } finally {
      Files.deleteIfExists(database);
    }
  }

  @Test
  void embeddedNulStringsAreRejectedBeforeNativeUse() {
    assertThrows(
        InvalidArgumentException.class,
        () -> RuntimeHandle.create(new RuntimeOptions().assetPath("asset\0path")));
    try (var runtime = RuntimeHandle.create()) {
      assertThrows(
          InvalidArgumentException.class,
          () -> runtime.startMergeOfflineRegionsDatabase("offline\0database.db"));
    }
  }

  @Test
  void jniStringValidationDiagnosticDoesNotReusePreviousCFailure() {
    var nativeException =
        assertThrows(
            InvalidArgumentException.class,
            () -> Status.check(MaplibreNativeC.mln_network_status_set(999_999)));
    assertTrue(nativeException.diagnostic().contains("network status"));

    try (var runtime = RuntimeHandle.create()) {
      var jniException =
          assertThrows(
              InvalidArgumentException.class,
              () -> runtime.startMergeOfflineRegionsDatabase("offline\0database.db"));

      assertTrue(jniException.diagnostic().contains("embedded NUL"));
      assertFalse(jniException.diagnostic().contains("network status"));
    }
  }
}
