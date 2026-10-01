package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.lifecycle.WeakBox
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

/**
 * Strong roots for values native holds by token, such as a callback's `user_data` or a pending
 * completion. Tokens are never reused, so a stray or repeated release finds nothing.
 */
@OptIn(ExperimentalAtomicApi::class)
internal object NativeRoots {
  private val locked = AtomicInt(0)
  private var next = 1L
  private val roots = HashMap<Long, Any>()

  private inline fun <T> locked(block: () -> T): T {
    while (!locked.compareAndSet(0, 1)) yieldThread()
    try {
      return block()
    } finally {
      locked.store(0)
    }
  }

  fun retain(value: Any): Long = locked { next++.also { roots[it] = value } }

  fun get(token: Long): Any? = locked { roots[token] }

  /** Drops the root and returns its value, or null for a token already released. */
  fun release(token: Long): Any? = locked { roots.remove(token) }
}

/**
 * The callback roots native holds as `user_data` tokens. A root holds its value weakly from here,
 * so a registration never keeps its owner reachable; the owner's [CallbackOwner] holds it strongly.
 */
internal object CallbackRoots {
  fun retain(root: CallbackRoot): Long = NativeRoots.retain(WeakBox(root))

  fun get(token: Long): CallbackRoot? =
    (NativeRoots.get(token) as? WeakBox<*>)?.get() as? CallbackRoot

  fun release(token: Long) {
    ((NativeRoots.release(token) as? WeakBox<*>)?.get() as? CallbackRoot)?.release()
  }
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
