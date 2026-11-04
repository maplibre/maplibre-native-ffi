package org.maplibre.nativeffi.render

import android.graphics.PixelFormat
import android.media.ImageReader
import android.os.Handler
import android.os.HandlerThread
import android.view.Surface
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.withTimeout
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.WAIT_TIMEOUT
import org.maplibre.nativeffi.attachAndroidSurface
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.setAndroidSurfaceTarget
import org.maplibre.nativeffi.withMap

/** The forms of the Android surface calls that take a [Surface], which only the binding has. */
class AndroidSurfaceTest {
  @Test
  fun aSessionPresentsToEachSurfaceThatItGets(): Unit = runSuspendTest {
    SurfaceReader().use { first ->
      SurfaceReader().use { second ->
        withMap {
          withRenderSession(
            RenderDriverKind.CORE_WORKER,
            attach = { options ->
              map.attachAndroidSurface(first.surface, SIZE, options = options)
            },
          ) {
            setStyle(RED_BACKGROUND_STYLE)
            renderFrame()
            first.expectRedFrame()

            complete(session.releaseTarget(), "the target release")
            complete(session.setAndroidSurfaceTarget(second.surface, SIZE), "the new surface")
            renderFrame()
            second.expectRedFrame()
            // The readers close next, so the session lets go of their windows first.
            complete(session.releaseTarget(), "the last target release")
          }
        }
      }
    }
  }
}

private const val WINDOW_SIZE = 64
private val SIZE = LogicalSize(WINDOW_SIZE.toUInt(), WINDOW_SIZE.toUInt())

/** An [ImageReader] whose [surface] stands in for an app's, so a test reads what it shows. */
private class SurfaceReader : AutoCloseable {
  private val thread = HandlerThread("surface-reader").apply { start() }
  private val reader = ImageReader.newInstance(WINDOW_SIZE, WINDOW_SIZE, PixelFormat.RGBA_8888, 2)
  private val images = Channel<Unit>(Channel.CONFLATED)

  init {
    reader.setOnImageAvailableListener({ images.trySend(Unit) }, Handler(thread.looper))
  }

  val surface: Surface
    get() = reader.surface

  /** Waits for the surface's next frame, and expects the style's red at its center. */
  suspend fun expectRedFrame() {
    try {
      withTimeout(WAIT_TIMEOUT) { images.receive() }
    } catch (timeout: TimeoutCancellationException) {
      throw AssertionError("timed out waiting for a frame on the surface", timeout)
    }
    val image = checkNotNull(reader.acquireLatestImage()) { "the surface has no frame" }
    val pixel =
      try {
        val plane = image.planes[0]
        val offset = (WINDOW_SIZE / 2) * plane.rowStride + (WINDOW_SIZE / 2) * plane.pixelStride
        List(4) { plane.buffer.get(offset + it).toInt() and 0xff }
      } finally {
        image.close()
      }
    assertEquals(listOf(0xff, 0x00, 0x00, 0xff), pixel)
  }

  override fun close() {
    reader.close()
    thread.quitSafely()
  }
}
