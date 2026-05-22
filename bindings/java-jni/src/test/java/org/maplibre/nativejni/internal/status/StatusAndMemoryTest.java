package org.maplibre.nativejni.internal.status;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.bridge.RuntimeNative;
import org.maplibre.nativejni.render.NativePointer;
import org.maplibre.nativejni.test.NativeTestSupport;

final class StatusAndMemoryTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void statusConversionCapturesDiagnostics() {
    var error =
        assertThrows(
            InvalidArgumentException.class,
            () -> Status.check(RuntimeNative.mln_network_status_set(999_999)));
    assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status());
    assertTrue(error.diagnostic().contains("network status"));
  }

  @Test
  void nativePointerRoundTripsAddressValues() {
    assertEquals(NativePointer.NULL, NativePointer.ofAddress(0));
    var pointer = NativePointer.ofAddress(0x1234);
    assertEquals(0x1234, pointer.address());
    assertEquals(pointer, NativePointer.ofAddress(0x1234));
  }
}
