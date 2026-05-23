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
        1UL,
        2U,
        3U,
        2.0,
        4UL,
        NativePointer.ofAddress(0x10UL),
        NativePointer.ofAddress(0x20UL),
        80UL,
      )

    assertEquals(2U, frame.width())
    assertEquals(NativePointer.ofAddress(0x10UL), frame.texture())
    scope.close()
    assertFailsWith<IllegalStateException> { frame.width() }
    assertFailsWith<IllegalStateException> { frame.texture() }
  }

  @Test
  fun vulkanFrameRejectsAccessAfterScopeCloses() {
    val scope = FrameScope()
    val frame =
      VulkanOwnedTextureFrame(
        scope,
        1UL,
        2U,
        3U,
        2.0,
        4UL,
        NativePointer.ofAddress(0x10UL),
        NativePointer.ofAddress(0x20UL),
        NativePointer.ofAddress(0x30UL),
        44U,
        5U,
      )

    assertEquals(44U, frame.format())
    assertEquals(NativePointer.ofAddress(0x20UL), frame.imageView())
    scope.close()
    assertFailsWith<IllegalStateException> { frame.format() }
    assertFailsWith<IllegalStateException> { frame.imageView() }
  }
}
