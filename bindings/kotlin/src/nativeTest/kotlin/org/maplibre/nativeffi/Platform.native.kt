@file:OptIn(
  kotlinx.cinterop.ExperimentalForeignApi::class,
  kotlin.experimental.ExperimentalNativeApi::class,
  kotlin.native.runtime.NativeRuntimeApi::class,
)

package org.maplibre.nativeffi

import kotlin.native.ref.WeakReference
import kotlin.native.runtime.GC
import kotlin.time.Duration.Companion.seconds
import kotlin.time.TimeSource
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar

internal actual class TestThread actual constructor(block: () -> Unit) {
  private val body = ThreadBody(block)
  private val reference = StableRef.create(body)
  private val thread = nativeHeap.alloc<pthread_tVar>()

  init {
    val status =
      pthread_create(thread.ptr, null, staticCFunction(::runThreadBody), reference.asCPointer())
    if (status != 0) {
      reference.dispose()
      nativeHeap.free(thread.rawPtr)
      error("pthread_create failed with status $status")
    }
  }

  actual fun join() {
    pthread_join(thread.ptr[0], null)
    reference.dispose()
    nativeHeap.free(thread.rawPtr)
    body.failure?.let { throw it }
  }
}

private class ThreadBody(val block: () -> Unit) {
  var failure: Throwable? = null
}

private fun runThreadBody(raw: COpaquePointer?): COpaquePointer? {
  val body = requireNotNull(raw).asStableRef<ThreadBody>().get()
  try {
    body.block()
  } catch (error: Throwable) {
    body.failure = error
  }
  return null
}

internal actual class TestWeakReference actual constructor(value: Any) {
  private val reference = WeakReference(value)

  actual val value: Any?
    get() = reference.value

  actual val isCleared: Boolean
    get() = reference.value == null
}

internal actual fun awaitCollected(reference: TestWeakReference): Boolean {
  // GC.collect() runs a full collection before it returns, so each round is a real result.
  val deadline = TimeSource.Monotonic.markNow() + 10.seconds
  while (deadline.hasNotPassedNow()) {
    GC.collect()
    if (reference.isCleared) return true
  }
  return reference.isCleared
}

internal actual fun requestCollection() {
  GC.collect()
}
