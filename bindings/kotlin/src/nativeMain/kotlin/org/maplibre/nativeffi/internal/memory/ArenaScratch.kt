package org.maplibre.nativeffi.internal.memory

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.concurrent.ThreadLocal
import kotlin.native.ref.createCleaner

@ThreadLocal private val scratch = ArenaScratch()

internal actual fun threadArenaScratch(): ArenaScratch = scratch

@OptIn(ExperimentalNativeApi::class)
internal actual class ScratchBlock actual constructor(size: Int) {
  actual val address: Long = NativeMemory.allocateUninitialized(size.toLong())
  @Suppress("unused") private val cleaner = createCleaner(address) { NativeMemory.free(it) }
}
