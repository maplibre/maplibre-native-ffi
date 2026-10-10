package org.maplibre.nativeffi.internal.memory

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNull
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus

class MemoryTest {
  @Test
  fun nullTerminatedStringsRejectEmbeddedNul() {
    NativeArena().use { arena ->
      val error = assertFailsWith<InvalidArgumentException> { arena.cString("a\u0000b") }

      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertNull(error.nativeStatusCode)
      assertEquals("text contains an embedded NUL", error.diagnostic)
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
