import CMaplibreNativeC
import Testing

@testable import MaplibreNative

@Test func jsonValueMaterializesNestedObjectDescriptors() throws {
  let arena = NativeInputArena()
  let root = arena.nativeValue(.object([
    NativeJSONMember(key: "name", value: .string("map")),
    NativeJSONMember(key: "items", value: .array([.uint(1), .bool(true)])),
  ]))

  #expect(root.type == MLN_JSON_VALUE_TYPE_OBJECT.rawValue)
  #expect(root.data.object_value.member_count == 2)
  let first = root.data.object_value.members![0]
  #expect(first.key.size == 4)
  #expect(try NativeString.copyUTF8(data: first.key.data, size: first.key.size) == "name")
  #expect(first.value.pointee.type == MLN_JSON_VALUE_TYPE_STRING.rawValue)
}

@Test func publicValueTypesMapToNativeValueTrees() {
  let value = JSONValue.object([
    JSONMember(key: "geometry", value: .string("point")),
    JSONMember(key: "coordinates", value: .array([.double(1), .double(2)])),
  ])
  let feature = Feature(
    geometry: .point(LatLng(latitude: 1, longitude: 2)),
    properties: [JSONMember(key: "name", value: value)],
    identifier: .string("feature-id")
  )
  let geoJSON = GeoJSON.feature(feature)

  #expect(value.nativeValue != .null)
  #expect(feature.nativeFeature.properties.count == 1)
  #expect(geoJSON.nativeGeoJSON == .feature(feature.nativeFeature))
}

@Test func geometryMaterializesAndCopiesAllCVariants() throws {
  let geometry = Geometry.geometryCollection([
    .multiPoint([LatLng(latitude: 1, longitude: 2), LatLng(latitude: 3, longitude: 4)]),
    .multiLineString([
      [LatLng(latitude: 5, longitude: 6), LatLng(latitude: 7, longitude: 8)],
      [LatLng(latitude: 9, longitude: 10)],
    ]),
    .multiPolygon([[
      [LatLng(latitude: 11, longitude: 12), LatLng(latitude: 13, longitude: 14), LatLng(latitude: 11, longitude: 12)],
    ]]),
  ])

  let arena = NativeInputArena()
  let raw = arena.nativeGeometry(geometry.nativeGeometry)

  #expect(raw.type == MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION.rawValue)
  #expect(raw.data.geometry_collection.geometry_count == 3)
  #expect(raw.data.geometry_collection.geometries![0].type == MLN_GEOMETRY_TYPE_MULTI_POINT.rawValue)
  #expect(raw.data.geometry_collection.geometries![1].type == MLN_GEOMETRY_TYPE_MULTI_LINE_STRING.rawValue)
  #expect(raw.data.geometry_collection.geometries![2].type == MLN_GEOMETRY_TYPE_MULTI_POLYGON.rawValue)

  let copiedNative = try NativeGeometry(copying: raw)
  #expect(Geometry(native: copiedNative) == geometry)
}
