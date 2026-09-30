package org.maplibre.nativeffi.log

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.system.exitProcess
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogEvent
import org.maplibre.nativeffi.generated.LogSeverity
import org.maplibre.nativeffi.generated.LogSeverityMask
import org.maplibre.nativeffi.libraryProperties
import org.maplibre.nativeffi.runChildJvm
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.use
import org.maplibre.nativeffi.smallMapOptions

/** A JVM that has received native log records exits, with or without the callback installed. */
class LogProcessExitTest {
  @Test fun jvmExitsWithNativeLogCallbackInstalled() = assertProcessExits("installed")

  @Test fun jvmExitsAfterClearingNativeLogCallback() = assertProcessExits("cleared")

  private fun assertProcessExits(callbackState: String) {
    val child = runChildJvm(LogProcessExitProbe::class, listOf(callbackState), libraryProperties())
    assertTrue(
      child.output.lineSequence().any { it == READY_TO_EXIT },
      "Child did not finish the asynchronous log callback and cleanup ($callbackState):\n" +
        child.output,
    )
    assertEquals(0, child.exitCode, "JVM did not exit cleanly ($callbackState):\n${child.output}")
  }
}

object LogProcessExitProbe {
  @JvmStatic
  fun main(args: Array<String>) {
    val callbackState = args.single()
    require(callbackState == "installed" || callbackState == "cleared")
    val received = CountDownLatch(1)
    Maplibre.loadNativeLibrary()
    GeneratedApi.logSetAsyncSeverityMask(LogSeverityMask.WARNING)
    GeneratedApi.logSetCallback({ severity, event, _, _ ->
      if (event == LogEvent.PARSE_STYLE && severity == LogSeverity.WARNING) {
        received.countDown()
      }
      1u
    })
    runSuspendTest {
      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
        runtime.mapCreate(smallMapOptions()).await().use { map ->
          // An invalid center emits a native parser warning without network or rendering work.
          map
            .setStyleJson(
              """{"version":8,"center":false,"sources":{},"layers":[]}""".encodeToByteArray()
            )
            .await()
          check(received.await(10, TimeUnit.SECONDS)) { "Native parser warning never reached Java" }
        }
      }
    }
    if (callbackState == "cleared") {
      GeneratedApi.logClearCallback()
    }
    println(READY_TO_EXIT)
    exitProcess(0)
  }
}

private const val READY_TO_EXIT = "Native log callback observed; requesting JVM exit"
