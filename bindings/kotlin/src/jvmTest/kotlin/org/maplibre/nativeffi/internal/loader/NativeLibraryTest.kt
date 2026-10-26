package org.maplibre.nativeffi.internal.loader

import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.attribute.PosixFilePermissions
import kotlin.io.path.ExperimentalPathApi
import kotlin.io.path.createTempDirectory
import kotlin.io.path.deleteRecursively
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EXPECTED_C_ABI_VERSION
import org.maplibre.nativeffi.cVersion
import org.maplibre.nativeffi.loadNativeLibrary
import org.maplibre.nativeffi.runChildJvm

/** How the JVM binding finds the native library. */
@OptIn(ExperimentalPathApi::class)
class NativeLibraryTest {
  @Test
  fun theLibraryLoadsFromJavaLibraryPath() {
    loadNativeLibrary()
    assertEquals("java.library.path", NativeLibrary.loadedSource().get())
    assertEquals(EXPECTED_C_ABI_VERSION.toUInt(), cVersion())
  }

  @Test
  fun aLibraryPathWithNoLibraryFailsNamingTheWaysToLoadOne() {
    val empty = createTempDirectory("native-library-test")
    try {
      val child =
        runChildJvm(
          MissingLibraryProbe::class,
          properties = mapOf("java.library.path" to empty.toString()),
        )
      assertEquals(0, child.exitCode, child.output)
      assertTrue(child.output.contains("java.library.path"), child.output)
    } finally {
      empty.deleteRecursively()
    }
  }

  @Test
  fun anExactPathWithNoLibraryFailsNamingThePath() {
    val missing = Path.of("build", "missing-maplibre-native-c").toAbsolutePath().normalize()
    val child = runChildJvm(MissingLibraryProbe::class, arguments = listOf(missing.toString()))
    assertEquals(0, child.exitCode, child.output)
    assertTrue(child.output.contains(missing.toString()), child.output)
  }

  @Test
  fun processesShareOneOwnerOnlyExtractionRoot() {
    val temporaryDirectory = createTempDirectory("native-library-test")
    try {
      // Without POSIX modes, each process extracts to its own private directory instead.
      if (!posix(temporaryDirectory)) {
        assertNull(NativeLibrary.sharedExtractionRoot(temporaryDirectory))
        return
      }
      val root = assertNotNull(NativeLibrary.sharedExtractionRoot(temporaryDirectory))

      assertEquals(root, NativeLibrary.sharedExtractionRoot(temporaryDirectory))
      assertEquals(
        PosixFilePermissions.fromString("rwx------"),
        Files.getPosixFilePermissions(root),
      )
    } finally {
      temporaryDirectory.deleteRecursively()
    }
  }

  @Test
  fun sharedExtractionRootRefusesADirectoryOtherUsersCanWrite() {
    val temporaryDirectory = createTempDirectory("native-library-test")
    try {
      if (!posix(temporaryDirectory)) {
        return
      }
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

/**
 * Loads the library in a fresh JVM, from the exact path in its argument if it has one, and prints
 * the failure it expects.
 */
object MissingLibraryProbe {
  @JvmStatic
  fun main(args: Array<String>) {
    try {
      val path = args.singleOrNull()
      if (path != null) NativeLibrary.load(Path.of(path)) else NativeLibrary.load()
    } catch (error: UnsatisfiedLinkError) {
      println(error.message)
      return
    }
    error("the library loaded from a path that has none")
  }
}
