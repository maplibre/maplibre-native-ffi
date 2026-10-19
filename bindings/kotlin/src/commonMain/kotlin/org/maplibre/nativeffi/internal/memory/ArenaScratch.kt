package org.maplibre.nativeffi.internal.memory

/** One lease per thread; a reentrant call uses its own arena blocks. */
internal class ArenaScratch {
  var leased = false
  val small by lazy { ScratchBlock(1024) }
  private var large: ScratchBlock? = null
  private var capacity = 0

  fun large(size: Int): Long {
    if (size > capacity) {
      var rounded = 1024
      while (rounded < size) rounded *= 2
      large = ScratchBlock(rounded)
      capacity = rounded
    }
    return requireNotNull(large).address
  }
}

internal expect fun threadArenaScratch(): ArenaScratch

/** Storage reclaimed when its thread and active arenas release it. */
internal expect class ScratchBlock(size: Int) {
  val address: Long
}
