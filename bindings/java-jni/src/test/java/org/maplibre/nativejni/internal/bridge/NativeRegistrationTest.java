package org.maplibre.nativejni.internal.bridge;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.test.NativeTestSupport;

final class NativeRegistrationTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void explicitlyRegisteredMethodsAreCallable() {
    assertTrue(BaseNative.mln_c_version() >= 0);
    assertEquals(MaplibreStatus.OK.nativeCode(), JniTestNative.createManyLocalStrings(1));
  }

  @Test
  void unregisteredNativeMethodReportsArtifactMismatch() {
    assertThrows(UnsatisfiedLinkError.class, JniTestNative::unregisteredNativeForTesting);
  }
}
