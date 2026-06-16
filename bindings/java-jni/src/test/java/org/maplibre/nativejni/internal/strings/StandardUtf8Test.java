package org.maplibre.nativejni.internal.strings;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.file.Files;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.runtime.RuntimeOptions;

// Support invariant for BND-024, BND-025, and BND-063: Java JNI uses standard
// UTF-8 conversion for language strings and rejects embedded NUL for C strings.
final class StandardUtf8Test {
  @Test
  void bnd063StandardUtf8StringsReachNativeRuntimeOptions() {
    try (var runtime = RuntimeHandle.create(new RuntimeOptions().assetPath("assets/é"))) {
      runtime.runOnce();
    }
  }

  @Test
  void bnd063StandardUtf8PathStringsReachOfflineMergeOperation() throws IOException {
    var database = Files.createTempFile("offline-\uD83D\uDDFC", ".db");
    try (var runtime = RuntimeHandle.create()) {
      var operation = runtime.startMergeOfflineRegionsDatabase(database.toString());
      operation.close();
    } finally {
      Files.deleteIfExists(database);
    }
  }

  @Test
  void bnd024EmbeddedNulStringsAreRejectedBeforeNativeUse() {
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
  void bnd025JniStringValidationDiagnosticDoesNotReusePreviousCFailure() {
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
