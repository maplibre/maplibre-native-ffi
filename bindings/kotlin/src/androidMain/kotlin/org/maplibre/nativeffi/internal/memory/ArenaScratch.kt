package org.maplibre.nativeffi.internal.memory

import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions

private val scratch =
  object : ThreadLocal<ArenaScratch>() {
    override fun initialValue(): ArenaScratch = ArenaScratch()
  }

internal actual fun threadArenaScratch(): ArenaScratch = scratch.get()!!

internal actual class ScratchBlock actual constructor(size: Int) {
  actual val address: Long = NativeMemory.allocateUninitialized(size.toLong())

  init {
    val address = address
    UnreachableActions.register(this, Runnable { NativeMemory.free(address) })
  }
}
