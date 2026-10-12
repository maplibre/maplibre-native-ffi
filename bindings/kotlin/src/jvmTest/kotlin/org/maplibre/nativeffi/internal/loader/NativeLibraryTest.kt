package org.maplibre.nativeffi.internal.loader

import java.nio.file.Path
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.runChildJvm

/** How the JVM binding finds the native library. */
class NativeLibraryTest {
  @Test
  fun theLibraryLoadsFromTheConfiguredPath() {
    Maplibre.loadNativeLibrary()
    val configured = Path.of(System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY))
    assertEquals(configured.toAbsolutePath().normalize(), NativeLibrary.loadedPath().get())
    assertEquals(NativeLibrary.LIBRARY_PATH_PROPERTY, NativeLibrary.loadedSource().get())
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION.toUInt(), GeneratedApi.cVersion())
  }

  @Test
  fun aConfiguredPathWithNoLibraryFailsNamingItsSourceAndPath() {
    val missing = Path.of("build", "missing-maplibre-native-c").toAbsolutePath().normalize()
    val child =
      runChildJvm(
        MissingLibraryProbe::class,
        properties = mapOf(NativeLibrary.LIBRARY_PATH_PROPERTY to missing.toString()),
      )
    assertEquals(0, child.exitCode, child.output)
    assertTrue(child.output.contains(NativeLibrary.LIBRARY_PATH_PROPERTY), child.output)
    assertTrue(child.output.contains(missing.toString()), child.output)
  }
}

/** Loads the library in a fresh JVM, and prints the failure it expects. */
object MissingLibraryProbe {
  @JvmStatic
  fun main(args: Array<String>) {
    try {
      NativeLibrary.load()
    } catch (error: UnsatisfiedLinkError) {
      println(error.message)
      return
    }
    error("the library loaded from a path that has none")
  }
}
