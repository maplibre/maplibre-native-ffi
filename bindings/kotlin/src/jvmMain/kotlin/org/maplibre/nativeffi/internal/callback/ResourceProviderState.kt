package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.invoke.MethodHandles
import java.lang.invoke.MethodType
import org.maplibre.nativeffi.internal.c.mln_resource_provider
import org.maplibre.nativeffi.internal.c.mln_resource_provider_callback
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceRequestHandle

/** Owns runtime-scoped JVM FFM resource provider callback state. */
internal class ResourceProviderState(private val callback: ResourceProviderCallback) :
  AutoCloseable {
  private val arena = Arena.ofShared()
  private val gate = CallbackGate("resource provider callbacks") { arena.close() }
  private val stub: MemorySegment
  private val descriptor: MemorySegment

  init {
    val method =
      MethodHandles.lookup()
        .findVirtual(
          ResourceProviderState::class.java,
          "invoke",
          MethodType.methodType(
            Int::class.javaPrimitiveType,
            MemorySegment::class.java,
            MemorySegment::class.java,
            MemorySegment::class.java,
          ),
        )
        .bindTo(this)
    stub = Linker.nativeLinker().upcallStub(method, callbackDescriptor, arena)
    descriptor = mln_resource_provider.allocate(arena)
    mln_resource_provider.size(descriptor, mln_resource_provider.sizeof().toInt())
    mln_resource_provider.callback(descriptor, stub)
    mln_resource_provider.user_data(descriptor, MemorySegment.NULL)
  }

  fun descriptor(): MemorySegment = descriptor

  @Suppress("UNUSED_PARAMETER")
  fun invoke(userData: MemorySegment, request: MemorySegment, handle: MemorySegment): Int {
    if (request == MemorySegment.NULL || handle == MemorySegment.NULL) return UNKNOWN_DECISION
    val lease = gate.enter() ?: return UNKNOWN_DECISION
    var requestHandle: ResourceRequestHandle? = null
    return try {
      requestHandle = ResourceRequestHandle(handle)
      val decision = callback.handle(NativeAccess.resourceRequest(request), requestHandle)
      requestHandle.finishProviderDecision(decision)
    } catch (_: Throwable) {
      requestHandle?.finishProviderException() ?: UNKNOWN_DECISION
    } finally {
      lease.close()
    }
  }

  fun checkCanClose() = gate.checkCanClose()

  override fun close() = gate.close()

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1

    private val callbackDescriptor = mln_resource_provider_callback.descriptor()
  }
}
