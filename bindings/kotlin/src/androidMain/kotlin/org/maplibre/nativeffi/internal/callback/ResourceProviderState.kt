package org.maplibre.nativeffi.internal.callback

import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceLoadingMethod
import org.maplibre.nativeffi.resource.ResourcePriority
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceRequest
import org.maplibre.nativeffi.resource.ResourceRequestHandle
import org.maplibre.nativeffi.resource.ResourceStoragePolicy
import org.maplibre.nativeffi.resource.ResourceUsage

/** Owns runtime-scoped Android JNI resource provider callback state. */
internal class ResourceProviderState(private val callback: ResourceProviderCallback) :
  AutoCloseable {
  private val gate = CallbackGate("resource provider callbacks") { closeNative() }
  private val nativeCallback =
    object : MaplibreNativeC.mln_resource_provider_callback() {
      override fun call(
        userData: Pointer?,
        request: MaplibreNativeC.mln_resource_request?,
        handle: MaplibreNativeC.mln_resource_request_handle?,
      ): Int = invoke(request, handle)
    }
  private val provider = MaplibreNativeC.mln_resource_provider()

  init {
    provider.size(provider.sizeof())
    provider.callback(nativeCallback)
    provider.user_data(null)
  }

  fun descriptor(): MaplibreNativeC.mln_resource_provider = provider

  fun invoke(
    request: MaplibreNativeC.mln_resource_request?,
    handle: MaplibreNativeC.mln_resource_request_handle?,
  ): Int {
    if (request == null || handle == null || handle.isNull) return UNKNOWN_DECISION
    val lease = gate.enter() ?: return UNKNOWN_DECISION
    var requestHandle: ResourceRequestHandle? = null
    return try {
      requestHandle = ResourceRequestHandle(handle.address())
      val decision = callback.handle(resourceRequest(request), requestHandle)
      requestHandle.finishProviderDecision(decision)
    } catch (_: Throwable) {
      requestHandle?.finishProviderException() ?: UNKNOWN_DECISION
    } finally {
      lease.close()
    }
  }

  fun checkCanClose() = gate.checkCanClose()

  override fun close() = gate.close()

  private fun closeNative() {
    provider.close()
    nativeCallback.close()
  }

  private fun resourceRequest(request: MaplibreNativeC.mln_resource_request): ResourceRequest =
    ResourceRequest(
      JavaCppSupport.cString(request.url()),
      ResourceKind.fromNative(request.kind()),
      ResourceLoadingMethod.fromNative(request.loading_method()),
      ResourcePriority.fromNative(request.priority()),
      ResourceUsage.fromNative(request.usage()),
      ResourceStoragePolicy.fromNative(request.storage_policy()),
      if (request.has_range()) ResourceRequest.ByteRange(request.range_start(), request.range_end())
      else null,
      if (request.has_prior_modified()) request.prior_modified_unix_ms() else null,
      if (request.has_prior_expires()) request.prior_expires_unix_ms() else null,
      optionalCString(request.prior_etag()),
      JavaCppSupport.byteArray(request.prior_data(), request.prior_data_size()),
    )

  private fun optionalCString(pointer: BytePointer?): String? =
    if (pointer == null || pointer.isNull) null else JavaCppSupport.cString(pointer)

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1
  }
}
