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

@Test func nativeJSONCopyRejectsNullPointersForNonEmptyContainers() throws {
  var arrayValue = mln_json_value()
  arrayValue.type = MLN_JSON_VALUE_TYPE_ARRAY.rawValue
  arrayValue.data.array_value = mln_json_array(values: nil, value_count: 1)

  try expectSwiftFailure("JSON array values pointer is null") {
    _ = try NativeJSONValue(copying: arrayValue)
  }

  var objectValue = mln_json_value()
  objectValue.type = MLN_JSON_VALUE_TYPE_OBJECT.rawValue
  objectValue.data.object_value = mln_json_object(members: nil, member_count: 1)

  try expectSwiftFailure("JSON object members pointer is null") {
    _ = try NativeJSONValue(copying: objectValue)
  }

  let member = mln_json_member(key: mln_string_view(), value: nil)
  try expectSwiftFailure("JSON member value pointer is null") {
    _ = try NativeJSONMember(copying: member)
  }
}

@Test func nativeGeometryCopyRejectsNullPointersForNonEmptyContainers() throws {
  var line = mln_geometry()
  line.type = MLN_GEOMETRY_TYPE_LINE_STRING.rawValue
  line.data.line_string = mln_coordinate_span(coordinates: nil, coordinate_count: 1)

  try expectSwiftFailure("coordinate span coordinates pointer is null") {
    _ = try NativeGeometry(copying: line)
  }

  var polygon = mln_geometry()
  polygon.type = MLN_GEOMETRY_TYPE_POLYGON.rawValue
  polygon.data.polygon = mln_polygon_geometry(rings: nil, ring_count: 1)

  try expectSwiftFailure("polygon geometry rings pointer is null") {
    _ = try NativeGeometry(copying: polygon)
  }

  var collection = mln_geometry()
  collection.type = MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION.rawValue
  collection.data.geometry_collection = mln_geometry_collection(geometries: nil, geometry_count: 1)

  try expectSwiftFailure("geometry collection geometries pointer is null") {
    _ = try NativeGeometry(copying: collection)
  }
}

@Test func nativeFeatureCopyRejectsNullPointersForRequiredFields() throws {
  var feature = mln_feature()
  feature.geometry = nil

  try expectSwiftFailure("feature geometry pointer is null") {
    _ = try NativeFeature(copying: feature)
  }

  var geometry = mln_geometry()
  geometry.type = MLN_GEOMETRY_TYPE_POINT.rawValue
  geometry.data.point = mln_lat_lng(latitude: 1, longitude: 2)

  try withUnsafePointer(to: &geometry) { geometry in
    var feature = mln_feature()
    feature.geometry = geometry
    feature.property_count = 1
    feature.properties = nil

    try expectSwiftFailure("feature properties pointer is null") {
      _ = try NativeFeature(copying: feature)
    }
  }
}

@Test func nativeFeatureExtensionResultCopyRejectsNullPointersForPayloads() throws {
  var valueResult = mln_feature_extension_result_info()
  valueResult.type = MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE.rawValue
  valueResult.data.value = nil

  try expectSwiftFailure("feature extension result value pointer is null") {
    _ = try NativeFeatureExtensionResult(copying: valueResult)
  }

  var collectionResult = mln_feature_extension_result_info()
  collectionResult.type = MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION.rawValue
  collectionResult.data.feature_collection = mln_feature_collection(features: nil, feature_count: 1)

  try expectSwiftFailure("feature extension result feature collection pointer is null") {
    _ = try NativeFeatureExtensionResult(copying: collectionResult)
  }
}

private func expectSwiftFailure(_ diagnostic: String, _ body: () throws -> Void) throws {
  do {
    try body()
    Issue.record("expected Swift validation failure")
  } catch let failure as NativeStatusFailure {
    #expect(!failure.isNativeStatus)
    #expect(failure.rawStatus == MLN_STATUS_NATIVE_ERROR.rawValue)
    #expect(failure.diagnostic == diagnostic)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}
