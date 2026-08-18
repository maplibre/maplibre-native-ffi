package org.maplibre.nativeffi.internal.callback

import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong
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
  private val token = TOKENS.getAndIncrement()
  private val gate = CallbackGate("resource provider callbacks") { closeNative() }
  private val provider = MaplibreNativeC.mln_resource_provider()

  init {
    provider.size(provider.sizeof())
    provider.callback(NATIVE_CALLBACK)
    provider.user_data(JavaCppSupport.addressPointer(token))
    STATES[token] = this
  }

  fun descriptor(): MaplibreNativeC.mln_resource_provider = provider

  fun invoke(request: MaplibreNativeC.mln_resource_request?, handle: Long): Int {
    if (request == null || handle == 0L) return UNKNOWN_DECISION
    val lease = gate.enter() ?: return UNKNOWN_DECISION
    var requestHandle: ResourceRequestHandle? = null
    return try {
      requestHandle = ResourceRequestHandle(handle)
      val decision = callback.handle(resourceRequest(request), requestHandle)
      requestHandle.finishProviderDecision(decision)
    } catch (_: Throwable) {
      requestHandle?.finishProviderException() ?: UNKNOWN_DECISION
    } finally {
      lease.close()
    }
  }

  fun checkCanClose() = gate.checkCanClose()

  fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  override fun close() = gate.close()

  private fun closeNative() {
    STATES.remove(token)
    provider.close()
  }

  private fun resourceRequest(request: MaplibreNativeC.mln_resource_request): ResourceRequest =
    ResourceRequest(
      JavaCppSupport.cString(request.requested_url()),
      JavaCppSupport.cString(request.resolved_url()),
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
    private val TOKENS = AtomicLong(1)
    private val STATES = ConcurrentHashMap<Long, ResourceProviderState>()

    /**
     * One process-wide thunk. JavaCPP's FunctionPointer pool is ten slots per generated class, so
     * per-runtime thunks ran out at eleven live runtimes.
     */
    private val NATIVE_CALLBACK =
      object : MaplibreNativeC.mln_resource_provider_callback() {
        override fun call(
          userData: Pointer?,
          request: MaplibreNativeC.mln_resource_request?,
          handle: Long,
        ): Int = STATES[userData?.address() ?: 0L]?.invoke(request, handle) ?: UNKNOWN_DECISION
      }
  }
}
