package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnCoordinateSpan
import org.maplibre.nativeffi.internal.wasm.generated.MlnGeometry
import org.maplibre.nativeffi.internal.wasm.generated.MlnGeometryCollection
import org.maplibre.nativeffi.internal.wasm.generated.MlnGeometryType
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnMultiLineGeometry
import org.maplibre.nativeffi.internal.wasm.generated.MlnMultiPolygonGeometry
import org.maplibre.nativeffi.internal.wasm.generated.MlnPolygonGeometry

/**
 * Places a [Geometry] tree into the Emscripten heap.
 *
 * The C descriptor is a tagged union whose arms point at spans that point at further spans, and
 * every one of those pointers has to address memory native can read. So the tree is measured first,
 * placed in one arena, and handed to native as a single root pointer — which also means one
 * allocation and one release however deep the tree goes.
 *
 * Measuring and writing walk the same shape in the same order. They are written next to each other
 * for that reason: a change to one that is not made to the other is what the arena's own bounds
 * check reports.
 */
internal object GeometryMarshal {
  private const val POINTER_ALIGN = 4
  private const val COORDINATE_ALIGN = 8

  /**
   * Bytes [geometry] needs, including its root descriptor.
   *
   * Every addition and every element count is bounded as it is taken rather than only at the end. A
   * subtotal that wrapped would produce a small positive size that passed both this check and the
   * arena's, and the write that followed would run past the block.
   */
  fun measure(geometry: Geometry): Int =
    plus(MlnGeometry.SIZEOF.toLong(), measurePayload(geometry, 0)).toInt()

  /** Adds two measured sizes, refusing a total the heap could not address. */
  private fun plus(left: Long, right: Long): Long {
    val total = left + right
    if (total > Int.MAX_VALUE || total < 0) {
      throw Status.invalidArgument("geometry is too large to place in the module's heap")
    }
    return total
  }

  /** Sizes an array of [count] elements, refusing one the heap could not address. */
  private fun sizeOf(elementBytes: Int, count: Int): Long {
    Status.requireArgument(count >= 0) { "geometry element count must be non-negative" }
    return plus(elementBytes.toLong() * count, 0)
  }

  private fun measurePayload(geometry: Geometry, depth: Int): Long {
    requireDepth(depth)
    return when (geometry) {
      is Geometry.Empty -> 0L
      // Held by value inside the root descriptor's union arm, so it needs no
      // storage of its own.
      is Geometry.Point -> 0L
      is Geometry.LineString -> coordinates(geometry.coordinates.size)
      is Geometry.MultiPoint -> coordinates(geometry.coordinates.size)
      is Geometry.Polygon -> rings(geometry.rings)
      is Geometry.MultiLineString -> rings(geometry.lines)
      is Geometry.MultiPolygon ->
        geometry.polygons.fold(
          HeapArena.aligned(
            sizeOf(MlnPolygonGeometry.SIZEOF, geometry.polygons.size),
            POINTER_ALIGN,
          )
        ) { total, polygon ->
          plus(total, rings(polygon))
        }
      is Geometry.Collection ->
        geometry.geometries.fold(
          HeapArena.aligned(sizeOf(MlnGeometry.SIZEOF, geometry.geometries.size), COORDINATE_ALIGN)
        ) { total, child ->
          plus(total, measurePayload(child, depth + 1))
        }
      // A geometry read back from a native tag this binding did not recognise. Its shape is
      // unknown, so there is nothing to measure and nothing that could be written back.
      is Geometry.Unknown -> throw unknownGeometry(geometry)
    }
  }

  private fun unknownGeometry(geometry: Geometry.Unknown) =
    Status.invalidArgument(
      "A geometry of unknown native type ${geometry.rawType} cannot be sent to native; it was " +
        "read from a tag this binding does not recognise."
    )

  private fun coordinates(count: Int): Long =
    HeapArena.aligned(sizeOf(MlnLatLng.SIZEOF, count), COORDINATE_ALIGN)

  private fun rings(rings: List<List<LatLng>>): Long =
    rings.fold(HeapArena.aligned(sizeOf(MlnCoordinateSpan.SIZEOF, rings.size), POINTER_ALIGN)) {
      total,
      ring ->
      plus(total, coordinates(ring.size))
    }

  /**
   * Refuses a tree deeper than the C API accepts.
   *
   * Checked before recursing rather than left to native: the walk below would otherwise descend an
   * over-deep tree first, and a deep enough one exhausts this module's stack before native ever
   * sees it.
   */
  private fun requireDepth(depth: Int) {
    if (depth > Geometry.MAX_COLLECTION_DEPTH) {
      throw Status.invalidArgument(
        "geometry nests deeper than the ${Geometry.MAX_COLLECTION_DEPTH} levels the C API accepts"
      )
    }
  }

  /** Writes [geometry] into [arena] and returns the root descriptor's address. */
  fun write(arena: HeapArena, geometry: Geometry): HeapPointer {
    val root = arena.allocate(MlnGeometry.SIZEOF, COORDINATE_ALIGN)
    writeInto(arena, root, geometry, 0)
    return root
  }

  private fun writeInto(arena: HeapArena, base: HeapPointer, geometry: Geometry, depth: Int) {
    requireDepth(depth)
    MlnGeometry.setSize(base, MlnGeometry.SIZEOF)
    val data = base + MlnGeometry.OFFSET_DATA
    when (geometry) {
      is Geometry.Empty -> MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_EMPTY)
      is Geometry.Point -> {
        MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_POINT)
        // The point arm holds the coordinate by value rather than by pointer.
        MlnLatLng.setLatitude(data, geometry.coordinate.latitude)
        MlnLatLng.setLongitude(data, geometry.coordinate.longitude)
      }
      is Geometry.LineString -> {
        MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_LINE_STRING)
        writeSpan(arena, data, geometry.coordinates)
      }
      is Geometry.MultiPoint -> {
        MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_MULTI_POINT)
        writeSpan(arena, data, geometry.coordinates)
      }
      is Geometry.Polygon -> {
        MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_POLYGON)
        val spans = writeSpans(arena, geometry.rings)
        MlnPolygonGeometry.setRings(data, spans)
        MlnPolygonGeometry.setRingCount(data, geometry.rings.size)
      }
      is Geometry.MultiLineString -> {
        MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING)
        val spans = writeSpans(arena, geometry.lines)
        MlnMultiLineGeometry.setLines(data, spans)
        MlnMultiLineGeometry.setLineCount(data, geometry.lines.size)
      }
      is Geometry.MultiPolygon -> {
        MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_MULTI_POLYGON)
        val polygons =
          arena.allocate(
            sizeOf(MlnPolygonGeometry.SIZEOF, geometry.polygons.size).toInt(),
            POINTER_ALIGN,
          )
        geometry.polygons.forEachIndexed { index, rings ->
          val entry = polygons + index * MlnPolygonGeometry.SIZEOF
          MlnPolygonGeometry.setRings(entry, writeSpans(arena, rings))
          MlnPolygonGeometry.setRingCount(entry, rings.size)
        }
        MlnMultiPolygonGeometry.setPolygons(data, polygons)
        MlnMultiPolygonGeometry.setPolygonCount(data, geometry.polygons.size)
      }
      is Geometry.Unknown -> throw unknownGeometry(geometry)
      is Geometry.Collection -> {
        MlnGeometry.setType(base, MlnGeometryType.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION)
        val children =
          arena.allocate(
            sizeOf(MlnGeometry.SIZEOF, geometry.geometries.size).toInt(),
            COORDINATE_ALIGN,
          )
        geometry.geometries.forEachIndexed { index, child ->
          writeInto(arena, children + index * MlnGeometry.SIZEOF, child, depth + 1)
        }
        MlnGeometryCollection.setGeometries(data, children)
        MlnGeometryCollection.setGeometryCount(data, geometry.geometries.size)
      }
    }
  }

  /** Writes one coordinate span in place at [span], with its coordinates in the arena. */
  private fun writeSpan(arena: HeapArena, span: HeapPointer, coordinates: List<LatLng>) {
    MlnCoordinateSpan.setCoordinates(span, writeCoordinates(arena, coordinates))
    MlnCoordinateSpan.setCoordinateCount(span, coordinates.size)
  }

  /** Writes an array of spans and returns where it starts. */
  private fun writeSpans(arena: HeapArena, rings: List<List<LatLng>>): HeapPointer {
    val spans = arena.allocate(sizeOf(MlnCoordinateSpan.SIZEOF, rings.size).toInt(), POINTER_ALIGN)
    rings.forEachIndexed { index, ring ->
      writeSpan(arena, spans + index * MlnCoordinateSpan.SIZEOF, ring)
    }
    return spans
  }

  private fun writeCoordinates(arena: HeapArena, coordinates: List<LatLng>): HeapPointer {
    val array = arena.allocate(sizeOf(MlnLatLng.SIZEOF, coordinates.size).toInt(), COORDINATE_ALIGN)
    coordinates.forEachIndexed { index, coordinate ->
      val entry = array + index * MlnLatLng.SIZEOF
      MlnLatLng.setLatitude(entry, coordinate.latitude)
      MlnLatLng.setLongitude(entry, coordinate.longitude)
    }
    return array
  }
}
