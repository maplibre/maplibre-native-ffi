package org.maplibre.nativeffi.lifecycle

import java.util.concurrent.CountDownLatch
import kotlin.system.exitProcess
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.FrameDemand
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.OpenglClientApi
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.graphicsProperties
import org.maplibre.nativeffi.libraryProperties
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.TestBackend
import org.maplibre.nativeffi.render.TestGraphics
import org.maplibre.nativeffi.render.withOwnedTexture
import org.maplibre.nativeffi.runChildJvm
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap

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
    runChildJvm(
      RenderSessionExitProbe::class,
      listOf(scenario),
      libraryProperties() + graphicsProperties(),
    )
}

object RenderSessionExitProbe {
  @Volatile private var exiting = false

  @JvmStatic
  fun main(args: Array<String>) {
    Maplibre.loadNativeLibrary()
    when (args.single()) {
      "live" -> exitWithParkedDemand()
      "callback" -> exitFromCompletion()
      else -> error("unknown scenario ${args.single()}")
    }
  }

  /**
   * Leaves a session with every ring slot acquired and one more demand parked behind them, then
   * exits. Nothing but abandon resolves that demand before exit.
   */
  private fun exitWithParkedDemand(): Unit = runSuspendTest {
    withOwnedTexture(onFrameWake = { if (exiting) println(ABANDONED_AT_EXIT) }) {
      setStyle(EMPTY_STYLE_JSON)
      renderStill()
      val rendered = renderFrame().disposition
      check(rendered == RenderResult.RENDERED) { "the frame to acquire resolved as $rendered" }
      checkNotNull(session.acquireFrame()) { "the one-slot ring has no frame to acquire" }
      session.requestFrame(FrameDemand(flags = FrameDemandFlag(0u), token = PARKED_TOKEN))
      withContext(TestGraphics.thread) { session.serviceDriverWork(0uL) }
      val parked = session.getSnapshot().pendingDemandCount
      check(parked == 1u) { "expected one parked demand, found $parked" }
      exiting = true
      exitProcess(0)
    }
  }

  /** Exits from a session completion, on the thread that delivers it inside a driver call. */
  private fun exitFromCompletion(): Unit = runSuspendTest {
    if (TestGraphics.backend == TestBackend.WGL) {
      // WGL has no core-worker owned texture, so the completion arrives inside driver service on
      // the host's graphics thread instead.
      withOwnedTexture { exitInsideCompletion(session::reduceMemoryUse) { complete(it) } }
    } else {
      withMap(MapMode.STATIC, options = smallMapOptions(MapMode.STATIC, WIDTH, HEIGHT)) {
        val attachment = withContext(TestGraphics.thread) { attachCoreWorkerTexture(map) }
        attachment.ready.await()
        exitInsideCompletion(attachment.session::reduceMemoryUse) {}
      }
    }
  }

  /**
   * Submits until a completion arrives inside the MapLibre callback that delivers it, exits from
   * that callback, and never returns. A submission that completes before its handler is registered
   * runs the handler inline on this thread, outside any callback, so it does not count. [deliver]
   * drives a submission to completion when no core worker does.
   */
  private suspend fun exitInsideCompletion(
    submit: () -> Deferred<Unit>,
    deliver: suspend (Deferred<Unit>) -> Unit,
  ): Nothing {
    val submitter = Thread.currentThread()
    while (true) {
      var completedInline = false
      val completion = submit()
      completion.invokeOnCompletion {
        if (Thread.currentThread() === submitter) {
          completedInline = true
        } else {
          println(EXITING_FROM_CALLBACK)
          exitProcess(0)
        }
      }
      if (completedInline) continue
      deliver(completion)
      // Exit ends the process from the delivering thread while this thread waits.
      CountDownLatch(1).await()
    }
  }

  /**
   * Attaches an owned texture that a core worker drives on the build's backend. An OpenGL core
   * worker owns a dedicated context, which EGL creates from the display and config.
   */
  private suspend fun attachCoreWorkerTexture(map: MapHandle): RenderSessionAttachment {
    val options = RenderSessionAttachOptions(driver = RenderDriverKind.CORE_WORKER)
    if (TestGraphics.backend != TestBackend.EGL) {
      return TestGraphics.attachOwnedTexture(map, WIDTH, HEIGHT, options)
    }
    val context = TestGraphics.context()
    val egl =
      EglContextDescriptor(
        NativePointer.ofAddress(context.eglDisplay),
        NativePointer.ofAddress(context.eglConfig),
        NativePointer.NULL_POINTER,
        OpenglClientApi.GLES,
        NativePointer.NULL_POINTER,
      )
    return map.openglOwnedTextureAttach(
      OpenglOwnedTextureDescriptor(
        RenderTargetExtent(WIDTH.toUInt(), HEIGHT.toUInt(), 1.0),
        OpenglContextDescriptor(
          OpenglContextOwnership.DEDICATED,
          OpenglContextDescriptorData.Egl(egl),
        ),
      ),
      options,
    )
  }
}

private const val WIDTH = 32
private const val HEIGHT = 16
private const val PARKED_TOKEN = 99uL
private const val ABANDONED_AT_EXIT = "The parked demand resolved during exit"
private const val EXITING_FROM_CALLBACK = "Requesting JVM exit from a render-session completion"
