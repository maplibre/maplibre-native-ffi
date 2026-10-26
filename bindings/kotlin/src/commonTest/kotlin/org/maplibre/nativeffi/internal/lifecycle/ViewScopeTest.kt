package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.MetalTextureFrame
import org.maplibre.nativeffi.NativePointer

class ViewScopeTest {
  @Test
  fun generatedGpuDescriptorsAndPointersExpireTogether() {
    val scope = ViewScope()
    val texture = NativePointer.scoped(0x10L, scope)
    val frame =
      MetalTextureFrame(
          generation = ULong.MAX_VALUE,
          width = 2u,
          height = 3u,
          scaleFactor = 1.0,
          frameId = 1uL,
          slot = 0u,
          texture = texture,
          device = NativePointer.scoped(0x20L, scope),
          pixelFormat = 0uL,
        )
        .also { it.bindingScope = scope }
    assertEquals(ULong.MAX_VALUE, frame.generation)
    assertEquals(2u, frame.width)
    assertEquals(0x10L, texture.address)
    scope.close()
    // The scope guards only what the frame lends; plain values stay readable.
    assertEquals(2u, frame.width)
    assertFailsWith<IllegalStateException> { frame.texture }
    assertFailsWith<IllegalStateException> { texture.address }
    assertFailsWith<IllegalStateException> { texture.toString() }
    assertFailsWith<IllegalStateException> { texture.hashCode() }
  }
}
