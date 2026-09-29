package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.lifecycle.yieldThread

@OptIn(ExperimentalAtomicApi::class)
internal class CallbackRoot(value: Any, val owner: Long? = null) {
  private val stored = AtomicReference<Any?>(value)
  val value: Any?
    get() = stored.load()

  val ownerList = AtomicReference<CallbackOwner?>(null)
  var previous: CallbackRoot? = null
  var next: CallbackRoot? = null

  fun release() {
    stored.store(null)
    ownerList.load()?.remove(this)
  }
}

internal expect object CallbackRoots {
  fun retain(root: CallbackRoot): Long

  fun get(token: Long): CallbackRoot?

  fun release(token: Long)
}

@OptIn(ExperimentalAtomicApi::class)
internal class CallbackOwner {
  private val locked = AtomicInt(0)
  private var first: CallbackRoot? = null

  private inline fun <T> locked(block: () -> T): T {
    while (!locked.compareAndSet(0, 1)) yieldThread()
    try {
      return block()
    } finally {
      locked.store(0)
    }
  }

  fun retain(root: CallbackRoot) = locked {
    root.ownerList.store(this)
    if (root.value != null) {
      root.next = first
      first?.previous = root
      first = root
    } else root.ownerList.store(null)
  }

  fun remove(root: CallbackRoot) = locked {
    if (root.ownerList.load() === this) {
      if (root.previous == null) first = root.next else root.previous!!.next = root.next
      root.next?.previous = root.previous
      root.previous = null
      root.next = null
      root.ownerList.store(null)
    }
  }

  /** Counts the callbacks this owner roots, which native has taken and not yet released. */
  fun rootCountForTesting(): Int = locked {
    var count = 0
    var root = first
    while (root != null) {
      count += 1
      root = root.next
    }
    count
  }

  companion object {
    val global = CallbackOwner()
  }
}

internal class CallbackRegistrationScope : AutoCloseable {
  private val roots = mutableListOf<Pair<Long, CallbackRoot>>()
  private var accepted = false

  fun register(value: Any, owner: Long? = null): Long {
    val root = CallbackRoot(value, owner)
    val token = CallbackRoots.retain(root)
    try {
      roots.add(token to root)
    } catch (error: Throwable) {
      CallbackRoots.release(token)
      throw error
    }
    return token
  }

  fun accept(owner: CallbackOwner) {
    accepted = true
    roots.forEach { owner.retain(it.second) }
  }

  override fun close() {
    if (!accepted) roots.asReversed().forEach { CallbackRoots.release(it.first) }
    roots.clear()
  }
}
