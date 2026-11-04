package org.maplibre.nativeffi.render

import android.hardware.HardwareBuffer
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.AndroidHardwareBufferRingDescriptor
import org.maplibre.nativeffi.GpuSync
import org.maplibre.nativeffi.GpuSyncKind
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.attachAndroidHardwareBufferRing
import org.maplibre.nativeffi.getHardwareBuffer
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withAndroidHardwareBuffer
import org.maplibre.nativeffi.withMap

/**
 * The form of an acquired frame's hardware buffer as a [HardwareBuffer], which only the binding
 * has.
 */
class AndroidHardwareBufferTest {
  @Test
  fun aFrameHandsOverItsBufferAsAHardwareBuffer(): Unit = runSuspendTest {
    withMap {
      withRenderSession(
        RenderDriverKind.CORE_WORKER,
        attach = { options ->
          map.attachAndroidHardwareBufferRing(
            AndroidHardwareBufferRingDescriptor(
              SIZE,
              bufferCount = 2u,
              usage = HardwareBuffer.USAGE_CPU_READ_OFTEN.toULong(),
            ),
            options = options,
          )
        },
      ) {
        setStyle(RED_BACKGROUND_STYLE)
        renderFrame()
        val frame = assertNotNull(session.acquireFrame())
        val (width, height) = frame.withAndroidHardwareBuffer { it.width to it.height }
        val buffer =
          try {
            frame.getHardwareBuffer()
          } finally {
            frame.release(GpuSync(kind = GpuSyncKind.CPU_COMPLETE))
          }
        // The object holds its own reference, so it outlives the frame.
        buffer.use {
          assertFalse(it.isClosed)
          assertEquals(width.toInt(), it.width)
          assertEquals(height.toInt(), it.height)
          assertEquals(HardwareBuffer.RGBA_8888, it.format)
          assertTrue(it.usage and HardwareBuffer.USAGE_CPU_READ_OFTEN != 0L)
        }
      }
    }
  }
}

private val SIZE = LogicalSize(64u, 48u)
