package org.maplibre.nativejni.internal.bridge;

import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.test.NativeTestSupport;

final class NativeRegistrationTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void javaCppNativeBridgeServesCAbiCalls() {
    assertTrue(BaseNative.mln_c_version() >= 0);
  }
}
