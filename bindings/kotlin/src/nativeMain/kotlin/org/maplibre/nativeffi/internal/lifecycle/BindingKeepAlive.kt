package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

@OptIn(ExperimentalAtomicApi::class) private val ownerFence = AtomicReference<Any?>(null)

@OptIn(ExperimentalAtomicApi::class)
internal actual fun bindingKeepAlive(owner: Any) {
  // A shared atomic publication keeps the whole owner alive through the preceding call.
  ownerFence.store(owner)
  ownerFence.compareAndSet(owner, null)
}
