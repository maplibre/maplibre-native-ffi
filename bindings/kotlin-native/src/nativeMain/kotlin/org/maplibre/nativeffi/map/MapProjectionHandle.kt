package org.maplibre.nativeffi.map

import cnames.structs.mln_map_projection
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.mln_map_projection_create
import org.maplibre.nativeffi.internal.c.mln_map_projection_destroy
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.status.Status

/** Owned standalone projection snapshot created from a map. */
@OptIn(ExperimentalForeignApi::class)
public class MapProjectionHandle private constructor(handle: CPointer<mln_map_projection>) :
  AutoCloseable {
  private val state = HandleState("MapProjectionHandle", handle)

  override fun close() {
    state.closeOnce(::mln_map_projection_destroy)
  }

  public fun isClosed(): Boolean = state.isReleased()

  internal fun nativeHandle(): CPointer<mln_map_projection> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  public companion object {
    public fun create(map: MapHandle): MapProjectionHandle = memScoped {
      val outProjection = alloc<CPointerVarOf<CPointer<mln_map_projection>>>()
      outProjection.value = null
      Status.check(mln_map_projection_create(map.nativeHandle(), outProjection.ptr))
      MapProjectionHandle(
        requireNotNull(outProjection.value) { "mln_map_projection_create returned null" }
      )
    }
  }
}
