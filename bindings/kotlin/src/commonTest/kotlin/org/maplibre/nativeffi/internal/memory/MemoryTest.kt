package org.maplibre.nativeffi.internal.memory

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class MemoryTest {
  @Test
  fun stringViewsEncodeUtf8DirectlyWithPlatformReplacement() {
    for (text in
      listOf(
        "",
        "plain",
        "a\u0000b",
        "é€",
        "\uD83D\uDE03",
        "\uD800x\uDC00",
        "\uDC00\uD800",
        "é".repeat(129),
      )) {
      NativeArena().use { arena ->
        val view = arena.view(text)
        assertContentEquals(
          text.encodeToByteArray(),
          readBytes(readAddress(view), readSize(view + NativeMemory.addressSize)),
        )
        if ('\u0000' !in text) {
          assertContentEquals(
            text.encodeToByteArray(),
            NativeMemory.getBytes(arena.cString(text), text.encodeToByteArray().size),
          )
        }
      }
    }
  }

  @Test
  fun reentrantArenasPreserveOuterInputsAndReuseZeroedStorage() {
    val image = ByteArray(256 * 256 * 4) { it.toByte() }
    NativeArena().use { outer ->
      val record = outer.allocate(32)
      NativeMemory.putLong(record, -1)
      val pixels = outer.bytes(image)
      NativeArena().use { inner ->
        val nested = inner.allocate(32)
        assertEquals(0L, NativeMemory.getLong(nested))
        NativeMemory.putLong(nested, 123)
        inner.bytes(ByteArray(image.size) { 42 })
        assertEquals(-1L, NativeMemory.getLong(record))
        assertContentEquals(image, NativeMemory.getBytes(pixels, image.size))
      }
      // A second large view in the same call must also preserve the first one.
      outer.bytes(ByteArray(image.size) { 43 })
      assertContentEquals(image, NativeMemory.getBytes(pixels, image.size))
    }
    NativeArena().use { next ->
      assertEquals(0L, NativeMemory.getLong(next.allocate(32)))
    }
  }

  @Test
  fun nullTerminatedStringsRejectEmbeddedNul() {
    NativeArena().use { arena ->
      val error = assertFailsWith<IllegalArgumentException> { arena.cString("a\u0000b") }

      assertEquals("text contains an embedded NUL", error.message)
    }
  }

  @Test
  fun stringViewCopiesRejectOversizedNativeLengths() {
    NativeArena().use { arena ->
      val byte = arena.allocate(1)

      assertFailsWith<IllegalArgumentException> { readBytes(byte, Int.MAX_VALUE.toULong() + 1UL) }
    }
  }
}
