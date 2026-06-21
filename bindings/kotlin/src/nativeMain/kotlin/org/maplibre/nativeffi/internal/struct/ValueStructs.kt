package org.maplibre.nativeffi.internal.struct

import cnames.structs.mln_json_snapshot
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.value
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_INT
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_NULL
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_STRING
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_UINT
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_TYPE_FEATURE
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_TYPE_FEATURE_COLLECTION
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_TYPE_GEOMETRY
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_EMPTY
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_MULTI_POINT
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_MULTI_POLYGON
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_POINT
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_POLYGON
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_ARRAY
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_BOOL
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_DOUBLE
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_INT
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_NULL
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_OBJECT
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_STRING
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_UINT
import org.maplibre.nativeffi.internal.c.mln_coordinate_span
import org.maplibre.nativeffi.internal.c.mln_feature
import org.maplibre.nativeffi.internal.c.mln_geojson
import org.maplibre.nativeffi.internal.c.mln_geometry
import org.maplibre.nativeffi.internal.c.mln_json_member
import org.maplibre.nativeffi.internal.c.mln_json_snapshot_destroy
import org.maplibre.nativeffi.internal.c.mln_json_snapshot_get
import org.maplibre.nativeffi.internal.c.mln_json_value
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_polygon_geometry
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.json.JsonValue

/** Materializes JSON, geometry, and GeoJSON descriptor graphs at the C boundary. */
@OptIn(ExperimentalForeignApi::class)
internal object ValueStructs {
  fun jsonValue(value: JsonValue, scope: MemScope): CPointer<mln_json_value> {
    val native = scope.alloc<mln_json_value>()
    writeJson(native, value, scope, 0)
    return native.ptr
  }

  fun geometry(value: Geometry, scope: MemScope): CPointer<mln_geometry> {
    val native = scope.alloc<mln_geometry>()
    writeGeometry(native, value, scope, 0)
    return native.ptr
  }

  fun geoJson(value: GeoJson, scope: MemScope): CPointer<mln_geojson> {
    val native = scope.alloc<mln_geojson>()
    native.size = sizeOf<mln_geojson>().toUInt()
    when (value) {
      is GeoJson.GeometryValue -> {
        native.type = MLN_GEOJSON_TYPE_GEOMETRY
        native.data.geometry = geometry(value.geometry, scope)
      }
      is GeoJson.FeatureValue -> {
        native.type = MLN_GEOJSON_TYPE_FEATURE
        native.data.feature = feature(value.feature, scope)
      }
      is GeoJson.FeatureCollection -> {
        native.type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION
        val features =
          if (value.features.isEmpty()) null else scope.allocArray<mln_feature>(value.features.size)
        value.features.forEachIndexed { index, feature ->
          writeFeature(features!![index], feature, scope)
        }
        native.data.feature_collection.features = features
        native.data.feature_collection.feature_count = value.features.size.toULong()
      }
    }
    return native.ptr
  }

  fun jsonSnapshot(value: CPointer<mln_json_value>?): JsonValue {
    require(value != null) { "JSON value pointer must not be null" }
    return readJson(value.pointed)
  }

  fun jsonSnapshotHandle(
    snapshot: CPointer<mln_json_snapshot>?,
    getter:
      (CPointer<mln_json_snapshot>, CPointer<CPointerVarOf<CPointer<mln_json_value>>>) -> Int =
      ::mln_json_snapshot_get,
    destroyer: (CPointer<mln_json_snapshot>) -> Unit = ::mln_json_snapshot_destroy,
  ): JsonValue? {
    if (snapshot == null) return null
    return try {
      memScoped {
        val outValue = alloc<CPointerVarOf<CPointer<mln_json_value>>>()
        outValue.value = null
        Status.check(getter(snapshot, outValue.ptr))
        outValue.value?.let(::jsonSnapshot)
      }
    } finally {
      destroyer(snapshot)
    }
  }

  fun geometrySnapshot(value: CPointer<mln_geometry>?): Geometry {
    require(value != null) { "Geometry pointer must not be null" }
    return readGeometry(value.pointed)
  }

  fun featureSnapshot(value: CPointer<mln_feature>?): Feature {
    require(value != null) { "Feature pointer must not be null" }
    return readFeature(value.pointed)
  }

  fun feature(value: Feature, scope: MemScope): CPointer<mln_feature> {
    val native = scope.alloc<mln_feature>()
    writeFeature(native, value, scope)
    return native.ptr
  }

  private fun writeFeature(native: mln_feature, value: Feature, scope: MemScope) {
    native.size = sizeOf<mln_feature>().toUInt()
    native.geometry = geometry(value.geometry, scope)
    val values =
      if (value.properties.isEmpty()) null
      else scope.allocArray<mln_json_value>(value.properties.size)
    val members =
      if (value.properties.isEmpty()) null
      else scope.allocArray<mln_json_member>(value.properties.size)
    value.properties.forEachIndexed { index, member ->
      writeJson(values!![index], member.value, scope, 0)
      CoreStructs.setStringView(members!![index].key, member.key, scope)
      members[index].value = values[index].ptr
    }
    native.properties = members
    native.property_count = value.properties.size.toULong()
    when (val identifier = value.identifier) {
      FeatureIdentifier.Null -> native.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_NULL
      is FeatureIdentifier.UInt -> {
        native.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_UINT
        native.identifier.uint_value = identifier.value.toULong()
      }
      is FeatureIdentifier.Int -> {
        native.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_INT
        native.identifier.int_value = identifier.value
      }
      is FeatureIdentifier.DoubleValue -> {
        native.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE
        native.identifier.double_value = identifier.value
      }
      is FeatureIdentifier.StringValue -> {
        native.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING
        CoreStructs.setStringView(native.identifier.string_value, identifier.value, scope)
      }
      is FeatureIdentifier.Unknown ->
        throw Status.invalidArgument("unknown feature identifiers cannot be used as input")
    }
  }

  private fun writeJson(native: mln_json_value, value: JsonValue, scope: MemScope, depth: Int) {
    Status.requireArgument(depth <= JsonValue.MAX_DESCRIPTOR_DEPTH) {
      "JSON descriptor depth exceeds ${JsonValue.MAX_DESCRIPTOR_DEPTH}"
    }
    native.size = sizeOf<mln_json_value>().toUInt()
    when (value) {
      JsonValue.Null -> native.type = MLN_JSON_VALUE_TYPE_NULL
      is JsonValue.Bool -> {
        native.type = MLN_JSON_VALUE_TYPE_BOOL
        native.data.bool_value = value.value
      }
      is JsonValue.UInt -> {
        native.type = MLN_JSON_VALUE_TYPE_UINT
        native.data.uint_value = value.value.toULong()
      }
      is JsonValue.Int -> {
        native.type = MLN_JSON_VALUE_TYPE_INT
        native.data.int_value = value.value
      }
      is JsonValue.DoubleValue -> {
        native.type = MLN_JSON_VALUE_TYPE_DOUBLE
        native.data.double_value = value.value
      }
      is JsonValue.StringValue -> {
        native.type = MLN_JSON_VALUE_TYPE_STRING
        CoreStructs.setStringView(native.data.string_value, value.value, scope)
      }
      is JsonValue.Array -> {
        native.type = MLN_JSON_VALUE_TYPE_ARRAY
        val values =
          if (value.values.isEmpty()) null else scope.allocArray<mln_json_value>(value.values.size)
        value.values.forEachIndexed { index, child ->
          writeJson(values!![index], child, scope, depth + 1)
        }
        native.data.array_value.values = values
        native.data.array_value.value_count = value.values.size.toULong()
      }
      is JsonValue.ObjectValue -> {
        native.type = MLN_JSON_VALUE_TYPE_OBJECT
        val values =
          if (value.members.isEmpty()) null
          else scope.allocArray<mln_json_value>(value.members.size)
        val members =
          if (value.members.isEmpty()) null
          else scope.allocArray<mln_json_member>(value.members.size)
        value.members.forEachIndexed { index, member ->
          writeJson(values!![index], member.value, scope, depth + 1)
          CoreStructs.setStringView(members!![index].key, member.key, scope)
          members[index].value = values[index].ptr
        }
        native.data.object_value.members = members
        native.data.object_value.member_count = value.members.size.toULong()
      }
      is JsonValue.Unknown ->
        throw Status.invalidArgument("unknown JSON values cannot be used as input")
    }
  }

  private fun writeGeometry(native: mln_geometry, value: Geometry, scope: MemScope, depth: Int) {
    Status.requireArgument(depth <= Geometry.MAX_COLLECTION_DEPTH) {
      "Geometry collection depth exceeds ${Geometry.MAX_COLLECTION_DEPTH}"
    }
    native.size = sizeOf<mln_geometry>().toUInt()
    when (value) {
      Geometry.Empty -> native.type = MLN_GEOMETRY_TYPE_EMPTY
      is Geometry.Point -> {
        native.type = MLN_GEOMETRY_TYPE_POINT
        setLatLng(native.data.point, value.coordinate)
      }
      is Geometry.LineString -> {
        native.type = MLN_GEOMETRY_TYPE_LINE_STRING
        setCoordinateSpan(native.data.line_string, value.coordinates, scope)
      }
      is Geometry.Polygon -> {
        native.type = MLN_GEOMETRY_TYPE_POLYGON
        setPolygon(native.data.polygon, value.rings, scope)
      }
      is Geometry.MultiPoint -> {
        native.type = MLN_GEOMETRY_TYPE_MULTI_POINT
        setCoordinateSpan(native.data.multi_point, value.coordinates, scope)
      }
      is Geometry.MultiLineString -> {
        native.type = MLN_GEOMETRY_TYPE_MULTI_LINE_STRING
        val lines =
          if (value.lines.isEmpty()) null
          else scope.allocArray<mln_coordinate_span>(value.lines.size)
        value.lines.forEachIndexed { index, line -> setCoordinateSpan(lines!![index], line, scope) }
        native.data.multi_line_string.lines = lines
        native.data.multi_line_string.line_count = value.lines.size.toULong()
      }
      is Geometry.MultiPolygon -> {
        native.type = MLN_GEOMETRY_TYPE_MULTI_POLYGON
        val polygons =
          if (value.polygons.isEmpty()) null
          else scope.allocArray<mln_polygon_geometry>(value.polygons.size)
        value.polygons.forEachIndexed { index, polygon ->
          setPolygon(polygons!![index], polygon, scope)
        }
        native.data.multi_polygon.polygons = polygons
        native.data.multi_polygon.polygon_count = value.polygons.size.toULong()
      }
      is Geometry.Collection -> {
        native.type = MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION
        val geometries =
          if (value.geometries.isEmpty()) null
          else scope.allocArray<mln_geometry>(value.geometries.size)
        value.geometries.forEachIndexed { index, child ->
          writeGeometry(geometries!![index], child, scope, depth + 1)
        }
        native.data.geometry_collection.geometries = geometries
        native.data.geometry_collection.geometry_count = value.geometries.size.toULong()
      }
      is Geometry.Unknown ->
        throw Status.invalidArgument("unknown geometries cannot be used as input")
    }
  }

  private fun setCoordinateSpan(
    native: mln_coordinate_span,
    coordinates: List<LatLng>,
    scope: MemScope,
  ) {
    val nativeCoordinates =
      if (coordinates.isEmpty()) null else scope.allocArray<mln_lat_lng>(coordinates.size)
    coordinates.forEachIndexed { index, coordinate ->
      setLatLng(nativeCoordinates!![index], coordinate)
    }
    native.coordinates = nativeCoordinates
    native.coordinate_count = coordinates.size.toULong()
  }

  private fun setPolygon(native: mln_polygon_geometry, rings: List<List<LatLng>>, scope: MemScope) {
    val nativeRings =
      if (rings.isEmpty()) null else scope.allocArray<mln_coordinate_span>(rings.size)
    rings.forEachIndexed { index, ring -> setCoordinateSpan(nativeRings!![index], ring, scope) }
    native.rings = nativeRings
    native.ring_count = rings.size.toULong()
  }

  private fun setLatLng(native: mln_lat_lng, coordinate: LatLng) {
    native.latitude = coordinate.latitude
    native.longitude = coordinate.longitude
  }

  private fun readJson(native: mln_json_value): JsonValue =
    when (native.type) {
      MLN_JSON_VALUE_TYPE_NULL -> JsonValue.Null
      MLN_JSON_VALUE_TYPE_BOOL -> JsonValue.Bool(native.data.bool_value)
      MLN_JSON_VALUE_TYPE_UINT -> JsonValue.UInt(native.data.uint_value.toLong())
      MLN_JSON_VALUE_TYPE_INT -> JsonValue.Int(native.data.int_value)
      MLN_JSON_VALUE_TYPE_DOUBLE -> JsonValue.DoubleValue(native.data.double_value)
      MLN_JSON_VALUE_TYPE_STRING ->
        JsonValue.StringValue(CoreStructs.stringView(native.data.string_value))
      MLN_JSON_VALUE_TYPE_ARRAY ->
        JsonValue.Array(
          List(checkedInt(native.data.array_value.value_count, "JSON array value count")) { index ->
            readJson(native.data.array_value.values!![index])
          }
        )
      MLN_JSON_VALUE_TYPE_OBJECT ->
        JsonValue.ObjectValue(
          List(checkedInt(native.data.object_value.member_count, "JSON object member count")) {
            index ->
            val member = native.data.object_value.members!![index]
            JsonValue.Member(CoreStructs.stringView(member.key), jsonSnapshot(member.value))
          }
        )
      else ->
        JsonValue.Unknown(native.type.toInt(), checkedInt(native.size.toULong(), "JSON value size"))
    }

  private fun readGeometry(native: mln_geometry): Geometry =
    when (native.type) {
      MLN_GEOMETRY_TYPE_EMPTY -> Geometry.Empty
      MLN_GEOMETRY_TYPE_POINT -> Geometry.Point(CoreStructs.latLng(native.data.point))
      MLN_GEOMETRY_TYPE_LINE_STRING ->
        Geometry.LineString(readCoordinateSpan(native.data.line_string))
      MLN_GEOMETRY_TYPE_POLYGON -> Geometry.Polygon(readPolygon(native.data.polygon))
      MLN_GEOMETRY_TYPE_MULTI_POINT ->
        Geometry.MultiPoint(readCoordinateSpan(native.data.multi_point))
      MLN_GEOMETRY_TYPE_MULTI_LINE_STRING ->
        Geometry.MultiLineString(
          List(
            checkedInt(native.data.multi_line_string.line_count, "multi-line string line count")
          ) { index ->
            readCoordinateSpan(native.data.multi_line_string.lines!![index])
          }
        )
      MLN_GEOMETRY_TYPE_MULTI_POLYGON ->
        Geometry.MultiPolygon(
          List(
            checkedInt(native.data.multi_polygon.polygon_count, "multi-polygon polygon count")
          ) { index ->
            readPolygon(native.data.multi_polygon.polygons!![index])
          }
        )
      MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION ->
        Geometry.Collection(
          List(
            checkedInt(native.data.geometry_collection.geometry_count, "geometry collection count")
          ) { index ->
            readGeometry(native.data.geometry_collection.geometries!![index])
          }
        )
      else ->
        Geometry.Unknown(native.type.toInt(), checkedInt(native.size.toULong(), "geometry size"))
    }

  private fun readFeature(native: mln_feature): Feature {
    val properties =
      List(checkedInt(native.property_count, "feature property count")) { index ->
        val member = native.properties!![index]
        JsonValue.Member(CoreStructs.stringView(member.key), jsonSnapshot(member.value))
      }
    return Feature(
      readGeometry(native.geometry!!.pointed),
      properties,
      readFeatureIdentifier(native),
    )
  }

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private fun readFeatureIdentifier(native: mln_feature): FeatureIdentifier =
    when (native.identifier_type) {
      MLN_FEATURE_IDENTIFIER_TYPE_NULL -> FeatureIdentifier.Null
      MLN_FEATURE_IDENTIFIER_TYPE_UINT ->
        FeatureIdentifier.UInt(native.identifier.uint_value.toLong())
      MLN_FEATURE_IDENTIFIER_TYPE_INT -> FeatureIdentifier.Int(native.identifier.int_value)
      MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE ->
        FeatureIdentifier.DoubleValue(native.identifier.double_value)
      MLN_FEATURE_IDENTIFIER_TYPE_STRING ->
        FeatureIdentifier.StringValue(CoreStructs.stringView(native.identifier.string_value))
      else -> FeatureIdentifier.Unknown(native.identifier_type.toInt())
    }

  private fun readCoordinateSpan(native: mln_coordinate_span): List<LatLng> =
    CoreStructs.latLngArray(
      native.coordinates,
      checkedInt(native.coordinate_count, "coordinate count"),
    )

  private fun readPolygon(native: mln_polygon_geometry): List<List<LatLng>> =
    List(checkedInt(native.ring_count, "polygon ring count")) { index ->
      readCoordinateSpan(native.rings!![index])
    }
}
