package org.maplibre.nativeffi.lifecycle

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.system.exitProcess
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Completion
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.FrameDemand
import org.maplibre.nativeffi.FrameDemandFlag
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.libraryProperties
import org.maplibre.nativeffi.loadNativeLibrary
import org.maplibre.nativeffi.render.withOffscreenSession
import org.maplibre.nativeffi.runChildJvm
import org.maplibre.nativeffi.runSuspendTest

/**
 * A JVM that exits with a render session still attached ends the session's graphics calls first,
 * and still exits when the exit starts inside a MapLibre callback.
 */
class RenderSessionExitTest {
  @Test
  fun jvmExitAbandonsALiveRenderSession() {
    val child = runRenderingChild("live")
    // Only abandon resolves the parked demand, and it wakes the frame receiver as it does.
    assertTrue(
      child.output.lineSequence().any { it == ABANDONED_AT_EXIT },
      "The shutdown hook did not abandon the live session:\n${child.output}",
    )
    assertEquals(0, child.exitCode, "JVM did not exit cleanly:\n${child.output}")
  }

  @Test
  fun jvmExitFromARenderSessionCompletionDoesNotHang() {
    val child = runRenderingChild("callback")
    assertTrue(
      child.output.lineSequence().any { it == EXITING_FROM_CALLBACK },
      "The completion never requested exit:\n${child.output}",
    )
    assertEquals(0, child.exitCode, "JVM did not exit from the callback:\n${child.output}")
  }

  private fun runRenderingChild(scenario: String) =
    runChildJvm(RenderSessionExitProbe::class, listOf(scenario), libraryProperties())
}

object RenderSessionExitProbe {
  @Volatile private var exiting = false
  private val resolvedAtExit = CountDownLatch(1)

  @JvmStatic
  fun main(args: Array<String>) {
    loadNativeLibrary()
    when (args.single()) {
      "live" -> exitWithParkedDemand()
      "callback" -> exitFromCompletion()
      else -> error("unknown scenario ${args.single()}")
    }
  }

  /**
   * Leaves a session with a demand that waits for a map update, then exits. A static map publishes
   * no update after its still image, so nothing but abandon resolves that demand before exit.
   */
  private fun exitWithParkedDemand(): Unit = runSuspendTest {
    withOffscreenSession(onFrameWake = { if (exiting) resolvedAtExit.countDown() }) {
      setStyle(EMPTY_STYLE_JSON)
      renderStill()
      val rendered = renderFrame().disposition
      check(rendered == RenderResult.RENDERED) { "the last forced frame resolved as $rendered" }
      session.demandFrame(
        FrameDemand(flags = FrameDemandFlag.WAIT_FOR_UPDATE, token = PARKED_TOKEN),
      )
      // The fence completes after the driver evaluates every earlier demand, and dumping debug
      // logs publishes no map update, so a demand without a result by then is waiting.
      complete(session.dumpDebugLogs(), "the fence")
      check(session.drainFrameResults().isEmpty()) { "the parked demand resolved before exit" }
      // The abandon resolves the demand on the session's worker, whose frame wake can arrive after
      // the binding's hook returns. Hooks run at once and the JVM halts after the last, so this one
      // keeps the JVM up until the wake arrives.
      Runtime.getRuntime()
        .addShutdownHook(
          Thread {
            if (resolvedAtExit.await(EXIT_WAKE_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
              println(ABANDONED_AT_EXIT)
            }
          },
        )
      exiting = true
      exitProcess(0)
    }
  }

  /** Exits from a session completion, on the core worker thread that delivers it. */
  private fun exitFromCompletion(): Unit = runSuspendTest {
    withOffscreenSession { exitInsideCompletion(session::reduceMemoryUse) }
  }

  /**
   * Submits until a completion arrives inside the MapLibre callback that delivers it, exits from
   * that callback, and never returns. A submission that completes before its handler is registered
   * runs the handler inline on this thread, outside any callback, so it does not count.
   */
  private fun exitInsideCompletion(submit: () -> Completion<Unit>): Nothing {
    val submitter = Thread.currentThread()
    while (true) {
      var completedInline = false
      val completion = submit()
      completion.whenComplete { _, _ ->
        if (Thread.currentThread() === submitter) {
          completedInline = true
        } else {
          println(EXITING_FROM_CALLBACK)
          exitProcess(0)
        }
      }
      if (completedInline) continue
      // Exit ends the process from the delivering thread while this thread waits.
      CountDownLatch(1).await()
    }
  }
}

private const val PARKED_TOKEN = 99uL
private const val EXIT_WAKE_TIMEOUT_SECONDS = 10L
private const val ABANDONED_AT_EXIT = "The parked demand resolved during exit"
private const val EXITING_FROM_CALLBACK = "Requesting JVM exit from a render-session completion"
