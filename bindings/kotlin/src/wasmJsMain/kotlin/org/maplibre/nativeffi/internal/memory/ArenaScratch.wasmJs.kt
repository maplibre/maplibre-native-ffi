package org.maplibre.nativeffi.internal.memory

// Kotlin runs on one thread in the browser, and the module never unloads, so one scratch serves
// every arena and its blocks stay allocated.
private val scratch = ArenaScratch()

internal actual fun threadArenaScratch(): ArenaScratch = scratch

internal actual class ScratchBlock actual constructor(size: Int) {
  actual val address: Long = NativeMemory.allocateUninitialized(size.toLong())
}
