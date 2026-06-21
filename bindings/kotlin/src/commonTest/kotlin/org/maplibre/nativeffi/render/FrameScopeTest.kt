package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class FrameScopeTest {
  // BND-168, BND-173: acquired frame metadata and backend handles reject stale access.

  @Test
  fun metalFrameRejectsAccessAfterScopeCloses() {
    val scope = FrameScope()
    val highBit = Long.MAX_VALUE.toULong() + 1UL
    val frame =
      MetalOwnedTextureFrame(
        scope,
        highBit.toLong(),
        2,
        3,
        2.0,
        ULong.MAX_VALUE.toLong(),
        NativePointer.ofAddress(0x10L),
        NativePointer.ofAddress(0x20L),
        highBit.toLong(),
      )

    assertEquals(highBit.toLong(), frame.generation())
    assertEquals(2, frame.width())
    assertEquals(ULong.MAX_VALUE.toLong(), frame.frameId())
    assertEquals(highBit.toLong(), frame.pixelFormat())
    assertEquals(NativePointer.ofAddress(0x10L), frame.texture())
    val retainedTexture = NativePointer.scoped(0x10L, scope)
    scope.close()
    assertFailsWith<IllegalStateException> { frame.width() }
    assertFailsWith<IllegalStateException> { frame.texture() }
    assertFailsWith<IllegalStateException> { retainedTexture.address }
    assertFailsWith<IllegalStateException> { retainedTexture.toString() }
    assertFailsWith<IllegalStateException> { retainedTexture.hashCode() }
    assertFailsWith<IllegalStateException> {
      retainedTexture.equals(NativePointer.ofAddress(0x10L))
    }
  }

  @Test
  fun vulkanFrameRejectsAccessAfterScopeCloses() {
    val scope = FrameScope()
    val highBit = Long.MAX_VALUE.toULong() + 1UL
    val frame =
      VulkanOwnedTextureFrame(
        scope,
        highBit.toLong(),
        2,
        3,
        2.0,
        ULong.MAX_VALUE.toLong(),
        NativePointer.ofAddress(0x10L),
        NativePointer.ofAddress(0x20L),
        NativePointer.ofAddress(0x30L),
        UInt.MAX_VALUE.toInt(),
        0x8000_0000U.toInt(),
      )

    assertEquals(highBit.toLong(), frame.generation())
    assertEquals(ULong.MAX_VALUE.toLong(), frame.frameId())
    assertEquals(UInt.MAX_VALUE.toInt(), frame.format())
    assertEquals(0x8000_0000U.toInt(), frame.layout())
    assertEquals(NativePointer.ofAddress(0x20L), frame.imageView())
    scope.close()
    assertFailsWith<IllegalStateException> { frame.format() }
    assertFailsWith<IllegalStateException> { frame.imageView() }
  }

  @Test
  fun openglFramePreservesHighBitTextureValues() {
    val scope = FrameScope()
    val highBit = Long.MAX_VALUE.toULong() + 1UL
    val frame =
      OpenGLOwnedTextureFrame(
        scope,
        highBit.toLong(),
        2,
        3,
        2.0,
        ULong.MAX_VALUE.toLong(),
        UInt.MAX_VALUE.toInt(),
        0x0de1,
        0x8000_8058U.toInt(),
        0x8000_1908U.toInt(),
        0x8000_1401U.toInt(),
      )

    assertEquals(highBit.toLong(), frame.generation())
    assertEquals(ULong.MAX_VALUE.toLong(), frame.frameId())
    assertEquals(UInt.MAX_VALUE.toInt(), frame.texture())
    assertEquals(0x8000_8058U.toInt(), frame.internalFormat())
    assertEquals(0x8000_1908U.toInt(), frame.format())
    assertEquals(0x8000_1401U.toInt(), frame.type())
    scope.close()
    assertFailsWith<IllegalStateException> { frame.width() }
    assertFailsWith<IllegalStateException> { frame.texture() }
    assertFailsWith<IllegalStateException> { frame.internalFormat() }
  }
}
