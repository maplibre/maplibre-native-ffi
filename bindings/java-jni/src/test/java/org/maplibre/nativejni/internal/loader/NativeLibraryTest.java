package org.maplibre.nativejni.internal.loader;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.test.NativeTestSupport;

final class NativeLibraryTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void exposesDocumentedLookupInputs() {
    assertFalse(NativeLibrary.LIBRARY_PATH_PROPERTY.isBlank());
    assertFalse(NativeLibrary.LIBRARY_PATH_ENV.isBlank());
    assertFalse(NativeLibrary.LIBRARY_NAME.isBlank());
  }

  @Test
  void loadedLibraryServesCAbiCalls() {
    assertTrue(Maplibre.cVersion() >= 0);
    NativeLibrary.ensureLoaded();
  }
}
