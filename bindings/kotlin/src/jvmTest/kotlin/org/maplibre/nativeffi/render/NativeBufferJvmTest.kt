package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidArgumentException

class NativeBufferJvmTest {
  @Test
  fun capacityValidationUsesTheFfmSegmentLength(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      NativeBuffer.allocate(4L).use { buffer ->
        buffer.ensureCapacity(4L)
        assertFailsWith<InvalidArgumentException> { buffer.ensureCapacity(5L) }
      }
    }

  @Test
  fun closeDuringFfmBorrowDefersReleaseUntilBorrowReturns(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val buffer = NativeBuffer.allocate(4L)

      buffer.borrow { _, length ->
        assertEquals(4L, length)
        buffer.close()
        assertFailsWith<IllegalStateException> { buffer.byteLength() }
      }

      assertFailsWith<IllegalStateException> { buffer.toByteArray() }
    }
}
