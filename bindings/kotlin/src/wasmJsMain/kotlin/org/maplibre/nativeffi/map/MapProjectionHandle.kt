package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.CameraMarshal
import org.maplibre.nativeffi.internal.wasm.GeometryMarshal
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.generated.MlnEdgeInsets
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnScreenPoint
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_get_camera
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_lat_lng_for_pixel
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_pixel_for_lat_lng
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_set_camera
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_set_visible_coordinates
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_set_visible_geometry

/**
 * A standalone projection snapshot, owned by the thread the module runs its maps on.
 *
 * A projection is affine to the thread that owns the map it was taken from, which is the thread
 * this binding runs on, so every call here is an ordinary synchronous call as on every other
 * platform.
 */
public actual class MapProjectionHandle
internal constructor(private val handle: NativeMapProjection) : AutoCloseable {
  private val core = HandleStateCore(TYPE_NAME, handle.raw)

  private inline fun <T> live(body: () -> T): T {
    core.requireLive()
    return body()
  }

  public actual val camera: CameraOptions
    get() = live {
      Heap.withScratch(CameraMarshal.SIZEOF) { out ->
        // An output descriptor states its own size too: native reads it to decide which fields it
        // may write, and a zeroed block would ask for a zero-sized camera.
        CameraMarshal.writeHeader(out)
        Status.check(mln_map_projection_get_camera(handle.raw, out.address))
        CameraMarshal.read(out)
      }
    }

  public actual fun setCamera(camera: CameraOptions) {
    live {
      Heap.withScratch(CameraMarshal.SIZEOF) { descriptor ->
        CameraMarshal.write(descriptor, camera)
        Status.check(mln_map_projection_set_camera(handle.raw, descriptor.address))
      }
    }
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets) {
    live {
      // The coordinates and the padding share one block, so this costs one scratch acquisition
      // rather than two.
      val coordinateBytes = Heap.sizeOf(MlnLatLng.SIZEOF, coordinates.size)
      Heap.withScratch(coordinateBytes + MlnEdgeInsets.SIZEOF) { scratch ->
        coordinates.forEachIndexed { index, coordinate ->
          CameraMarshal.writeLatLng(scratch + index * MlnLatLng.SIZEOF, coordinate)
        }
        val insets = scratch + coordinateBytes
        CameraMarshal.writeEdgeInsets(insets, padding)
        Status.check(
          mln_map_projection_set_visible_coordinates(
            handle.raw,
            scratch.address,
            coordinates.size,
            insets.address,
          )
        )
      }
    }
  }

  public actual fun setVisibleGeometry(geometry: Geometry, padding: EdgeInsets) {
    live {
      // Measured before the block is taken. A geometry tree is many nested spans, and the arena
      // carves them out of one allocation rather than taking one per node.
      val geometryBytes = GeometryMarshal.measure(geometry)
      Heap.withScratch(geometryBytes + MlnEdgeInsets.SIZEOF) { scratch ->
        val root = GeometryMarshal.write(HeapArena(scratch, geometryBytes), geometry)
        val insets = scratch + geometryBytes
        CameraMarshal.writeEdgeInsets(insets, padding)
        Status.check(
          mln_map_projection_set_visible_geometry(handle.raw, root.address, insets.address)
        )
      }
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint = live {
    Heap.withScratch(MlnLatLng.SIZEOF + MlnScreenPoint.SIZEOF) { scratch ->
      val out = scratch + MlnLatLng.SIZEOF
      CameraMarshal.writeLatLng(scratch, coordinate)
      Status.check(mln_map_projection_pixel_for_lat_lng(handle.raw, scratch.address, out.address))
      ScreenPoint(MlnScreenPoint.x(out), MlnScreenPoint.y(out))
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng = live {
    Heap.withScratch(MlnScreenPoint.SIZEOF + MlnLatLng.SIZEOF) { scratch ->
      val out = scratch + MlnScreenPoint.SIZEOF
      MlnScreenPoint.setX(scratch, point.x)
      MlnScreenPoint.setY(scratch, point.y)
      Status.check(mln_map_projection_lat_lng_for_pixel(handle.raw, scratch.address, out.address))
      CameraMarshal.readLatLng(out)
    }
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce(destroy = { mln_map_projection_destroy(handle.raw) })
  }

  internal companion object {
    private const val TYPE_NAME = "MapProjectionHandle"

    fun fromNative(handle: NativeMapProjection): MapProjectionHandle = MapProjectionHandle(handle)
  }
}
