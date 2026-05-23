package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class NativeBufferTest {
  @Test
  fun nativeBufferTracksCapacityAndRejectsAfterClose() {
    val buffer = NativeBuffer.allocate(4UL)

    assertEquals(4UL, buffer.byteLength())
    assertEquals(4, buffer.toByteArray().size)
    buffer.ensureCapacity(4UL)
    assertFailsWith<IllegalArgumentException> { buffer.ensureCapacity(5UL) }

    buffer.close()
    buffer.close()
    assertFailsWith<IllegalStateException> { buffer.byteLength() }
  }

  @Test
  fun zeroLengthBufferHasNoBytes() {
    NativeBuffer.allocate(0UL).use { buffer ->
      assertEquals(0UL, buffer.byteLength())
      assertEquals(0, buffer.toByteArray().size)
    }
  }
}
