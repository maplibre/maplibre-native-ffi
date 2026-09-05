package org.maplibre.nativeffi.log

import java.nio.file.Files
import java.nio.file.Path
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference
import kotlin.system.exitProcess
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class LogProcessExitTest {
  @Test fun jvmExitsWithNativeLogCallbackInstalled() = assertProcessExits("installed")

  @Test fun jvmExitsAfterClearingNativeLogCallback() = assertProcessExits("cleared")

  private fun assertProcessExits(callbackState: String) {
    val outputFile = Files.createTempFile("maplibre-log-exit-", ".log")
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
      for (property in
        listOf("org.maplibre.nativeffi.library.path", "org.maplibre.nativeffi.library.dirs")) {
        System.getProperty(property)?.let { command += "-D$property=$it" }
      }
      command += listOf(LogProcessExitProbe::class.java.name, callbackState)
      val process =
        ProcessBuilder(command)
          .redirectErrorStream(true)
          .redirectOutput(outputFile.toFile())
          .start()
      try {
        val exited = process.waitFor(20, TimeUnit.SECONDS)
        val output = Files.readString(outputFile)
        assertTrue(
          output.lineSequence().any { it == READY_TO_EXIT },
          "Child did not finish the asynchronous log callback and cleanup ($callbackState):\n$output",
        )
        assertTrue(exited, "JVM did not exit after native logging ($callbackState):\n$output")
        assertEquals(0, process.exitValue(), output)
      } finally {
        if (process.isAlive) {
          process.destroyForcibly()
          check(process.waitFor(10, TimeUnit.SECONDS)) {
            "Could not stop log test child ${process.pid()}"
          }
        }
      }
    } finally {
      Files.deleteIfExists(outputFile)
    }
  }
}

object LogProcessExitProbe {
  @JvmStatic
  fun main(args: Array<String>) {
    val callbackState = args.single()
    require(callbackState == "installed" || callbackState == "cleared")
    val caller = Thread.currentThread()
    val callbackThread = AtomicReference<Thread>()
    val received = CountDownLatch(1)
    Maplibre.loadNativeLibrary()
    Maplibre.setAsyncLogSeverities(setOf(LogSeverity.WARNING))
    Maplibre.setLogCallback(
      LogCallback { record ->
        if (record.event == LogEvent.PARSE_STYLE && record.severity == LogSeverity.WARNING) {
          callbackThread.set(Thread.currentThread())
          received.countDown()
        }
        true
      }
    )
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(runtime, MapOptions()).use { map ->
        // An invalid center emits a native parser warning without network or rendering work.
        map.setStyleJson(
          """{"version":8,"center":false,"sources":{},"layers":[]}""".encodeToByteArray()
        )
        check(received.await(10, TimeUnit.SECONDS)) { "Native parser warning never reached Java" }
        check(callbackThread.get() !== caller) { "Log callback ran synchronously" }
      }
    }
    if (callbackState == "cleared") {
      Maplibre.clearLogCallback()
    }
    println(READY_TO_EXIT)
    exitProcess(0)
  }
}

private const val READY_TO_EXIT = "Native log callback observed; requesting JVM exit"
