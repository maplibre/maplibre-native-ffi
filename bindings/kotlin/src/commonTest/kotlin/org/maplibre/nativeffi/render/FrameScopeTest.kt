package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.generated.MetalOwnedTextureFrame

class FrameScopeTest {
  @Test
  fun generatedGpuDescriptorsAndPointersExpireTogether() {
    val scope = FrameScope()
    val texture = NativePointer.scoped(0x10L, scope)
    val frame =
      MetalOwnedTextureFrame(
          texture = texture,
          device = NativePointer.scoped(0x20L, scope),
          width = 2u,
          height = 3u,
          generation = ULong.MAX_VALUE,
        )
        .also { it.bindingScope = scope }
    assertEquals(ULong.MAX_VALUE, frame.generation)
    assertEquals(2u, frame.width)
    assertEquals(0x10L, texture.address)
    scope.close()
    assertFailsWith<IllegalStateException> { frame.width }
    assertFailsWith<IllegalStateException> { frame.texture }
    assertFailsWith<IllegalStateException> { texture.address }
    assertFailsWith<IllegalStateException> { texture.toString() }
    assertFailsWith<IllegalStateException> { texture.hashCode() }
  }
}
