package org.maplibre.nativeffi.internal.lifecycle

import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

internal actual class PlatformLock actual constructor() {
  private val lock = ReentrantLock()

  actual fun <R> withLock(block: () -> R): R = lock.withLock(block)
}
