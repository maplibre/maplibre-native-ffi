package org.maplibre.nativeffi.internal.callback

import cnames.structs.mln_resource_request_handle
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
@OptIn(ExperimentalForeignApi::class)
internal class ResourceProviderState(private val callback: ResourceProviderCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_resource_provider>()
  private var closed = false

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
    if (closed || request == null || handle == null) return UInt.MAX_VALUE
    val requestHandle = ResourceRequestHandle(handle)
    return try {
      val decision =
        callback.handle(ResourceStructs.resourceRequest(request.pointed), requestHandle)
      requestHandle.finishProviderDecision(decision)
    } catch (_: Throwable) {
      requestHandle.finishProviderException()
    }
  }

  override fun close() {
    if (closed) return
    closed = true
    selfRef.dispose()
    nativeHeap.free(descriptor.rawPtr)
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun resourceProviderCallback(
  userData: COpaquePointer?,
  request: CPointer<org.maplibre.nativeffi.internal.c.mln_resource_request>?,
  handle: CPointer<mln_resource_request_handle>?,
): UInt =
  userData?.asStableRef<ResourceProviderState>()?.get()?.invoke(request, handle) ?: UInt.MAX_VALUE
