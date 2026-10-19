package org.maplibre.nativeffi.internal.memory

import java.lang.foreign.Arena

private val scratch = ThreadLocal.withInitial { ArenaScratch() }

internal actual fun threadArenaScratch(): ArenaScratch = scratch.get()

internal actual class ScratchBlock actual constructor(size: Int) {
  private val segment = Arena.ofAuto().allocate(size.toLong(), 8)
  actual val address: Long = segment.address()
}
