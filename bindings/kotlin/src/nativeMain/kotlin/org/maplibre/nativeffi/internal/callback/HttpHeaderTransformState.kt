package org.maplibre.nativeffi.internal.callback

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
import org.maplibre.nativeffi.internal.c.mln_http_header_transform
import org.maplibre.nativeffi.internal.c.mln_http_header_transform_response
import org.maplibre.nativeffi.internal.c.mln_http_header_transform_response_set
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.memory.toCSize
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.HttpHeaderTransformRequest
import org.maplibre.nativeffi.resource.ResourceKind

@OptIn(ExperimentalForeignApi::class)
internal class HttpHeaderTransformState(private val callback: HttpHeaderTransformCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_http_header_transform>()
  private val gate = CallbackGate("HTTP header transform callbacks") { closeNative() }

  init {
    descriptor.size = kotlinx.cinterop.sizeOf<mln_http_header_transform>().toUInt()
    descriptor.callback = staticCFunction(::httpHeaderTransformCallback)
    descriptor.user_data = selfRef.asCPointer()
    descriptor.release_user_data = staticCFunction(::releaseHttpHeaderTransform)
  }

  fun descriptor(): CPointer<mln_http_header_transform> = descriptor.ptr

  fun invoke(
    rawKind: UInt,
    url: CPointer<ByteVar>?,
    response: CPointer<mln_http_header_transform_response>?,
  ): Int {
    if (response == null) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    val lease = gate.enter() ?: return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    return try {
      response.pointed.size = kotlinx.cinterop.sizeOf<mln_http_header_transform_response>().toUInt()
      val headers =
        callback.transform(
          HttpHeaderTransformRequest(ResourceKind.fromNative(rawKind), MemoryUtil.copyCString(url))
        )
      if (headers.map { it.name.lowercase() }.toSet().size != headers.size) {
        return MaplibreStatus.INVALID_ARGUMENT.nativeCode
      }
      headers.forEach { header ->
        MemoryUtil.requireValidCString(header.name)
        MemoryUtil.requireValidCString(header.value)
        val status =
          mln_http_header_transform_response_set(
            response,
            header.name,
            header.name.encodeToByteArray().size.toCSize(),
            header.value,
            header.value.encodeToByteArray().size.toCSize(),
          )
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
      lease.close()
    }
  }

  override fun close() = gate.close()

  private fun closeNative() {
    selfRef.dispose()
    nativeHeap.free(descriptor.rawPtr)
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun httpHeaderTransformCallback(
  userData: COpaquePointer?,
  kind: UInt,
  url: CPointer<ByteVar>?,
  response: CPointer<mln_http_header_transform_response>?,
): Int =
  userData?.asStableRef<HttpHeaderTransformState>()?.get()?.invoke(kind, url, response)
    ?: MaplibreStatus.INVALID_ARGUMENT.nativeCode

@OptIn(ExperimentalForeignApi::class)
private fun releaseHttpHeaderTransform(userData: COpaquePointer?) {
  userData?.asStableRef<HttpHeaderTransformState>()?.get()?.close()
}
