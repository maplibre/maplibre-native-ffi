package org.maplibre.nativeffi

import java.nio.file.Files
import java.nio.file.Path
import java.util.concurrent.TimeUnit
import kotlin.reflect.KClass

/** What a child JVM printed, and how it exited, or null when it never exited. */
internal class ChildJvmResult(val output: String, val exitCode: Int?)

/**
 * Runs [main]'s `main` in a child JVM on the test classpath, with [properties] as system
 * properties, and waits for it to exit.
 */
internal fun runChildJvm(
  main: KClass<*>,
  arguments: List<String> = emptyList(),
  properties: Map<String, String> = emptyMap(),
): ChildJvmResult {
  val outputFile = Files.createTempFile("maplibre-child-jvm-", ".log")
  try {
    val javaExecutable =
      if (System.getProperty("os.name").startsWith("Windows")) "java.exe" else "java"
    val command =
      mutableListOf(
        Path.of(System.getProperty("java.home"), "bin", javaExecutable).toString(),
        "--enable-native-access=ALL-UNNAMED",
        "-cp",
        requireNotNull(System.getProperty("org.maplibre.nativeffi.test.classpath")),
      )
    properties.forEach { (name, value) -> command += "-D$name=$value" }
    command += main.java.name
    command += arguments
    val process =
      ProcessBuilder(command).redirectErrorStream(true).redirectOutput(outputFile.toFile()).start()
    try {
      val exited = process.waitFor(CHILD_TIMEOUT_SECONDS, TimeUnit.SECONDS)
      return ChildJvmResult(Files.readString(outputFile), if (exited) process.exitValue() else null)
    } finally {
      if (process.isAlive) {
        process.destroyForcibly()
        check(process.waitFor(CHILD_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
          "Could not stop child JVM ${process.pid()}"
        }
      }
    }
  } finally {
    Files.deleteIfExists(outputFile)
  }
}

/** The library system properties this JVM runs with, for a child that loads the same library. */
internal fun libraryProperties(): Map<String, String> =
  listOf("org.maplibre.nativeffi.library.path", "org.maplibre.nativeffi.library.dirs")
    .mapNotNull { name -> System.getProperty(name)?.let { name to it } }
    .toMap()

/** The tests/graphics system property this JVM runs with, for a child that renders. */
internal fun graphicsProperties(): Map<String, String> =
  listOf("org.maplibre.nativeffi.test.graphics.library")
    .mapNotNull { name -> System.getProperty(name)?.let { name to it } }
    .toMap()

private const val CHILD_TIMEOUT_SECONDS = 30L
