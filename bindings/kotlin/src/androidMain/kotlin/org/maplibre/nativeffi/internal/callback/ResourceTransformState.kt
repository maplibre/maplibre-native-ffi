package org.maplibre.nativeffi.internal.callback

import java.nio.charset.StandardCharsets
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceTransformCallback
import org.maplibre.nativeffi.resource.ResourceTransformRequest

/** Owns runtime-scoped Android JNI resource transform callback state. */
internal class ResourceTransformState(private val callback: ResourceTransformCallback) :
  AutoCloseable {
  private val token = TOKENS.getAndIncrement()
  private val gate = CallbackGate("resource transform callbacks") { closeNative() }
  private val transform = MaplibreNativeC.mln_resource_transform()

  init {
    transform.size(transform.sizeof())
    transform.callback(NATIVE_CALLBACK)
    transform.user_data(JavaCppSupport.addressPointer(token))
    transform.release_user_data(NATIVE_RELEASE)
    STATES[token] = this
  }

  fun descriptor(): MaplibreNativeC.mln_resource_transform = transform

  fun invoke(
    rawKind: Int,
    url: BytePointer?,
    response: MaplibreNativeC.mln_resource_transform_response?,
  ): Int {
    if (response == null) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    val lease = gate.enter() ?: return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    return try {
      response.size(response.sizeof())
      response.url(null)
      val replacement =
        callback.transform(
          ResourceTransformRequest(ResourceKind.fromNative(rawKind), JavaCppSupport.cString(url))
        )
      if (!replacement.isNullOrEmpty()) {
        if ('\u0000' in replacement) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
        return setResponseUrl(response, replacement)
      }
      MaplibreStatus.OK.nativeCode
    } catch (_: IllegalArgumentException) {
      MaplibreStatus.INVALID_ARGUMENT.nativeCode
    } catch (_: Throwable) {
      MaplibreStatus.NATIVE_ERROR.nativeCode
    } finally {
      lease.close()
    }
  }

  fun checkCanClose() = gate.checkCanClose()

  fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  override fun close() = gate.close()

  private fun closeNative() {
    STATES.remove(token)
    transform.close()
  }

  private fun setResponseUrl(
    response: MaplibreNativeC.mln_resource_transform_response,
    value: String,
  ): Int {
    val bytes = value.toByteArray(StandardCharsets.UTF_8)
    BytePointer(bytes.size.toLong()).use { storage ->
      storage.put(bytes, 0, bytes.size)
      return MaplibreNativeC.mln_resource_transform_response_set_url(
        response,
        storage,
        bytes.size.toLong(),
      )
    }
  }

  private companion object {
    private val TOKENS = AtomicLong(1)
    private val STATES = ConcurrentHashMap<Long, ResourceTransformState>()

    /**
     * One process-wide thunk. JavaCPP's FunctionPointer pool is ten slots per generated class, so
     * per-runtime thunks ran out at eleven live runtimes.
     */
    private val NATIVE_CALLBACK =
      object : MaplibreNativeC.mln_resource_transform_callback() {
        override fun call(
          userData: Pointer?,
          kind: Int,
          url: BytePointer?,
          response: MaplibreNativeC.mln_resource_transform_response?,
        ): Int =
          STATES[userData?.address() ?: 0L]?.invoke(kind, url, response)
            ?: MaplibreStatus.INVALID_ARGUMENT.nativeCode
      }

    /** One process-wide release thunk; per-state thunks would exhaust the same pool. */
    private val NATIVE_RELEASE =
      object : MaplibreNativeC.mln_runtime_callback_release() {
        override fun call(userData: Pointer?) {
          val state = STATES[userData?.address() ?: 0L] ?: return
          HandleLeakCleaner.releaseNativeCallbackRoot(state)
          state.close()
        }
      }
  }
}
