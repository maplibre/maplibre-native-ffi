package org.maplibre.nativejni.internal.strings;

import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
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
  void embeddedNulStringsAreRejectedBeforeNativeUse() {
    assertThrows(
        InvalidArgumentException.class,
        () -> RuntimeHandle.create(new RuntimeOptions().assetPath("asset\0path")));
  }
}
