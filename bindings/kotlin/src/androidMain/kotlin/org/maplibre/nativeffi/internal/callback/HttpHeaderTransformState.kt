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
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.HttpHeaderTransformRequest
import org.maplibre.nativeffi.resource.ResourceKind

internal class HttpHeaderTransformState(private val callback: HttpHeaderTransformCallback) :
  AutoCloseable {
  private val token = TOKENS.getAndIncrement()
  private val gate = CallbackGate("HTTP header transform callbacks") { closeNative() }
  private val transform = MaplibreNativeC.mln_http_header_transform()

  init {
    transform.size(transform.sizeof())
    transform.callback(NATIVE_CALLBACK)
    transform.user_data(JavaCppSupport.addressPointer(token))
    transform.release_user_data(NATIVE_RELEASE)
    STATES[token] = this
  }

  fun descriptor(): MaplibreNativeC.mln_http_header_transform = transform

  fun invoke(
    rawKind: Int,
    url: BytePointer?,
    response: MaplibreNativeC.mln_http_header_transform_response?,
  ): Int {
    if (response == null) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    val lease = gate.enter() ?: return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    return try {
      response.size(response.sizeof())
      val headers =
        callback.transform(
          HttpHeaderTransformRequest(ResourceKind.fromNative(rawKind), JavaCppSupport.cString(url))
        )
      if (headers.map { it.name.lowercase() }.toSet().size != headers.size) {
        return MaplibreStatus.INVALID_ARGUMENT.nativeCode
      }
      headers.forEach { header ->
        if ('\u0000' in header.name || '\u0000' in header.value) {
          return MaplibreStatus.INVALID_ARGUMENT.nativeCode
        }
        val status = setResponseHeader(response, header.name, header.value)
        if (status != MaplibreStatus.OK.nativeCode) return status
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

  override fun close() = gate.close()

  private fun closeNative() {
    STATES.remove(token)
    transform.close()
  }

  private fun setResponseHeader(
    response: MaplibreNativeC.mln_http_header_transform_response,
    name: String,
    value: String,
  ): Int {
    val nameBytes = name.toByteArray(StandardCharsets.UTF_8)
    val valueBytes = value.toByteArray(StandardCharsets.UTF_8)
    BytePointer(nameBytes.size.toLong()).use { nameStorage ->
      BytePointer(valueBytes.size.toLong()).use { valueStorage ->
        nameStorage.put(nameBytes, 0, nameBytes.size)
        valueStorage.put(valueBytes, 0, valueBytes.size)
        return MaplibreNativeC.mln_http_header_transform_response_set(
          response,
          nameStorage,
          nameBytes.size.toLong(),
          valueStorage,
          valueBytes.size.toLong(),
        )
      }
    }
  }

  private companion object {
    private val TOKENS = AtomicLong(1)
    private val STATES = ConcurrentHashMap<Long, HttpHeaderTransformState>()

    /**
     * One process-wide thunk. JavaCPP's FunctionPointer pool is ten slots per generated class, so
     * per-runtime thunks ran out at eleven live runtimes.
     */
    private val NATIVE_CALLBACK =
      object : MaplibreNativeC.mln_http_header_transform_callback() {
        override fun call(
          userData: Pointer?,
          kind: Int,
          url: BytePointer?,
          response: MaplibreNativeC.mln_http_header_transform_response?,
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
