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

/** Owns runtime-scoped resource transform callback state. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
internal class ResourceTransformState(private val callback: ResourceTransformCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_resource_transform>()
  private val responseUrlLock = AtomicInt(0)
  private val responseUrls = mutableListOf<CPointer<ByteVar>>()
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
    return try {
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
        retainResponseUrl(responseUrl)
        outResponse.pointed.url = responseUrl
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
    clearResponseUrls()
    selfRef.dispose()
    nativeHeap.free(descriptor.rawPtr)
  }

  private fun retainResponseUrl(pointer: CPointer<ByteVar>) {
    withResponseUrlLock {
      responseUrls += pointer
      // The C API copies a replacement URL during the current transform invocation and bindings
      // usually retain per-thread storage until a later callback. Kotlin/Native has no portable
      // thread identity API across all native targets, so keep a bounded retirement window instead
      // of retaining every transformed URL until runtime teardown.
      while (responseUrls.size > MAX_RETAINED_RESPONSE_URLS) {
        nativeHeap.free(responseUrls.removeAt(0).rawValue)
      }
    }
  }

  private fun clearResponseUrls() {
    val urls = withResponseUrlLock {
      val copy = responseUrls.toList()
      responseUrls.clear()
      copy
    }
    urls.forEach { nativeHeap.free(it.rawValue) }
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

  private companion object {
    const val MAX_RETAINED_RESPONSE_URLS = 64
  }

  private fun allocateCString(value: String): CPointer<ByteVar> {
    MemoryUtil.requireValidCString(value)
    val bytes = value.encodeToByteArray()
    val pointer = nativeHeap.allocArray<ByteVar>(bytes.size + 1)
    bytes.forEachIndexed { index, byte -> pointer[index] = byte }
    pointer[bytes.size] = 0
    return pointer
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
