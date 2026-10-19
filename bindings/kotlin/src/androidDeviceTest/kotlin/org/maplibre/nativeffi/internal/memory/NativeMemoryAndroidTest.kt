package org.maplibre.nativeffi.internal.memory

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals

class NativeMemoryAndroidTest {
  @Test
  fun primitiveCarriersRoundTripThroughTheRegisteredAbi() {
    val address = NativeMemory.allocate(64)
    try {
      assertContentEquals(ByteArray(64), NativeMemory.getBytes(address, 64))
      // Unaligned addresses also exercise the memcpy accessors on ARM devices.
      NativeMemory.putByte(address + 1, -37)
      NativeMemory.putShort(address + 3, -12345)
      NativeMemory.putInt(address + 7, -123456789)
      NativeMemory.putLong(address + 13, -0x123456789ABCDEFL)
      NativeMemory.putFloat(address + 23, -123.5f)
      NativeMemory.putDouble(address + 29, 98765.125)

      assertEquals((-37).toByte(), NativeMemory.getByte(address + 1))
      assertEquals((-12345).toShort(), NativeMemory.getShort(address + 3))
      assertEquals(-123456789, NativeMemory.getInt(address + 7))
      assertEquals(-0x123456789ABCDEFL, NativeMemory.getLong(address + 13))
      assertEquals(-123.5f, NativeMemory.getFloat(address + 23))
      assertEquals(98765.125, NativeMemory.getDouble(address + 29))
    } finally {
      NativeMemory.free(address)
    }

    val text = NativeMemory.allocateUninitialized(4)
    try {
      NativeMemory.putBytes(text, byteArrayOf(97, 98, 99, 0))
      assertEquals(3L, NativeMemory.stringLength(text))
      assertContentEquals(byteArrayOf(97, 98, 99, 0), NativeMemory.getBytes(text, 4))
    } finally {
      NativeMemory.free(text)
    }
  }
}
