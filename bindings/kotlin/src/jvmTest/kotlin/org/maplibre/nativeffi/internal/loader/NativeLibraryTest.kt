package org.maplibre.nativeffi.internal.loader

import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.attribute.PosixFilePermissions
import kotlin.io.path.ExperimentalPathApi
import kotlin.io.path.createTempDirectory
import kotlin.io.path.deleteRecursively
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

@OptIn(ExperimentalPathApi::class)
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

  @Test
  fun processesShareOneOwnerOnlyExtractionRoot() {
    val temporaryDirectory = createTempDirectory("native-library-test")
    try {
      val root = assertNotNull(NativeLibrary.sharedExtractionRoot(temporaryDirectory))

      assertEquals(root, NativeLibrary.sharedExtractionRoot(temporaryDirectory))
      if (posix(temporaryDirectory)) {
        assertEquals(
          PosixFilePermissions.fromString("rwx------"),
          Files.getPosixFilePermissions(root),
        )
      }
    } finally {
      temporaryDirectory.deleteRecursively()
    }
  }

  @Test
  fun sharedExtractionRootRefusesADirectoryOtherUsersCanWrite() {
    val temporaryDirectory = createTempDirectory("native-library-test")
    // Windows temporary directories are already private to the user and carry no POSIX modes.
    if (!posix(temporaryDirectory)) {
      temporaryDirectory.deleteRecursively()
      return
    }
    try {
      val root = assertNotNull(NativeLibrary.sharedExtractionRoot(temporaryDirectory))
      Files.setPosixFilePermissions(root, PosixFilePermissions.fromString("rwxrwxrwx"))

      assertNull(NativeLibrary.sharedExtractionRoot(temporaryDirectory))
    } finally {
      temporaryDirectory.deleteRecursively()
    }
  }

  private fun posix(directory: Path): Boolean =
    Files.getFileStore(directory).supportsFileAttributeView("posix")
}
