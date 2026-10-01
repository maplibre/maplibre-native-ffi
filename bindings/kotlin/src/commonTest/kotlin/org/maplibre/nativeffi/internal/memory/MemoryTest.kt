package org.maplibre.nativeffi.internal.memory

import kotlin.test.Test
import kotlin.test.assertFailsWith

class MemoryTest {
  @Test
  fun stringViewCopiesRejectOversizedNativeLengths() {
    NativeArena().use { arena ->
      val byte = arena.allocate(1)

      assertFailsWith<IllegalArgumentException> { readBytes(byte, Int.MAX_VALUE.toULong() + 1UL) }
    }
  }
}
