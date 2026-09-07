package org.maplibre.nativeffi.map

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.invoke.MethodHandles
import java.lang.invoke.MethodType
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_release_callback
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_tile_callback
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions

/**
 * Owns map/style-scoped custom geometry source callback state.
 *
 * Native invokes [onReleased] after it stops referencing this state, which drops it from its map's
 * registry and closes it.
 */
internal class CustomGeometrySourceState(
  private val options: CustomGeometrySourceOptions,
  private val onReleased: () -> Unit,
) : AutoCloseable {
  private val arena = Arena.ofShared()
  private val token = TOKENS.getAndIncrement()
  private val gate =
    CallbackGate("custom geometry callbacks") {
      STATES.remove(token)
      arena.close()
    }
  private val fetchTileStub: MemorySegment = upcall("fetchTile")
  private val cancelTileStub: MemorySegment = upcall("cancelTile")
  private val descriptor =
    NativeAccess.customGeometrySourceOptions(
      arena,
      options,
      fetchTileStub,
      cancelTileStub,
      RELEASE_STUB,
      MemorySegment.ofAddress(token),
    )

  init {
    STATES[token] = this
  }

  fun descriptor(): MemorySegment = descriptor

  @Suppress("UNUSED_PARAMETER")
  private fun fetchTile(userData: MemorySegment, tileId: MemorySegment) {
    fetchTileForTesting(NativeAccess.canonicalTileId(tileId))
  }

  internal fun fetchTileForTesting(tileId: org.maplibre.nativeffi.geo.CanonicalTileId) {
    val lease = gate.enter() ?: return
    try {
      options.callback.fetchTile(tileId)
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

  internal fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

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
    private val TOKENS = AtomicLong(1)

    /** The live states by token, which is the `user_data` this binding hands to native. */
    private val STATES = ConcurrentHashMap<Long, CustomGeometrySourceState>()

    /**
     * One process-wide release stub, so releasing a state can close that state's own arena. A
     * per-state stub would live in the arena it has to free.
     */
    private val RELEASE_STUB: MemorySegment =
      mln_custom_geometry_source_release_callback.allocate(
        { userData -> releaseState(userData.address()) },
        Arena.global(),
      )

    private fun releaseState(token: Long) {
      try {
        STATES[token]?.onReleased?.invoke()
      } catch (_: Throwable) {
        // Native callbacks must not unwind through the C ABI.
      }
    }
  }
}
