package org.maplibre.nativeffi.internal.memory

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr

@OptIn(ExperimentalForeignApi::class)
class MemoryUtilTest {
  @Test
  fun stringViewCopiesRejectOversizedNativeLengths() {
    memScoped {
      val byte = alloc<ByteVar>()

      assertFailsWith<IllegalArgumentException> {
        MemoryUtil.copyStringView(byte.ptr, Int.MAX_VALUE.toULong() + 1UL)
      }
    }
  }
}
