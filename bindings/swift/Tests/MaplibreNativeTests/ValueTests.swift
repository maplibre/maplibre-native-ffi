import CMaplibreNativeC
import Testing

@testable import MaplibreNative
@testable import MaplibreNativeSupport

@Test func jsonValueMaterializesNestedObjectDescriptors() throws {
  let arena = NativeJSONArena()
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
