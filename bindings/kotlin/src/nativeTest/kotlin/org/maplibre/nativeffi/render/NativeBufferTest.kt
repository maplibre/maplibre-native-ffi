package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ExperimentalForeignApi

@OptIn(ExperimentalForeignApi::class)
class NativeBufferTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-166: readback buffers expose bounded mutable storage and reject invalid lifetime use.

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

  // BND-069: image descriptors snapshot caller-owned pixel arrays and return copies.

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
}
