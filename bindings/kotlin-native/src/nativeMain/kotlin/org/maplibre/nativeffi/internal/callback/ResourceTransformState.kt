package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.ByteVar
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
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_transform
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response_set_url
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceTransformCallback
import org.maplibre.nativeffi.resource.ResourceTransformRequest

/** Owns runtime-scoped resource transform callback state. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
internal class ResourceTransformState(private val callback: ResourceTransformCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_resource_transform>()
  private val state = AtomicInt(0)
  private val nativeClosed = AtomicInt(0)

  init {
    descriptor.size = kotlinx.cinterop.sizeOf<mln_resource_transform>().toUInt()
    descriptor.callback = staticCFunction(::resourceTransformCallback)
    descriptor.user_data = selfRef.asCPointer()
  }

  fun descriptor(): CPointer<mln_resource_transform> = descriptor.ptr

  fun invoke(
    rawKind: UInt,
    url: CPointer<ByteVar>?,
    outResponse: CPointer<mln_resource_transform_response>?,
  ): Int {
    if (outResponse == null || !enterCallback()) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    return try {
      outResponse.pointed.size = kotlinx.cinterop.sizeOf<mln_resource_transform_response>().toUInt()
      outResponse.pointed.url = null
      val request =
        ResourceTransformRequest(ResourceKind.fromNative(rawKind), MemoryUtil.copyCString(url))
      val replacement = callback.transform(request)
      if (!replacement.isNullOrEmpty()) {
        val status = setResponseUrl(outResponse, replacement)
        if (status != MaplibreStatus.OK.nativeCode) return status
      }
      MaplibreStatus.OK.nativeCode
    } catch (_: InvalidArgumentException) {
      MaplibreStatus.INVALID_ARGUMENT.nativeCode
    } catch (_: IllegalArgumentException) {
      MaplibreStatus.INVALID_ARGUMENT.nativeCode
    } catch (_: Throwable) {
      MaplibreStatus.NATIVE_ERROR.nativeCode
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
      check(activeCallbacks < ACTIVE_MASK) { "too many active resource transform callbacks" }
      if (state.compareAndSet(current, current + 1)) return true
    }
  }

  private fun exitCallback() {
    while (true) {
      val current = state.load()
      val activeCallbacks = current and ACTIVE_MASK
      check(activeCallbacks > 0) { "resource transform callback count underflow" }
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

  private fun setResponseUrl(
    outResponse: CPointer<mln_resource_transform_response>,
    value: String,
  ): Int {
    MemoryUtil.requireValidCString(value)
    return mln_resource_transform_response_set_url(
      outResponse,
      value,
      value.encodeToByteArray().size.toULong(),
    )
  }

  private companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val CLOSING_FLAG = 1 shl 30
    private const val ACTIVE_MASK = CLOSING_FLAG - 1
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun resourceTransformCallback(
  userData: COpaquePointer?,
  kind: UInt,
  url: CPointer<ByteVar>?,
  outResponse: CPointer<mln_resource_transform_response>?,
): Int =
  userData?.asStableRef<ResourceTransformState>()?.get()?.invoke(kind, url, outResponse)
    ?: MaplibreStatus.INVALID_ARGUMENT.nativeCode
