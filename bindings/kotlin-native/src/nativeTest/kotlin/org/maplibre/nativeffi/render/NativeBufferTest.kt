package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class NativeBufferTest {
  @Test
  fun nativeBufferTracksCapacityAndRejectsAfterClose() {
    val buffer = NativeBuffer.allocate(4L)

    assertEquals(4L, buffer.byteLength())
    assertEquals(4, buffer.toByteArray().size)
    buffer.ensureCapacity(4UL)
    assertFailsWith<IllegalArgumentException> { buffer.ensureCapacity(5UL) }

    buffer.close()
    buffer.close()
    assertFailsWith<IllegalStateException> { buffer.byteLength() }
  }

  @Test
  fun zeroLengthBufferHasNoBytes() {
    NativeBuffer.allocate(0L).use { buffer ->
      assertEquals(0L, buffer.byteLength())
      assertEquals(0, buffer.toByteArray().size)
    }
  }

  @Test
  fun premultipliedImagePixelsAreDefensiveCopies() {
    val source = byteArrayOf(1, 2, 3, 4)
    val image = PremultipliedRgba8Image(1, 1, 4, source)
    source[0] = 9

    val first = image.pixels
    assertContentEquals(byteArrayOf(1, 2, 3, 4), first)
    first[0] = 8
    assertContentEquals(byteArrayOf(1, 2, 3, 4), image.pixels)
  }
}
