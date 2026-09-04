package org.maplibre.nativeffi.internal.lifecycle

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.ptr
import platform.posix.pthread_mutex_init
import platform.posix.pthread_mutex_lock
import platform.posix.pthread_mutex_t
import platform.posix.pthread_mutex_unlock

@OptIn(ExperimentalForeignApi::class)
internal actual class PlatformLock actual constructor() {
  private val mutex = nativeHeap.alloc<pthread_mutex_t>().also { pthread_mutex_init(it.ptr, null) }

  actual fun <R> withLock(block: () -> R): R {
    pthread_mutex_lock(mutex.ptr)
    try {
      return block()
    } finally {
      pthread_mutex_unlock(mutex.ptr)
    }
  }
}
