package org.maplibre.nativeffi.map

import cnames.structs.mln_map_projection
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_map_projection_create
import org.maplibre.nativeffi.internal.c.mln_map_projection_destroy
import org.maplibre.nativeffi.internal.c.mln_map_projection_get_camera
import org.maplibre.nativeffi.internal.c.mln_map_projection_set_camera
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.MapStructs

/** Owned standalone projection snapshot created from a map. */
@OptIn(ExperimentalForeignApi::class)
public class MapProjectionHandle private constructor(handle: CPointer<mln_map_projection>) :
  AutoCloseable {
  private val state = HandleState("MapProjectionHandle", handle)

  public fun camera(): CameraOptions = memScoped {
    val outCamera = mln_camera_options_default().getPointer(this)
    Status.check(mln_map_projection_get_camera(state.requireLive(), outCamera))
    MapStructs.cameraOptions(outCamera.pointed)
  }

  public fun setCamera(camera: CameraOptions) {
    memScoped {
      Status.check(
        mln_map_projection_set_camera(state.requireLive(), MapStructs.cameraOptions(camera, this))
      )
    }
  }

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
