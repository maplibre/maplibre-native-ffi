package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class NativeBufferTest {
  @Test
  fun nativeBufferTracksCapacityAndRejectsAfterClose() {
    val buffer = NativeBuffer.allocate(4L)

    assertEquals(4L, buffer.byteLength())
    assertEquals(4, buffer.toByteArray().size)
    buffer.ensureCapacity(4L)
    assertFailsWith<IllegalArgumentException> { buffer.ensureCapacity(5L) }

    buffer.close()
    buffer.close()
    assertFailsWith<IllegalStateException> { buffer.byteLength() }
  }

  @Test
  fun closeDuringBorrowRejectsNewAccessAndCompletesAfterBorrowReturns() {
    val buffer = NativeBuffer.allocate(4L)

    buffer.borrow { _, length ->
      assertEquals(4L, length)
      buffer.close()
      assertFailsWith<IllegalStateException> { buffer.byteLength() }
    }

    buffer.close()
    assertFailsWith<IllegalStateException> { buffer.toByteArray() }
  }

  @Test
  fun zeroLengthBufferHasNoBytes() {
    NativeBuffer.allocate(0L).use { buffer ->
      assertEquals(0L, buffer.byteLength())
      assertEquals(0, buffer.toByteArray().size)
    }
  }
}
