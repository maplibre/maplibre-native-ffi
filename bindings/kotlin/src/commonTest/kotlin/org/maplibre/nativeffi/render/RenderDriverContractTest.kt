package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidArgumentException

class RenderDriverContractTest {
  @Test
  fun textureRingDepthRejectsNegativeValues() {
    // Native reads the depth as an unsigned hint and clamps it, so the binding only has to keep a
    // negative Int from reinterpreting as a very large ring.
    assertFailsWith<InvalidArgumentException> {
      RenderSessionAttachOptions(requestedTextureRingDepth = -1)
    }
  }
}
