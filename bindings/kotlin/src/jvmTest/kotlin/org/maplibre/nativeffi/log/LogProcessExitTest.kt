package org.maplibre.nativeffi.log

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.system.exitProcess
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.LogEvent
import org.maplibre.nativeffi.LogHandler
import org.maplibre.nativeffi.LogSeverity
import org.maplibre.nativeffi.LogSeverityFlag
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.RuntimeOptions
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.libraryProperties
import org.maplibre.nativeffi.loadNativeLibrary
import org.maplibre.nativeffi.runChildJvm
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.setLogAsyncSeverityMask
import org.maplibre.nativeffi.setLogCallback
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.use

/**
 * A JVM that has received native log records exits with the callback installed, both after closing
 * its runtime and map and with them and their callbacks still live.
 */
class LogProcessExitTest {
  @Test fun jvmExitsWithNativeLogCallbackInstalled() = assertProcessExits("installed")

  @Test fun jvmExitsWithLiveHandlesAndCallbacks() = assertProcessExits("live")

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
    require(callbackState in setOf("installed", "live"))
    val received = CountDownLatch(1)
    loadNativeLibrary()
    setLogAsyncSeverityMask(LogSeverityFlag.WARNING)
    setLogCallback(
      LogHandler { severity, event, _, _ ->
        if (event == LogEvent.PARSE_STYLE && severity == LogSeverity.WARNING) {
          received.countDown()
        }
        true
      },
    )
    if (callbackState == "live") {
      // The runtime keeps its provider and wake, and the map a loaded style, as the process exits.
      runSuspendTest {
        val runtime = RuntimeHandle.create(RuntimeOptions())
        runtime.discarding.setResourceProvider(denyingProvider())
        val (map, ready) = runtime.createMap(smallMapOptions())
        ready.await()
        liveHandles = listOf(runtime, map)
        warnFromStyle(map, received)
      }
    } else {
      runSuspendTest {
        RuntimeHandle.create(RuntimeOptions()).use { runtime ->
          val (map, ready) = runtime.createMap(smallMapOptions())
          map.use {
            ready.await()
            warnFromStyle(it, received)
          }
        }
      }
    }
    println(READY_TO_EXIT)
    exitProcess(0)
  }
}

/** Loads a style whose invalid center emits a native parser warning, and waits for the record. */
private suspend fun warnFromStyle(map: MapHandle, received: CountDownLatch) {
  map.setStyleJson("""{"version":8,"center":false,"sources":{},"layers":[]}""").await()
  check(received.await(10, TimeUnit.SECONDS)) { "Native parser warning never reached Java" }
}

/** Keeps the live variant's handles reachable until the process exits. */
@Volatile private var liveHandles: Any? = null

private const val READY_TO_EXIT = "Native log callback observed; requesting JVM exit"
