package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidArgumentException

class NativeBufferTest {
  // BND-166.

  @Test
  fun nativeBufferTracksLengthAndRejectsAfterClose() {
    val buffer = NativeBuffer.allocate(4L)

    assertEquals(4L, buffer.byteLength())
    assertEquals(4, buffer.toByteArray().size)

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

  // BND-069.

  @Test
  fun premultipliedImagePixelsSnapshotAndReturnCopies() {
    val source = byteArrayOf(1, 2, 3, 4)
    val image = PremultipliedRgba8Image(1, 1, 4, source)
    source[0] = 9

    val first = image.pixels
    assertContentEquals(byteArrayOf(1, 2, 3, 4), first)
    first[0] = 8
    assertContentEquals(byteArrayOf(1, 2, 3, 4), image.pixels)
  }

  @Test
  fun premultipliedImageRejectsEmptyPixels() {
    assertFailsWith<InvalidArgumentException> { PremultipliedRgba8Image(1, 1, 4, byteArrayOf()) }
  }
}
