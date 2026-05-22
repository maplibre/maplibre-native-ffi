package org.maplibre.nativejni.internal.refs;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.bridge.JniTestNative;
import org.maplibre.nativejni.test.NativeTestSupport;

final class LocalFrameTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void nativeLoopsUseBoundedLocalReferences() {
    assertEquals(MaplibreStatus.OK.nativeCode(), JniTestNative.createManyLocalStrings(10_000));
  }
}
