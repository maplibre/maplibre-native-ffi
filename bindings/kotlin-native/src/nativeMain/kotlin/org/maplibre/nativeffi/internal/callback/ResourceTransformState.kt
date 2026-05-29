package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.set
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_transform
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceTransformCallback
import org.maplibre.nativeffi.resource.ResourceTransformRequest
import platform.posix.pthread_self

/** Owns runtime-scoped resource transform callback state. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
internal class ResourceTransformState(private val callback: ResourceTransformCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_resource_transform>()
  private val responseUrlLock = AtomicInt(0)
  private val responseUrls = mutableMapOf<String, CPointer<ByteVar>>()
  private val closed = AtomicInt(0)

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
    if (closed.load() != 0 || outResponse == null) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    val threadKey = currentThreadKey()
    return try {
      clearResponseUrl(threadKey)
      outResponse.pointed.size = kotlinx.cinterop.sizeOf<mln_resource_transform_response>().toUInt()
      outResponse.pointed.url = null
      val request =
        ResourceTransformRequest(
          ResourceKind.fromNative(rawKind),
          rawKind.toInt(),
          MemoryUtil.copyCString(url),
        )
      val replacement = callback.transform(request)
      if (!replacement.isNullOrEmpty()) {
        val responseUrl = allocateCString(replacement)
        if (retainResponseUrl(threadKey, responseUrl)) {
          outResponse.pointed.url = responseUrl
        } else {
          nativeHeap.free(responseUrl.rawValue)
          return MaplibreStatus.INVALID_ARGUMENT.nativeCode
        }
      }
      MaplibreStatus.OK.nativeCode
    } catch (_: IllegalArgumentException) {
      MaplibreStatus.INVALID_ARGUMENT.nativeCode
    } catch (_: Throwable) {
      MaplibreStatus.NATIVE_ERROR.nativeCode
    }
  }

  override fun close() {
    if (!closed.compareAndSet(0, 1)) return
    clearAllResponseUrls()
    selfRef.dispose()
    nativeHeap.free(descriptor.rawPtr)
  }

  private fun retainResponseUrl(threadKey: String, pointer: CPointer<ByteVar>): Boolean =
    withResponseUrlLock {
      if (closed.load() != 0) {
        false
      } else {
        responseUrls.remove(threadKey)?.let { nativeHeap.free(it.rawValue) }
        responseUrls[threadKey] = pointer
        true
      }
    }

  private fun clearResponseUrl(threadKey: String) {
    withResponseUrlLock { responseUrls.remove(threadKey) }?.let { nativeHeap.free(it.rawValue) }
  }

  private fun clearAllResponseUrls() {
    val urls = withResponseUrlLock {
      val copy = responseUrls.toList()
      responseUrls.clear()
      copy
    }
    urls.forEach { nativeHeap.free(it.second.rawValue) }
  }

  private inline fun <T> withResponseUrlLock(block: () -> T): T {
    while (!responseUrlLock.compareAndSet(0, 1)) {
      // Callback results can arrive from worker threads; protect native URL storage bookkeeping.
    }
    try {
      return block()
    } finally {
      responseUrlLock.store(0)
    }
  }

  private fun allocateCString(value: String): CPointer<ByteVar> {
    MemoryUtil.requireValidCString(value)
    val bytes = value.encodeToByteArray()
    val pointer = nativeHeap.allocArray<ByteVar>(bytes.size + 1)
    bytes.forEachIndexed { index, byte -> pointer[index] = byte }
    pointer[bytes.size] = 0
    return pointer
  }

  private fun currentThreadKey(): String = pthread_self().toString()
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
