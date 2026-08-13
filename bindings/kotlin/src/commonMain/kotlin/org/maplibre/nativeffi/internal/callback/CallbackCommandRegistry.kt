package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.runtime.CommandDisposition

/** Keeps callback descriptors rooted across asynchronous set/clear commands. */
@OptIn(ExperimentalAtomicApi::class)
internal class CallbackCommandRegistry<T : AutoCloseable>(
  private val retain: (T) -> Unit,
  private val release: (T) -> Unit,
) {
  private val lock = AtomicInt(0)
  private var current: T? = null
  private val pending = mutableMapOf<ULong, Change<T>>()

  fun set(replacement: T, accept: () -> ULong): ULong = locked {
    retain(replacement)
    try {
      val commandId = accept()
      pending[commandId] = Change.Set(replacement)
      commandId
    } catch (error: Throwable) {
      release(replacement)
      throw error
    }
  }

  fun clear(accept: () -> ULong): ULong = locked {
    val commandId = accept()
    pending[commandId] = Change.Clear()
    commandId
  }

  fun finish(commandId: ULong, disposition: CommandDisposition) {
    var retired: T? = null
    locked {
      when (val change = pending.remove(commandId) ?: return@locked) {
        is Change.Set -> {
          if (disposition == CommandDisposition.COMMITTED) {
            retired = current
            current = change.replacement
          } else {
            retired = change.replacement
          }
        }
        is Change.Clear -> {
          if (disposition == CommandDisposition.COMMITTED) {
            retired = current
            current = null
          }
        }
      }
    }
    retired?.let(release)
  }

  fun close() {
    val roots = locked {
      buildList {
        current?.let(::add)
        pending.values.forEach { change -> if (change is Change.Set) add(change.replacement) }
        current = null
        pending.clear()
      }
    }

    roots.forEach(release)
  }

  fun currentForTesting(): T? = locked { current }

  private inline fun <R> locked(block: () -> R): R {
    while (!lock.compareAndSet(0, 1)) {}
    try {
      return block()
    } finally {
      lock.store(0)
    }
  }

  private sealed interface Change<T> {
    class Set<T>(val replacement: T) : Change<T>

    class Clear<T> : Change<T>
  }
}
