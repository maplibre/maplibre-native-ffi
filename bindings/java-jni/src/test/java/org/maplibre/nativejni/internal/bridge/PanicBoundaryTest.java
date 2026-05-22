package org.maplibre.nativejni.internal.bridge;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.test.NativeTestSupport;

final class PanicBoundaryTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void rustPanicBecomesNativeErrorStatus() {
    var status = JniTestNative.panicStatus();

    assertEquals(MaplibreStatus.NATIVE_ERROR.nativeCode(), status);
    var exception = assertThrows(MaplibreException.class, () -> Status.check(status));
    assertEquals(MaplibreStatus.NATIVE_ERROR, exception.status());
  }
}
