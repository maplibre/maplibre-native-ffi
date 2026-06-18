package org.maplibre.nativeffi.internal.callback

import cnames.structs.mln_resource_request_handle
import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.internal.c.mln_resource_provider
import org.maplibre.nativeffi.internal.struct.ResourceStructs
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceRequestHandle

/** Owns runtime-scoped resource provider callback state. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
internal class ResourceProviderState(private val callback: ResourceProviderCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_resource_provider>()
  private val state = AtomicInt(0)
  private val nativeClosed = AtomicInt(0)

  init {
    descriptor.size = kotlinx.cinterop.sizeOf<mln_resource_provider>().toUInt()
    descriptor.callback = staticCFunction(::resourceProviderCallback)
    descriptor.user_data = selfRef.asCPointer()
  }

  fun descriptor(): CPointer<mln_resource_provider> = descriptor.ptr

  fun invoke(
    request: CPointer<org.maplibre.nativeffi.internal.c.mln_resource_request>?,
    handle: CPointer<mln_resource_request_handle>?,
  ): UInt {
    if (request == null || handle == null || !enterCallback()) return UInt.MAX_VALUE
    return try {
      val requestHandle = ResourceRequestHandle(handle)
      try {
        val decision =
          callback.handle(ResourceStructs.resourceRequest(request.pointed), requestHandle)
        requestHandle.finishProviderDecision(decision)
      } catch (_: Throwable) {
        requestHandle.finishProviderException()
      }
    } catch (_: Throwable) {
      UInt.MAX_VALUE
    } finally {
      exitCallback()
    }
  }

  override fun close() {
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) return
      val activeCallbacks = current and ACTIVE_MASK
      val next = if (activeCallbacks == 0) CLOSED_FLAG else current or CLOSING_FLAG
      if (state.compareAndSet(current, next)) {
        if (next and CLOSED_FLAG != 0) closeNative()
        return
      }
    }
  }

  internal fun isClosedForTesting(): Boolean = state.load() and CLOSED_FLAG != 0

  private fun enterCallback(): Boolean {
    while (true) {
      val current = state.load()
      if (current and (CLOSING_FLAG or CLOSED_FLAG) != 0) return false
      val activeCallbacks = current and ACTIVE_MASK
      check(activeCallbacks < ACTIVE_MASK) { "too many active resource provider callbacks" }
      if (state.compareAndSet(current, current + 1)) return true
    }
  }

  private fun exitCallback() {
    while (true) {
      val current = state.load()
      val activeCallbacks = current and ACTIVE_MASK
      check(activeCallbacks > 0) { "resource provider callback count underflow" }
      val next =
        if (activeCallbacks == 1 && current and CLOSING_FLAG != 0) {
          CLOSED_FLAG
        } else {
          current - 1
        }
      if (state.compareAndSet(current, next)) {
        if (next and CLOSED_FLAG != 0) closeNative()
        return
      }
    }
  }

  private fun closeNative() {
    if (nativeClosed.compareAndSet(0, 1)) {
      selfRef.dispose()
      nativeHeap.free(descriptor.rawPtr)
    }
  }

  private companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val CLOSING_FLAG = 1 shl 30
    private const val ACTIVE_MASK = CLOSING_FLAG - 1
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun resourceProviderCallback(
  userData: COpaquePointer?,
  request: CPointer<org.maplibre.nativeffi.internal.c.mln_resource_request>?,
  handle: CPointer<mln_resource_request_handle>?,
): UInt =
  userData?.asStableRef<ResourceProviderState>()?.get()?.invoke(request, handle) ?: UInt.MAX_VALUE
