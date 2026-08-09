package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.style.SourceType

/** Owns callback state for custom geometry sources attached to one map. */
internal class CustomGeometrySourceRegistry<T : AutoCloseable>(private val release: (T) -> Unit) {
  private val states = mutableMapOf<String, T>()

  val size: Int
    get() = states.size

  fun install(sourceId: String, replacement: T, installNative: () -> Unit) {
    try {
      installNative()
    } catch (error: Throwable) {
      release(replacement)
      throw error
    }
    states.put(sourceId, replacement)?.let(release)
  }

  fun remove(sourceId: String) {
    states.remove(sourceId)?.let(release)
  }

  fun clear() {
    states.values.forEach(release)
    states.clear()
  }

  fun releaseDetached(sourceType: (String) -> SourceType?) {
    val iterator = states.iterator()
    while (iterator.hasNext()) {
      val entry = iterator.next()
      if (sourceType(entry.key) != SourceType.CUSTOM_VECTOR) {
        release(entry.value)
        iterator.remove()
      }
    }
  }
}
