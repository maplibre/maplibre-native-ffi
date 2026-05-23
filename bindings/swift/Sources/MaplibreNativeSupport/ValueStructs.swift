import CMaplibreNativeC
import Foundation

public enum NativeJSONValue: Equatable, Sendable {
  case null
  case bool(Bool)
  case uint(UInt64)
  case int(Int64)
  case double(Double)
  case string(String)
  case array([NativeJSONValue])
  case object([NativeJSONMember])

  public init(copying raw: mln_json_value) throws {
    switch raw.type {
    case MLN_JSON_VALUE_TYPE_NULL.rawValue:
      self = .null
    case MLN_JSON_VALUE_TYPE_BOOL.rawValue:
      self = .bool(raw.data.bool_value)
    case MLN_JSON_VALUE_TYPE_UINT.rawValue:
      self = .uint(raw.data.uint_value)
    case MLN_JSON_VALUE_TYPE_INT.rawValue:
      self = .int(raw.data.int_value)
    case MLN_JSON_VALUE_TYPE_DOUBLE.rawValue:
      self = .double(raw.data.double_value)
    case MLN_JSON_VALUE_TYPE_STRING.rawValue:
      self = .string(try NativeString.copyUTF8(data: raw.data.string_value.data, size: raw.data.string_value.size))
    case MLN_JSON_VALUE_TYPE_ARRAY.rawValue:
      let array = raw.data.array_value
      let values = (0..<array.value_count).map { index in
        array.values![index]
      }
      self = .array(try values.map { try NativeJSONValue(copying: $0) })
    case MLN_JSON_VALUE_TYPE_OBJECT.rawValue:
      let object = raw.data.object_value
      let members = try (0..<object.member_count).map { index in
        try NativeJSONMember(copying: object.members![index])
      }
      self = .object(members)
    default:
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "unknown JSON value type \(raw.type)")
    }
  }
}

public struct NativeJSONMember: Equatable, Sendable {
  public let key: String
  public let value: NativeJSONValue

  public init(key: String, value: NativeJSONValue) {
    self.key = key
    self.value = value
  }

  public init(copying raw: mln_json_member) throws {
    key = try NativeString.copyUTF8(data: raw.key.data, size: raw.key.size)
    value = try NativeJSONValue(copying: raw.value.pointee)
  }
}

public final class NativeJSONArena {
  private var strings: [ContiguousArray<UInt8>] = []
  private var values: [UnsafeMutablePointer<mln_json_value>] = []
  private var arrays: [UnsafeMutableBufferPointer<mln_json_value>] = []
  private var members: [UnsafeMutableBufferPointer<mln_json_member>] = []
  private var coordinateArrays: [UnsafeMutableBufferPointer<mln_lat_lng>] = []
  private var coordinateSpans: [UnsafeMutableBufferPointer<mln_coordinate_span>] = []
  private var geometries: [UnsafeMutablePointer<mln_geometry>] = []
  private var features: [UnsafeMutablePointer<mln_feature>] = []
  private var featureArrays: [UnsafeMutableBufferPointer<mln_feature>] = []

  public init() {}

  deinit {
    for value in values {
      value.deinitialize(count: 1)
      value.deallocate()
    }
    for array in arrays {
      array.baseAddress?.deinitialize(count: array.count)
      array.baseAddress?.deallocate()
    }
    for memberArray in members {
      memberArray.baseAddress?.deinitialize(count: memberArray.count)
      memberArray.baseAddress?.deallocate()
    }
    for coordinateArray in coordinateArrays {
      coordinateArray.baseAddress?.deinitialize(count: coordinateArray.count)
      coordinateArray.baseAddress?.deallocate()
    }
    for spanArray in coordinateSpans {
      spanArray.baseAddress?.deinitialize(count: spanArray.count)
      spanArray.baseAddress?.deallocate()
    }
    for geometry in geometries {
      geometry.deinitialize(count: 1)
      geometry.deallocate()
    }
    for feature in features {
      feature.deinitialize(count: 1)
      feature.deallocate()
    }
    for featureArray in featureArrays {
      featureArray.baseAddress?.deinitialize(count: featureArray.count)
      featureArray.baseAddress?.deallocate()
    }
  }

  public func view(_ text: String) -> mln_string_view {
    let bytes = ContiguousArray(text.utf8)
    strings.append(bytes)
    return strings.withUnsafeBufferPointer { storage in
      let index = storage.count - 1
      return storage[index].withUnsafeBufferPointer { bytes in
        mln_string_view(
          data: bytes.baseAddress.map { UnsafeRawPointer($0).assumingMemoryBound(to: CChar.self) },
          size: bytes.count
        )
      }
    }
  }

  public func allocate(_ value: NativeJSONValue) -> UnsafePointer<mln_json_value> {
    let pointer = UnsafeMutablePointer<mln_json_value>.allocate(capacity: 1)
    pointer.initialize(to: nativeValue(value))
    values.append(pointer)
    return UnsafePointer(pointer)
  }

  public func nativeValue(_ value: NativeJSONValue) -> mln_json_value {
    var raw = mln_json_value()
    raw.size = UInt32(MemoryLayout<mln_json_value>.size)
    switch value {
    case .null:
      raw.type = MLN_JSON_VALUE_TYPE_NULL.rawValue
    case .bool(let bool):
      raw.type = MLN_JSON_VALUE_TYPE_BOOL.rawValue
      raw.data.bool_value = bool
    case .uint(let uint):
      raw.type = MLN_JSON_VALUE_TYPE_UINT.rawValue
      raw.data.uint_value = uint
    case .int(let int):
      raw.type = MLN_JSON_VALUE_TYPE_INT.rawValue
      raw.data.int_value = int
    case .double(let double):
      raw.type = MLN_JSON_VALUE_TYPE_DOUBLE.rawValue
      raw.data.double_value = double
    case .string(let string):
      raw.type = MLN_JSON_VALUE_TYPE_STRING.rawValue
      raw.data.string_value = view(string)
    case .array(let array):
      raw.type = MLN_JSON_VALUE_TYPE_ARRAY.rawValue
      let buffer = UnsafeMutablePointer<mln_json_value>.allocate(capacity: array.count)
      for (index, value) in array.enumerated() {
        buffer.advanced(by: index).initialize(to: nativeValue(value))
      }
      arrays.append(UnsafeMutableBufferPointer(start: buffer, count: array.count))
      raw.data.array_value = mln_json_array(values: UnsafePointer(buffer), value_count: array.count)
    case .object(let object):
      raw.type = MLN_JSON_VALUE_TYPE_OBJECT.rawValue
      let buffer = UnsafeMutablePointer<mln_json_member>.allocate(capacity: object.count)
      for (index, member) in object.enumerated() {
        let rawMember = mln_json_member(key: view(member.key), value: allocate(member.value))
        buffer.advanced(by: index).initialize(to: rawMember)
      }
      members.append(UnsafeMutableBufferPointer(start: buffer, count: object.count))
      raw.data.object_value = mln_json_object(members: UnsafePointer(buffer), member_count: object.count)
    }
    return raw
  }

  public func allocateGeometry(_ geometry: NativeGeometry) -> UnsafePointer<mln_geometry> {
    let pointer = UnsafeMutablePointer<mln_geometry>.allocate(capacity: 1)
    pointer.initialize(to: nativeGeometry(geometry))
    geometries.append(pointer)
    return UnsafePointer(pointer)
  }

  public func nativeGeometry(_ geometry: NativeGeometry) -> mln_geometry {
    var raw = mln_geometry()
    raw.size = UInt32(MemoryLayout<mln_geometry>.size)
    switch geometry {
    case .empty:
      raw.type = MLN_GEOMETRY_TYPE_EMPTY.rawValue
    case .point(let point):
      raw.type = MLN_GEOMETRY_TYPE_POINT.rawValue
      raw.data.point = point.native
    case .lineString(let coordinates):
      raw.type = MLN_GEOMETRY_TYPE_LINE_STRING.rawValue
      raw.data.line_string = coordinateSpan(coordinates)
    case .polygon(let rings):
      raw.type = MLN_GEOMETRY_TYPE_POLYGON.rawValue
      raw.data.polygon = polygonGeometry(rings)
    }
    return raw
  }

  public func allocateFeature(_ feature: NativeFeature) -> UnsafePointer<mln_feature> {
    let pointer = UnsafeMutablePointer<mln_feature>.allocate(capacity: 1)
    pointer.initialize(to: nativeFeature(feature))
    features.append(pointer)
    return UnsafePointer(pointer)
  }

  public func nativeFeature(_ feature: NativeFeature) -> mln_feature {
    var raw = mln_feature()
    raw.size = UInt32(MemoryLayout<mln_feature>.size)
    raw.geometry = allocateGeometry(feature.geometry)
    if !feature.properties.isEmpty {
      let buffer = UnsafeMutablePointer<mln_json_member>.allocate(capacity: feature.properties.count)
      for (index, member) in feature.properties.enumerated() {
        buffer.advanced(by: index).initialize(to: mln_json_member(key: view(member.key), value: allocate(member.value)))
      }
      members.append(UnsafeMutableBufferPointer(start: buffer, count: feature.properties.count))
      raw.properties = UnsafePointer(buffer)
      raw.property_count = feature.properties.count
    }
    switch feature.identifier {
    case .none:
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_NULL.rawValue
    case .uint(let value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_UINT.rawValue
      raw.identifier.uint_value = value
    case .int(let value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_INT.rawValue
      raw.identifier.int_value = value
    case .double(let value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE.rawValue
      raw.identifier.double_value = value
    case .string(let value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING.rawValue
      raw.identifier.string_value = view(value)
    }
    return raw
  }

  public func withNativeGeoJSON<Result>(
    _ geoJSON: NativeGeoJSON,
    _ body: (UnsafePointer<mln_geojson>) throws -> Result
  ) throws -> Result {
    var raw = mln_geojson()
    raw.size = UInt32(MemoryLayout<mln_geojson>.size)
    switch geoJSON {
    case .geometry(let geometry):
      raw.type = MLN_GEOJSON_TYPE_GEOMETRY.rawValue
      raw.data.geometry = allocateGeometry(geometry)
    case .feature(let feature):
      raw.type = MLN_GEOJSON_TYPE_FEATURE.rawValue
      raw.data.feature = allocateFeature(feature)
    case .featureCollection(let features):
      raw.type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION.rawValue
      if !features.isEmpty {
        let buffer = UnsafeMutablePointer<mln_feature>.allocate(capacity: features.count)
        for (index, feature) in features.enumerated() {
          buffer.advanced(by: index).initialize(to: nativeFeature(feature))
        }
        featureArrays.append(UnsafeMutableBufferPointer(start: buffer, count: features.count))
        raw.data.feature_collection = mln_feature_collection(features: UnsafePointer(buffer), feature_count: features.count)
      }
    }
    return try withUnsafePointer(to: &raw, body)
  }

  private func coordinateSpan(_ coordinates: [NativeLatLng]) -> mln_coordinate_span {
    guard !coordinates.isEmpty else { return mln_coordinate_span(coordinates: nil, coordinate_count: 0) }
    let buffer = UnsafeMutablePointer<mln_lat_lng>.allocate(capacity: coordinates.count)
    for (index, coordinate) in coordinates.enumerated() {
      buffer.advanced(by: index).initialize(to: coordinate.native)
    }
    coordinateArrays.append(UnsafeMutableBufferPointer(start: buffer, count: coordinates.count))
    return mln_coordinate_span(coordinates: UnsafePointer(buffer), coordinate_count: coordinates.count)
  }

  private func polygonGeometry(_ rings: [[NativeLatLng]]) -> mln_polygon_geometry {
    guard !rings.isEmpty else { return mln_polygon_geometry(rings: nil, ring_count: 0) }
    let buffer = UnsafeMutablePointer<mln_coordinate_span>.allocate(capacity: rings.count)
    for (index, ring) in rings.enumerated() {
      buffer.advanced(by: index).initialize(to: coordinateSpan(ring))
    }
    coordinateSpans.append(UnsafeMutableBufferPointer(start: buffer, count: rings.count))
    return mln_polygon_geometry(rings: UnsafePointer(buffer), ring_count: rings.count)
  }
}

public enum NativeGeometry: Equatable, Sendable {
  case empty
  case point(NativeLatLng)
  case lineString([NativeLatLng])
  case polygon([[NativeLatLng]])

  public init(copying raw: mln_geometry) throws {
    switch raw.type {
    case MLN_GEOMETRY_TYPE_EMPTY.rawValue:
      self = .empty
    case MLN_GEOMETRY_TYPE_POINT.rawValue:
      self = .point(NativeLatLng(raw.data.point))
    case MLN_GEOMETRY_TYPE_LINE_STRING.rawValue:
      let span = raw.data.line_string
      self = .lineString((0..<span.coordinate_count).map { NativeLatLng(span.coordinates![$0]) })
    case MLN_GEOMETRY_TYPE_POLYGON.rawValue:
      let polygon = raw.data.polygon
      self = .polygon((0..<polygon.ring_count).map { ringIndex in
        let ring = polygon.rings![ringIndex]
        return (0..<ring.coordinate_count).map { NativeLatLng(ring.coordinates![$0]) }
      })
    default:
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "unsupported geometry type \(raw.type)")
    }
  }
}

public enum NativeFeatureIdentifier: Equatable, Sendable {
  case none
  case uint(UInt64)
  case int(Int64)
  case double(Double)
  case string(String)
}

public struct NativeFeature: Equatable, Sendable {
  public let geometry: NativeGeometry
  public let properties: [NativeJSONMember]
  public let identifier: NativeFeatureIdentifier

  public init(
    geometry: NativeGeometry,
    properties: [NativeJSONMember] = [],
    identifier: NativeFeatureIdentifier = .none
  ) {
    self.geometry = geometry
    self.properties = properties
    self.identifier = identifier
  }

  public init(copying raw: mln_feature) throws {
    geometry = try NativeGeometry(copying: raw.geometry.pointee)
    properties = try (0..<raw.property_count).map { index in
      try NativeJSONMember(copying: raw.properties![index])
    }
    switch raw.identifier_type {
    case MLN_FEATURE_IDENTIFIER_TYPE_NULL.rawValue:
      identifier = .none
    case MLN_FEATURE_IDENTIFIER_TYPE_UINT.rawValue:
      identifier = .uint(raw.identifier.uint_value)
    case MLN_FEATURE_IDENTIFIER_TYPE_INT.rawValue:
      identifier = .int(raw.identifier.int_value)
    case MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE.rawValue:
      identifier = .double(raw.identifier.double_value)
    case MLN_FEATURE_IDENTIFIER_TYPE_STRING.rawValue:
      identifier = .string(try NativeString.copyUTF8(data: raw.identifier.string_value.data, size: raw.identifier.string_value.size))
    default:
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "unknown feature identifier type \(raw.identifier_type)")
    }
  }
}

public enum NativeGeoJSON: Equatable, Sendable {
  case geometry(NativeGeometry)
  case feature(NativeFeature)
  case featureCollection([NativeFeature])
}

public struct NativeQueriedFeature: Equatable, Sendable {
  public let feature: NativeFeature
  public let sourceId: String?
  public let sourceLayerId: String?
  public let state: NativeJSONValue?

  public init(feature: NativeFeature, sourceId: String? = nil, sourceLayerId: String? = nil, state: NativeJSONValue? = nil) {
    self.feature = feature
    self.sourceId = sourceId
    self.sourceLayerId = sourceLayerId
    self.state = state
  }

  public init(copying raw: mln_queried_feature) throws {
    feature = try NativeFeature(copying: raw.feature)
    sourceId = (raw.fields & MLN_QUERIED_FEATURE_SOURCE_ID.rawValue) != 0
      ? try NativeString.copyUTF8(data: raw.source_id.data, size: raw.source_id.size)
      : nil
    sourceLayerId = (raw.fields & MLN_QUERIED_FEATURE_SOURCE_LAYER_ID.rawValue) != 0
      ? try NativeString.copyUTF8(data: raw.source_layer_id.data, size: raw.source_layer_id.size)
      : nil
    state = (raw.fields & MLN_QUERIED_FEATURE_STATE.rawValue) != 0 && raw.state != nil
      ? try NativeJSONValue(copying: raw.state.pointee)
      : nil
  }
}

public enum NativeFeatureExtensionResult: Equatable, Sendable {
  case value(NativeJSONValue)
  case featureCollection([NativeFeature])

  public init(copying raw: mln_feature_extension_result_info) throws {
    switch raw.type {
    case MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE.rawValue:
      self = .value(try NativeJSONValue(copying: raw.data.value.pointee))
    case MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION.rawValue:
      let collection = raw.data.feature_collection
      let features = try (0..<collection.feature_count).map { index in
        try NativeFeature(copying: collection.features![index])
      }
      self = .featureCollection(features)
    default:
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "unknown feature extension result type \(raw.type)")
    }
  }
}
