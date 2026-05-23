package org.maplibre.nativeffi.internal.callback

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
@OptIn(ExperimentalForeignApi::class)
internal class ResourceTransformState(private val callback: ResourceTransformCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_resource_transform>()
  private var responseUrl: CPointer<ByteVar>? = null
  private var closed = false

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
    if (closed || outResponse == null) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    return try {
      clearResponseUrl()
      outResponse.pointed.size = kotlinx.cinterop.sizeOf<mln_resource_transform_response>().toUInt()
      outResponse.pointed.url = null
      val request =
        ResourceTransformRequest(
          ResourceKind.fromNative(rawKind),
          rawKind,
          MemoryUtil.copyCString(url),
        )
      val replacement = callback.transform(request)
      if (!replacement.isNullOrEmpty()) {
        responseUrl = allocateCString(replacement)
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
    if (closed) return
    closed = true
    clearResponseUrl()
    selfRef.dispose()
    nativeHeap.free(descriptor.rawPtr)
  }

  private fun clearResponseUrl() {
    responseUrl?.let { nativeHeap.free(it.rawValue) }
    responseUrl = null
  }

  private fun allocateCString(value: String): CPointer<ByteVar> {
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
