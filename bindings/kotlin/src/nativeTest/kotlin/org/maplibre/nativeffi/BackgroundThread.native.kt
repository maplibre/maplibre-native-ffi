@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi

import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar

internal actual fun runOnBackgroundThread(block: () -> Unit) {
  memScoped {
    val selfRef = StableRef.create(block)
    val thread = alloc<pthread_tVar>()
    val status =
      pthread_create(
        thread.ptr,
        null,
        staticCFunction(::invokeBackgroundThreadBlock),
        selfRef.asCPointer(),
      )
    if (status != 0) {
      selfRef.dispose()
      error("pthread_create failed with status $status")
    }
    pthread_join(thread.ptr[0], null)
  }
}

private fun invokeBackgroundThreadBlock(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<() -> Unit>()
  try {
    selfRef.get().invoke()
  } finally {
    selfRef.dispose()
  }
  return null
}
