package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import java.lang.invoke.MethodHandles
import java.lang.invoke.MethodType
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_transform
import org.maplibre.nativeffi.internal.c.mln_resource_transform_callback
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceTransformCallback
import org.maplibre.nativeffi.resource.ResourceTransformRequest

/** Owns runtime-scoped JVM FFM resource transform callback state. */
internal class ResourceTransformState(private val callback: ResourceTransformCallback) :
  AutoCloseable {
  private val arena = Arena.ofShared()
  private val gate = CallbackGate("resource transform callbacks") { arena.close() }
  private val stub: MemorySegment
  private val descriptor: MemorySegment

  init {
    val method =
      MethodHandles.lookup()
        .findVirtual(
          ResourceTransformState::class.java,
          "invoke",
          MethodType.methodType(
            Int::class.javaPrimitiveType,
            MemorySegment::class.java,
            Int::class.javaPrimitiveType,
            MemorySegment::class.java,
            MemorySegment::class.java,
          ),
        )
        .bindTo(this)
    stub = Linker.nativeLinker().upcallStub(method, callbackDescriptor, arena)
    descriptor = mln_resource_transform.allocate(arena)
    mln_resource_transform.size(descriptor, mln_resource_transform.sizeof().toInt())
    mln_resource_transform.callback(descriptor, stub)
    mln_resource_transform.user_data(descriptor, MemorySegment.NULL)
  }

  fun descriptor(): MemorySegment = descriptor

  fun invoke(
    userData: MemorySegment,
    rawKind: Int,
    url: MemorySegment,
    outResponse: MemorySegment,
  ): Int {
    if (outResponse == MemorySegment.NULL) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    val lease = gate.enter() ?: return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    return try {
      mln_resource_transform_response.size(
        outResponse,
        mln_resource_transform_response.sizeof().toInt(),
      )
      mln_resource_transform_response.url(outResponse, MemorySegment.NULL)
      val replacement =
        callback.transform(
          ResourceTransformRequest(ResourceKind.fromNative(rawKind), copyCString(url))
        )
      if (!replacement.isNullOrEmpty()) {
        if ('\u0000' in replacement) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
        return NativeAccess.setResourceTransformResponseUrl(outResponse, replacement)
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

  override fun close() = gate.close()

  private fun copyCString(address: MemorySegment): String {
    if (address == MemorySegment.NULL) {
      return ""
    }
    var length = 0L
    while (address.reinterpret(length + 1).get(ValueLayout.JAVA_BYTE, length) != 0.toByte()) {
      length++
    }
    return String(address.reinterpret(length).toArray(ValueLayout.JAVA_BYTE), Charsets.UTF_8)
  }

  private companion object {
    private val callbackDescriptor = mln_resource_transform_callback.descriptor()
  }
}
