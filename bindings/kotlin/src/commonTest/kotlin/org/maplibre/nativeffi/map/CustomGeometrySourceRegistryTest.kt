package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class CustomGeometrySourceRegistryTest {
  @Test
  fun failedReplacementPreservesExistingCallbackState() {
    val released = mutableListOf<String>()
    val registry = registry(released)
    registry.install("source", State("existing")) {}

    assertFailsWith<IllegalStateException> {
      registry.install("source", State("replacement")) { error("native install failed") }
    }

    assertEquals(1, registry.size)
    assertEquals(listOf("replacement"), released)
  }

  @Test
  fun clearingReleasesEveryRemainingCallbackState() {
    val released = mutableListOf<String>()
    val registry = registry(released)
    registry.install("first", State("first")) {}
    registry.install("second", State("second")) {}

    registry.clear()

    assertEquals(0, registry.size)
    assertEquals(listOf("first", "second"), released)
  }

  private fun registry(released: MutableList<String>): CustomGeometrySourceRegistry<State> =
    CustomGeometrySourceRegistry { state ->
      state.close()
      released += state.name
    }

  private class State(val name: String) : AutoCloseable {
    override fun close() = Unit
  }
}
