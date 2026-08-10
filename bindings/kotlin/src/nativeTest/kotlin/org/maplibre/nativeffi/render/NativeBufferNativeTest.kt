package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.error.InvalidArgumentException

@OptIn(ExperimentalForeignApi::class)
class NativeBufferNativeTest {
  @Test
  fun capacityValidationUsesTheNativeAllocationLength() {
    NativeBuffer.allocate(4L).use { buffer ->
      buffer.ensureCapacity(4UL)
      assertFailsWith<InvalidArgumentException> { buffer.ensureCapacity(5UL) }
    }
  }

  @Test
  fun closeDuringPointerBorrowDefersReleaseUntilBorrowReturns() {
    val buffer = NativeBuffer.allocate(4L)

    buffer.borrow { _, length ->
      assertEquals(4L, length)
      buffer.close()
      assertFailsWith<IllegalStateException> { buffer.byteLength() }
    }

    assertFailsWith<IllegalStateException> { buffer.toByteArray() }
  }
}
