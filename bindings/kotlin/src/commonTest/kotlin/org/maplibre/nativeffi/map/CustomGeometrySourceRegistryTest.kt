package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.style.SourceType

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
  fun sourceDetachmentReleasesCallbackState() {
    val released = mutableListOf<String>()
    val registry = registry(released)
    registry.install("attached", State("attached")) {}
    registry.install("detached", State("detached")) {}

    registry.releaseDetached { sourceId ->
      if (sourceId == "attached") SourceType.CUSTOM_VECTOR else null
    }

    assertEquals(1, registry.size)
    assertEquals(listOf("detached"), released)
  }

  @Test
  fun staleStyleLoadedEventPreservesReusedSourceId() {
    val released = mutableListOf<String>()
    val registry = registry(released)
    registry.install("source", State("old")) {}
    registry.clear()
    registry.install("source", State("new")) {}

    registry.releaseDetached { SourceType.CUSTOM_VECTOR }

    assertEquals(1, registry.size)
    assertEquals(listOf("old"), released)
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
