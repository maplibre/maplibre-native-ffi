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
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.internal.wasm.GeometryMarshal
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.generated.MlnEdgeInsets
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnScreenPoint

/**
 * A standalone projection snapshot, owned by the thread the module runs its maps on.
 *
 * Every call here is placed on that thread rather than run on the page. A projection is affine to
 * the thread that owns the map it was taken from, and the C API reports an owner-thread status for
 * a call from anywhere else, so the dispatcher is what lets this keep the ordinary synchronous
 * shape the other platforms have.
 */
public actual class MapProjectionHandle
internal constructor(private val handle: NativeMapProjection) : AutoCloseable {
  private val core = HandleStateCore("MapProjectionHandle", handle.raw)

  /**
   * Checks this handle is live and then runs [body], without holding a use count across it.
   *
   * `withLive` would hold one, and every call here parks the Kotlin stack while the owner thread
   * works. A close arriving during that park would drain a count that cannot be released until the
   * park ends, which is the invariant `yieldWhileClosing` refuses to spin on. The window this
   * leaves is the one the C API already closes: a handle destroyed between the check and the call
   * is a stale handle, and native reports invalid argument for it.
   */
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
        call("mln_map_projection_get_camera", out)
        CameraMarshal.read(out)
      }
    }

  public actual fun setCamera(camera: CameraOptions) {
    live {
      Heap.withScratch(CameraMarshal.SIZEOF) { descriptor ->
        CameraMarshal.write(descriptor, camera)
        call("mln_map_projection_set_camera", descriptor)
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
        Dispatcher.call(
          "mln_map_projection_set_visible_coordinates",
          4,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, scratch)
            slots.setInt(2, coordinates.size)
            slots.setPointer(3, insets)
          },
          { Status.check(Heap.loadInt(it)) },
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
        Dispatcher.call(
          "mln_map_projection_set_visible_geometry",
          3,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, root)
            slots.setPointer(2, insets)
          },
          { Status.check(Heap.loadInt(it)) },
        )
      }
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint = live {
    Heap.withScratch(MlnLatLng.SIZEOF + MlnScreenPoint.SIZEOF) { scratch ->
      val out = scratch + MlnLatLng.SIZEOF
      CameraMarshal.writeLatLng(scratch, coordinate)
      convert("mln_map_projection_pixel_for_lat_lng", scratch, out)
      ScreenPoint(MlnScreenPoint.x(out), MlnScreenPoint.y(out))
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng = live {
    Heap.withScratch(MlnScreenPoint.SIZEOF + MlnLatLng.SIZEOF) { scratch ->
      val out = scratch + MlnScreenPoint.SIZEOF
      MlnScreenPoint.setX(scratch, point.x)
      MlnScreenPoint.setY(scratch, point.y)
      convert("mln_map_projection_lat_lng_for_pixel", scratch, out)
      CameraMarshal.readLatLng(out)
    }
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce(
      destroy = {
        Dispatcher.call(
          "mln_map_projection_destroy",
          1,
          { slots -> slots.setLong(0, handle.raw) },
          { Heap.loadInt(it) },
        )
      }
    )
  }

  /** One handle argument and one descriptor, which is the shape most of these calls take. */
  private fun call(name: String, descriptor: HeapPointer) {
    Dispatcher.call(
      name,
      2,
      { slots ->
        slots.setLong(0, handle.raw)
        slots.setPointer(1, descriptor)
      },
      { Status.check(Heap.loadInt(it)) },
    )
  }

  /** One handle argument, one input descriptor, and one output descriptor. */
  private fun convert(name: String, input: HeapPointer, output: HeapPointer) {
    Dispatcher.call(
      name,
      3,
      { slots ->
        slots.setLong(0, handle.raw)
        slots.setPointer(1, input)
        slots.setPointer(2, output)
      },
      { Status.check(Heap.loadInt(it)) },
    )
  }

  internal companion object {
    fun fromNative(handle: NativeMapProjection): MapProjectionHandle = MapProjectionHandle(handle)
  }
}
