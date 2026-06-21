package org.maplibre.nativeffi.map

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.invoke.MethodHandles
import java.lang.invoke.MethodType
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_tile_callback
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions

/** Owns map/style-scoped custom geometry source callback state. */
internal class CustomGeometrySourceState(private val options: CustomGeometrySourceOptions) :
  AutoCloseable {
  private val arena = Arena.ofShared()
  private val gate = CallbackGate("custom geometry callbacks") { arena.close() }
  private val fetchTileStub: MemorySegment = upcall("fetchTile")
  private val cancelTileStub: MemorySegment = upcall("cancelTile")
  private val descriptor =
    NativeAccess.customGeometrySourceOptions(arena, options, fetchTileStub, cancelTileStub)

  fun descriptor(): MemorySegment = descriptor

  @Suppress("UNUSED_PARAMETER")
  private fun fetchTile(userData: MemorySegment, tileId: MemorySegment) {
    val lease = gate.enter() ?: return
    try {
      options.callback.fetchTile(NativeAccess.canonicalTileId(tileId))
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  @Suppress("UNUSED_PARAMETER")
  private fun cancelTile(userData: MemorySegment, tileId: MemorySegment) {
    val lease = gate.enter() ?: return
    try {
      options.callback.cancelTile(NativeAccess.canonicalTileId(tileId))
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  override fun close() {
    gate.close()
  }

  private fun upcall(methodName: String): MemorySegment {
    val method =
      LOOKUP.findVirtual(
          CustomGeometrySourceState::class.java,
          methodName,
          MethodType.methodType(Void.TYPE, MemorySegment::class.java, MemorySegment::class.java),
        )
        .bindTo(this)
    return LINKER.upcallStub(method, CALLBACK_DESCRIPTOR, arena)
  }

  private companion object {
    private val LOOKUP = MethodHandles.lookup()
    private val LINKER = java.lang.foreign.Linker.nativeLinker()
    private val CALLBACK_DESCRIPTOR = mln_custom_geometry_source_tile_callback.descriptor()
  }
}
