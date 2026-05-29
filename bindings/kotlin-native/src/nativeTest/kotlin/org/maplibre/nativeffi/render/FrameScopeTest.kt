package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class FrameScopeTest {
  @Test
  fun metalFrameRejectsAccessAfterScopeCloses() {
    val scope = FrameScope()
    val frame =
      MetalOwnedTextureFrame(
        scope,
        1L,
        2,
        3,
        2.0,
        4L,
        NativePointer.ofAddress(0x10L),
        NativePointer.ofAddress(0x20L),
        80L,
      )

    assertEquals(2, frame.width())
    assertEquals(NativePointer.ofAddress(0x10L), frame.texture())
    val retainedTexture = NativePointer.scoped(0x10L, scope)
    scope.close()
    assertFailsWith<IllegalStateException> { frame.width() }
    assertFailsWith<IllegalStateException> { frame.texture() }
    assertFailsWith<IllegalStateException> { retainedTexture.address }
  }

  @Test
  fun vulkanFrameRejectsAccessAfterScopeCloses() {
    val scope = FrameScope()
    val frame =
      VulkanOwnedTextureFrame(
        scope,
        1L,
        2,
        3,
        2.0,
        4L,
        NativePointer.ofAddress(0x10L),
        NativePointer.ofAddress(0x20L),
        NativePointer.ofAddress(0x30L),
        44,
        5,
      )

    assertEquals(44, frame.format())
    assertEquals(NativePointer.ofAddress(0x20L), frame.imageView())
    scope.close()
    assertFailsWith<IllegalStateException> { frame.format() }
    assertFailsWith<IllegalStateException> { frame.imageView() }
  }

  @Test
  fun openglFramePreservesHighBitTextureValues() {
    val scope = FrameScope()
    val frame =
      OpenGLOwnedTextureFrame(
        scope,
        1L,
        2,
        3,
        2.0,
        4L,
        UInt.MAX_VALUE.toInt(),
        0x0de1,
        0x8058,
        0x1908,
        0x1401,
      )

    assertEquals(UInt.MAX_VALUE.toInt(), frame.texture())
  }
}
