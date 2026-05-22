package org.maplibre.nativejni.internal.callback;

import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.test.NativeTestSupport;

final class LogCallbackStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @AfterEach
  void clearCallback() {
    Maplibre.clearLogCallback();
  }

  @Test
  void installsAndClearsProcessGlobalCallback() {
    LogCallbackState.set(record -> true);
    LogCallbackState.clear();
  }

  @Test
  void rejectsNullCallback() {
    assertThrows(NullPointerException.class, () -> LogCallbackState.set(null));
  }
}
