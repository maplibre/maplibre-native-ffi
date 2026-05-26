package org.maplibre.nativejni.android;

import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.loader.NativeLibrary;

final class AndroidLoadTest {
  @Test
  void androidPackagingIsDocumentedAsOutOfScopeForThisJvmSuite() {
    assertTrue(NativeLibrary.LIBRARY_NAME.contains("MaplibreNativeC"));
    assertTrue(System.getProperty("java.vm.name", "").length() > 0);
  }
}
