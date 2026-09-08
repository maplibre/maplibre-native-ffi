package org.maplibre.nativeffi.map

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

/**
 * Owns callback state for custom geometry sources attached to one map.
 *
 * Host threads install and remove entries while the native release upcall removes them from a
 * worker thread, so the table is swapped atomically rather than mutated in place.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class CustomGeometrySourceRegistry<T : AutoCloseable>(private val release: (T) -> Unit) {
  private val states = AtomicReference(emptyMap<String, T>())

  val size: Int
    get() = states.load().size

  /**
   * Publishes [replacement] for [sourceId] before [installNative] submits it, so a release upcall
   * that fires as soon as the command is accepted finds the entry it has to drop.
   *
   * A rejected registration restores the entry it displaced and releases only [replacement]; an
   * accepted one releases the state it replaced.
   */
  fun install(sourceId: String, replacement: T, installNative: () -> Unit) {
    var previous: T? = null
    update { current ->
      previous = current[sourceId]
      current + (sourceId to replacement)
    }
    try {
      installNative()
    } catch (error: Throwable) {
      val displaced = previous
      update { current ->
        when {
          current[sourceId] !== replacement -> current
          displaced == null -> current - sourceId
          else -> current + (sourceId to displaced)
        }
      }
      release(replacement)
      throw error
    }
    previous?.let(release)
  }

  fun remove(sourceId: String) {
    var removed: T? = null
    update { current ->
      removed = current[sourceId]
      current - sourceId
    }
    removed?.let(release)
  }

  fun clear() {
    var pending: Collection<T> = emptyList()
    update { current ->
      pending = current.values
      emptyMap()
    }
    pending.forEach(release)
  }

  private inline fun update(transform: (Map<String, T>) -> Map<String, T>) {
    while (true) {
      val current = states.load()
      if (states.compareAndSet(current, transform(current))) return
    }
  }
}
