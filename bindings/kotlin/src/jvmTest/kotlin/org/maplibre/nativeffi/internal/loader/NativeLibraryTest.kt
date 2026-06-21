package org.maplibre.nativeffi.internal.loader

import java.nio.file.Path
import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue

class NativeLibraryTest {
  @Test
  fun explicitMissingPathReportsSourceAndPath() {
    val missingPath = Path.of("build", "missing-maplibre-native-c")

    if (NativeLibrary.isLoaded()) {
      NativeLibrary.load(missingPath)
      assertTrue(NativeLibrary.isLoaded())
      return
    }

    val error = assertFailsWith<UnsatisfiedLinkError> { NativeLibrary.load(missingPath) }

    assertTrue(error.message.orEmpty().contains("explicit path"))
    assertTrue(error.message.orEmpty().contains(missingPath.toString()))
    assertFalse(NativeLibrary.isLoaded())
  }
}
