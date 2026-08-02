internal import CMaplibreNativeC
import Foundation

enum NativeJSONValue: Equatable {
  case null
  case bool(Bool)
  case uint(UInt64)
  case int(Int64)
  case double(Double)
  case string(String)
  case array([NativeJSONValue])
  case object([NativeJSONMember])

  init(copying raw: mln_json_value) throws {
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
      self = try .string(NativeString.copyUTF8(
        data: raw.data.string_value.data,
        size: raw.data.string_value.size
      ))
    case MLN_JSON_VALUE_TYPE_ARRAY.rawValue:
      let array = raw.data.array_value
      let values = try Self.copyJSONValues(
        array.values,
        count: array.value_count
      )
      self = try .array(values.map { try NativeJSONValue(copying: $0) })
    case MLN_JSON_VALUE_TYPE_OBJECT.rawValue:
      let object = raw.data.object_value
      let rawMembers = try Self.copyJSONMembers(
        object.members,
        count: object.member_count
      )
      let members = try rawMembers.map { try NativeJSONMember(copying: $0) }
      self = .object(members)
    default:
      throw NativeStatusFailure
        .swiftNativeError("unknown JSON value type \(raw.type)")
    }
  }

  private static func copyJSONValues(
    _ values: UnsafePointer<mln_json_value>?,
    count: Int
  ) throws -> [mln_json_value] {
    guard count > 0 else { return [] }
    guard let values
    else {
      throw NativeStatusFailure
        .swiftNativeError("JSON array values pointer is null")
    }
    return (0 ..< count).map { values[$0] }
  }

  private static func copyJSONMembers(
    _ members: UnsafePointer<mln_json_member>?,
    count: Int
  ) throws -> [mln_json_member] {
    guard count > 0 else { return [] }
    guard let members
    else {
      throw NativeStatusFailure
        .swiftNativeError("JSON object members pointer is null")
    }
    return (0 ..< count).map { members[$0] }
  }
}

struct NativeJSONMember: Equatable {
  let key: String
  let value: NativeJSONValue

  init(key: String, value: NativeJSONValue) {
    self.key = key
    self.value = value
  }

  init(copying raw: mln_json_member) throws {
    key = try NativeString.copyUTF8(data: raw.key.data, size: raw.key.size)
    guard let rawValue = raw.value else {
      throw NativeStatusFailure
        .swiftNativeError("JSON member value pointer is null")
    }
    value = try NativeJSONValue(copying: rawValue.pointee)
  }
}

/// Owns per-call native input storage whose pointers stay valid until the C
/// call returns.
final class NativeInputArena {
  private var strings: [ContiguousArray<UInt8>] = []
  private var values: [UnsafeMutablePointer<mln_json_value>] = []
  private var arrays: [UnsafeMutableBufferPointer<mln_json_value>] = []
  private var members: [UnsafeMutableBufferPointer<mln_json_member>] = []
  private var coordinateArrays: [UnsafeMutableBufferPointer<mln_lat_lng>] = []
  private var coordinateSpans: [
    UnsafeMutableBufferPointer<mln_coordinate_span>
  ] =
    []
  private var polygonArrays: [
    UnsafeMutableBufferPointer<mln_polygon_geometry>
  ] =
    []
  private var geometries: [UnsafeMutablePointer<mln_geometry>] = []
  private var geometryArrays: [UnsafeMutableBufferPointer<mln_geometry>] = []
  private var features: [UnsafeMutablePointer<mln_feature>] = []
  private var featureArrays: [UnsafeMutableBufferPointer<mln_feature>] = []

  init() {}

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
    for polygonArray in polygonArrays {
      polygonArray.baseAddress?.deinitialize(count: polygonArray.count)
      polygonArray.baseAddress?.deallocate()
    }
    for geometry in geometries {
      geometry.deinitialize(count: 1)
      geometry.deallocate()
    }
    for geometryArray in geometryArrays {
      geometryArray.baseAddress?.deinitialize(count: geometryArray.count)
      geometryArray.baseAddress?.deallocate()
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

  func view(_ text: String) -> mln_string_view {
    let bytes = ContiguousArray(text.utf8)
    strings.append(bytes)
    return strings.withUnsafeBufferPointer { storage in
      let index = storage.count - 1
      return storage[index].withUnsafeBufferPointer { bytes in
        mln_string_view(
          data: bytes.baseAddress
            .map { UnsafeRawPointer($0).assumingMemoryBound(to: CChar.self) },
          size: bytes.count
        )
      }
    }
  }

  func allocate(_ value: NativeJSONValue) -> UnsafePointer<mln_json_value> {
    let pointer = UnsafeMutablePointer<mln_json_value>.allocate(capacity: 1)
    pointer.initialize(to: nativeValue(value))
    values.append(pointer)
    return UnsafePointer(pointer)
  }

  func nativeValue(_ value: NativeJSONValue) -> mln_json_value {
    var raw = mln_json_value()
    raw.size = UInt32(MemoryLayout<mln_json_value>.size)
    switch value {
    case .null:
      raw.type = MLN_JSON_VALUE_TYPE_NULL.rawValue
    case let .bool(bool):
      raw.type = MLN_JSON_VALUE_TYPE_BOOL.rawValue
      raw.data.bool_value = bool
    case let .uint(uint):
      raw.type = MLN_JSON_VALUE_TYPE_UINT.rawValue
      raw.data.uint_value = uint
    case let .int(int):
      raw.type = MLN_JSON_VALUE_TYPE_INT.rawValue
      raw.data.int_value = int
    case let .double(double):
      raw.type = MLN_JSON_VALUE_TYPE_DOUBLE.rawValue
      raw.data.double_value = double
    case let .string(string):
      raw.type = MLN_JSON_VALUE_TYPE_STRING.rawValue
      raw.data.string_value = view(string)
    case let .array(array):
      raw.type = MLN_JSON_VALUE_TYPE_ARRAY.rawValue
      let buffer = UnsafeMutablePointer<mln_json_value>
        .allocate(capacity: array.count)
      for (index, value) in array.enumerated() {
        buffer.advanced(by: index).initialize(to: nativeValue(value))
      }
      arrays.append(UnsafeMutableBufferPointer(
        start: buffer,
        count: array.count
      ))
      raw.data.array_value = mln_json_array(
        values: UnsafePointer(buffer),
        value_count: array.count
      )
    case let .object(object):
      raw.type = MLN_JSON_VALUE_TYPE_OBJECT.rawValue
      let buffer = UnsafeMutablePointer<mln_json_member>
        .allocate(capacity: object.count)
      for (index, member) in object.enumerated() {
        let rawMember = mln_json_member(
          key: view(member.key),
          value: allocate(member.value)
        )
        buffer.advanced(by: index).initialize(to: rawMember)
      }
      members.append(UnsafeMutableBufferPointer(
        start: buffer,
        count: object.count
      ))
      raw.data.object_value = mln_json_object(
        members: UnsafePointer(buffer),
        member_count: object.count
      )
    }
    return raw
  }

  func allocateGeometry(_ geometry: NativeGeometry)
    -> UnsafePointer<mln_geometry>
  {
    let pointer = UnsafeMutablePointer<mln_geometry>.allocate(capacity: 1)
    pointer.initialize(to: nativeGeometry(geometry))
    geometries.append(pointer)
    return UnsafePointer(pointer)
  }

  func nativeGeometry(_ geometry: NativeGeometry) -> mln_geometry {
    var raw = mln_geometry()
    raw.size = UInt32(MemoryLayout<mln_geometry>.size)
    switch geometry {
    case .empty:
      raw.type = MLN_GEOMETRY_TYPE_EMPTY.rawValue
    case let .point(point):
      raw.type = MLN_GEOMETRY_TYPE_POINT.rawValue
      raw.data.point = point.native
    case let .lineString(coordinates):
      raw.type = MLN_GEOMETRY_TYPE_LINE_STRING.rawValue
      raw.data.line_string = coordinateSpan(coordinates)
    case let .polygon(rings):
      raw.type = MLN_GEOMETRY_TYPE_POLYGON.rawValue
      raw.data.polygon = polygonGeometry(rings)
    case let .multiPoint(coordinates):
      raw.type = MLN_GEOMETRY_TYPE_MULTI_POINT.rawValue
      raw.data.multi_point = coordinateSpan(coordinates)
    case let .multiLineString(lines):
      raw.type = MLN_GEOMETRY_TYPE_MULTI_LINE_STRING.rawValue
      raw.data.multi_line_string = multiLineGeometry(lines)
    case let .multiPolygon(polygons):
      raw.type = MLN_GEOMETRY_TYPE_MULTI_POLYGON.rawValue
      raw.data.multi_polygon = multiPolygonGeometry(polygons)
    case let .geometryCollection(geometries):
      raw.type = MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION.rawValue
      raw.data.geometry_collection = geometryCollection(geometries)
    }
    return raw
  }

  func allocateFeature(_ feature: NativeFeature) -> UnsafePointer<mln_feature> {
    let pointer = UnsafeMutablePointer<mln_feature>.allocate(capacity: 1)
    pointer.initialize(to: nativeFeature(feature))
    features.append(pointer)
    return UnsafePointer(pointer)
  }

  func nativeFeature(_ feature: NativeFeature) -> mln_feature {
    var raw = mln_feature()
    raw.size = UInt32(MemoryLayout<mln_feature>.size)
    raw.geometry = allocateGeometry(feature.geometry)
    if !feature.properties.isEmpty {
      let buffer = UnsafeMutablePointer<mln_json_member>
        .allocate(capacity: feature.properties.count)
      for (index, member) in feature.properties.enumerated() {
        buffer.advanced(by: index).initialize(to: mln_json_member(
          key: view(member.key),
          value: allocate(member.value)
        ))
      }
      members.append(UnsafeMutableBufferPointer(
        start: buffer,
        count: feature.properties.count
      ))
      raw.properties = UnsafePointer(buffer)
      raw.property_count = feature.properties.count
    }
    switch feature.identifier {
    case .none:
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_NULL.rawValue
    case let .uint(value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_UINT.rawValue
      raw.identifier.uint_value = value
    case let .int(value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_INT.rawValue
      raw.identifier.int_value = value
    case let .double(value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE.rawValue
      raw.identifier.double_value = value
    case let .string(value):
      raw.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING.rawValue
      raw.identifier.string_value = view(value)
    }
    return raw
  }

  func withNativeGeoJSON<Result>(
    _ geoJSON: NativeGeoJSON,
    _ body: (UnsafePointer<mln_geojson>) throws -> Result
  ) throws -> Result {
    var raw = mln_geojson()
    raw.size = UInt32(MemoryLayout<mln_geojson>.size)
    switch geoJSON {
    case let .geometry(geometry):
      raw.type = MLN_GEOJSON_TYPE_GEOMETRY.rawValue
      raw.data.geometry = allocateGeometry(geometry)
    case let .feature(feature):
      raw.type = MLN_GEOJSON_TYPE_FEATURE.rawValue
      raw.data.feature = allocateFeature(feature)
    case let .featureCollection(features):
      raw.type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION.rawValue
      if !features.isEmpty {
        let buffer = UnsafeMutablePointer<mln_feature>
          .allocate(capacity: features.count)
        for (index, feature) in features.enumerated() {
          buffer.advanced(by: index).initialize(to: nativeFeature(feature))
        }
        featureArrays.append(UnsafeMutableBufferPointer(
          start: buffer,
          count: features.count
        ))
        raw.data.feature_collection = mln_feature_collection(
          features: UnsafePointer(buffer),
          feature_count: features.count
        )
      }
    }
    return try withUnsafePointer(to: &raw, body)
  }

  private func coordinateSpan(_ coordinates: [NativeLatLng])
    -> mln_coordinate_span
  {
    guard !coordinates.isEmpty else { return mln_coordinate_span(
      coordinates: nil,
      coordinate_count: 0
    ) }
    let buffer = UnsafeMutablePointer<mln_lat_lng>
      .allocate(capacity: coordinates.count)
    for (index, coordinate) in coordinates.enumerated() {
      buffer.advanced(by: index).initialize(to: coordinate.native)
    }
    coordinateArrays.append(UnsafeMutableBufferPointer(
      start: buffer,
      count: coordinates.count
    ))
    return mln_coordinate_span(
      coordinates: UnsafePointer(buffer),
      coordinate_count: coordinates.count
    )
  }

  private func polygonGeometry(_ rings: [[NativeLatLng]])
    -> mln_polygon_geometry
  {
    guard !rings.isEmpty else { return mln_polygon_geometry(
      rings: nil,
      ring_count: 0
    ) }
    let buffer = UnsafeMutablePointer<mln_coordinate_span>
      .allocate(capacity: rings.count)
    for (index, ring) in rings.enumerated() {
      buffer.advanced(by: index).initialize(to: coordinateSpan(ring))
    }
    coordinateSpans.append(UnsafeMutableBufferPointer(
      start: buffer,
      count: rings.count
    ))
    return mln_polygon_geometry(
      rings: UnsafePointer(buffer),
      ring_count: rings.count
    )
  }

  private func multiLineGeometry(_ lines: [[NativeLatLng]])
    -> mln_multi_line_geometry
  {
    guard !lines.isEmpty else { return mln_multi_line_geometry(
      lines: nil,
      line_count: 0
    ) }
    let buffer = UnsafeMutablePointer<mln_coordinate_span>
      .allocate(capacity: lines.count)
    for (index, line) in lines.enumerated() {
      buffer.advanced(by: index).initialize(to: coordinateSpan(line))
    }
    coordinateSpans.append(UnsafeMutableBufferPointer(
      start: buffer,
      count: lines.count
    ))
    return mln_multi_line_geometry(
      lines: UnsafePointer(buffer),
      line_count: lines.count
    )
  }

  private func multiPolygonGeometry(_ polygons: [[[NativeLatLng]]])
    -> mln_multi_polygon_geometry
  {
    guard !polygons.isEmpty else { return mln_multi_polygon_geometry(
      polygons: nil,
      polygon_count: 0
    ) }
    let buffer = UnsafeMutablePointer<mln_polygon_geometry>
      .allocate(capacity: polygons.count)
    for (index, polygon) in polygons.enumerated() {
      buffer.advanced(by: index).initialize(to: polygonGeometry(polygon))
    }
    polygonArrays.append(UnsafeMutableBufferPointer(
      start: buffer,
      count: polygons.count
    ))
    return mln_multi_polygon_geometry(
      polygons: UnsafePointer(buffer),
      polygon_count: polygons.count
    )
  }

  private func geometryCollection(_ geometries: [NativeGeometry])
    -> mln_geometry_collection
  {
    guard !geometries.isEmpty else { return mln_geometry_collection(
      geometries: nil,
      geometry_count: 0
    ) }
    let buffer = UnsafeMutablePointer<mln_geometry>
      .allocate(capacity: geometries.count)
    for (index, geometry) in geometries.enumerated() {
      buffer.advanced(by: index).initialize(to: nativeGeometry(geometry))
    }
    geometryArrays.append(UnsafeMutableBufferPointer(
      start: buffer,
      count: geometries.count
    ))
    return mln_geometry_collection(
      geometries: UnsafePointer(buffer),
      geometry_count: geometries.count
    )
  }
}

enum NativeGeometry: Equatable {
  case empty
  case point(NativeLatLng)
  case lineString([NativeLatLng])
  case polygon([[NativeLatLng]])
  case multiPoint([NativeLatLng])
  case multiLineString([[NativeLatLng]])
  case multiPolygon([[[NativeLatLng]]])
  case geometryCollection([NativeGeometry])

  init(copying raw: mln_geometry) throws {
    switch raw.type {
    case MLN_GEOMETRY_TYPE_EMPTY.rawValue:
      self = .empty
    case MLN_GEOMETRY_TYPE_POINT.rawValue:
      self = .point(NativeLatLng(raw.data.point))
    case MLN_GEOMETRY_TYPE_LINE_STRING.rawValue:
      self = try .lineString(Self.copyCoordinateSpan(raw.data.line_string))
    case MLN_GEOMETRY_TYPE_POLYGON.rawValue:
      self = try .polygon(Self.copyPolygon(raw.data.polygon))
    case MLN_GEOMETRY_TYPE_MULTI_POINT.rawValue:
      self = try .multiPoint(Self.copyCoordinateSpan(raw.data.multi_point))
    case MLN_GEOMETRY_TYPE_MULTI_LINE_STRING.rawValue:
      let multiLine = raw.data.multi_line_string
      let lines = try Self.copyCoordinateSpans(
        multiLine.lines,
        count: multiLine.line_count,
        diagnostic: "multi-line geometry lines pointer is null"
      )
      self = try .multiLineString(lines.map { line in
        try Self.copyCoordinateSpan(line)
      })
    case MLN_GEOMETRY_TYPE_MULTI_POLYGON.rawValue:
      let multiPolygon = raw.data.multi_polygon
      let polygons = try Self.copyPolygons(
        multiPolygon.polygons,
        count: multiPolygon.polygon_count,
        diagnostic: "multi-polygon geometry polygons pointer is null"
      )
      self = try .multiPolygon(polygons.map { polygon in
        try Self.copyPolygon(polygon)
      })
    case MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION.rawValue:
      let collection = raw.data.geometry_collection
      let geometries = try Self.copyGeometries(
        collection.geometries,
        count: collection.geometry_count,
        diagnostic: "geometry collection geometries pointer is null"
      )
      self = try .geometryCollection(geometries.map { geometry in
        try NativeGeometry(copying: geometry)
      })
    default:
      throw NativeStatusFailure
        .swiftNativeError("unsupported geometry type \(raw.type)")
    }
  }

  private static func copyCoordinateSpan(_ span: mln_coordinate_span) throws
    -> [NativeLatLng]
  {
    guard span.coordinate_count > 0 else { return [] }
    guard let coordinates = span.coordinates else {
      throw NativeStatusFailure
        .swiftNativeError("coordinate span coordinates pointer is null")
    }
    return (0 ..< span.coordinate_count).map { NativeLatLng(coordinates[$0]) }
  }

  private static func copyPolygon(_ polygon: mln_polygon_geometry) throws
    -> [[NativeLatLng]]
  {
    let rings = try copyCoordinateSpans(
      polygon.rings,
      count: polygon.ring_count,
      diagnostic: "polygon geometry rings pointer is null"
    )
    return try rings.map { ring in
      try copyCoordinateSpan(ring)
    }
  }

  private static func copyCoordinateSpans(
    _ spans: UnsafePointer<mln_coordinate_span>?,
    count: Int,
    diagnostic: String
  ) throws -> [mln_coordinate_span] {
    guard count > 0 else { return [] }
    guard let spans
    else { throw NativeStatusFailure.swiftNativeError(diagnostic) }
    return (0 ..< count).map { spans[$0] }
  }

  private static func copyPolygons(
    _ polygons: UnsafePointer<mln_polygon_geometry>?,
    count: Int,
    diagnostic: String
  ) throws -> [mln_polygon_geometry] {
    guard count > 0 else { return [] }
    guard let polygons
    else { throw NativeStatusFailure.swiftNativeError(diagnostic) }
    return (0 ..< count).map { polygons[$0] }
  }

  private static func copyGeometries(
    _ geometries: UnsafePointer<mln_geometry>?,
    count: Int,
    diagnostic: String
  ) throws -> [mln_geometry] {
    guard count > 0 else { return [] }
    guard let geometries
    else { throw NativeStatusFailure.swiftNativeError(diagnostic) }
    return (0 ..< count).map { geometries[$0] }
  }
}

enum NativeFeatureIdentifier: Equatable {
  case none
  case uint(UInt64)
  case int(Int64)
  case double(Double)
  case string(String)
}

struct NativeFeature: Equatable {
  let geometry: NativeGeometry
  let properties: [NativeJSONMember]
  let identifier: NativeFeatureIdentifier

  init(
    geometry: NativeGeometry,
    properties: [NativeJSONMember] = [],
    identifier: NativeFeatureIdentifier = .none
  ) {
    self.geometry = geometry
    self.properties = properties
    self.identifier = identifier
  }

  init(copying raw: mln_feature) throws {
    guard let rawGeometry = raw.geometry else {
      throw NativeStatusFailure
        .swiftNativeError("feature geometry pointer is null")
    }
    geometry = try NativeGeometry(copying: rawGeometry.pointee)
    properties = try Self.copyProperties(
      raw.properties,
      count: raw.property_count
    ).map { property in
      try NativeJSONMember(copying: property)
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
      identifier = try .string(NativeString.copyUTF8(
        data: raw.identifier.string_value.data,
        size: raw.identifier.string_value.size
      ))
    default:
      throw NativeStatusFailure
        .swiftNativeError(
          "unknown feature identifier type \(raw.identifier_type)"
        )
    }
  }

  private static func copyProperties(
    _ properties: UnsafePointer<mln_json_member>?,
    count: Int
  ) throws -> [mln_json_member] {
    guard count > 0 else { return [] }
    guard let properties
    else {
      throw NativeStatusFailure
        .swiftNativeError("feature properties pointer is null")
    }
    return (0 ..< count).map { properties[$0] }
  }
}

enum NativeGeoJSON: Equatable {
  case geometry(NativeGeometry)
  case feature(NativeFeature)
  case featureCollection([NativeFeature])
}

struct NativeQueriedFeature: Equatable {
  let feature: NativeFeature
  let sourceId: String?
  let sourceLayerId: String?
  let state: NativeJSONValue?

  init(
    feature: NativeFeature,
    sourceId: String? = nil,
    sourceLayerId: String? = nil,
    state: NativeJSONValue? = nil
  ) {
    self.feature = feature
    self.sourceId = sourceId
    self.sourceLayerId = sourceLayerId
    self.state = state
  }

  init(copying raw: mln_queried_feature) throws {
    feature = try NativeFeature(copying: raw.feature)
    sourceId = (raw.fields & MLN_QUERIED_FEATURE_SOURCE_ID.rawValue) != 0
      ? try NativeString.copyUTF8(
        data: raw.source_id.data,
        size: raw.source_id.size
      )
      : nil
    sourceLayerId = (
      raw.fields & MLN_QUERIED_FEATURE_SOURCE_LAYER_ID.rawValue
    ) !=
      0
      ? try NativeString.copyUTF8(
        data: raw.source_layer_id.data,
        size: raw.source_layer_id.size
      )
      : nil
    if (raw.fields & MLN_QUERIED_FEATURE_STATE.rawValue) != 0,
       let statePtr = raw.state
    {
      state = try NativeJSONValue(copying: statePtr.pointee)
    } else {
      state = nil
    }
  }
}

enum NativeFeatureExtensionResult: Equatable {
  case value(NativeJSONValue)
  case featureCollection([NativeFeature])

  init(copying raw: mln_feature_extension_result_info) throws {
    switch raw.type {
    case MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE.rawValue:
      guard let value = raw.data.value else {
        throw NativeStatusFailure
          .swiftNativeError("feature extension result value pointer is null")
      }
      self = try .value(NativeJSONValue(copying: value.pointee))
    case MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION.rawValue:
      let collection = raw.data.feature_collection
      let features = try Self.copyFeatures(
        collection.features,
        count: collection.feature_count
      ).map { feature in
        try NativeFeature(copying: feature)
      }
      self = .featureCollection(features)
    default:
      throw NativeStatusFailure
        .swiftNativeError("unknown feature extension result type \(raw.type)")
    }
  }

  private static func copyFeatures(
    _ features: UnsafePointer<mln_feature>?,
    count: Int
  ) throws -> [mln_feature] {
    guard count > 0 else { return [] }
    guard let features
    else {
      throw NativeStatusFailure
        .swiftNativeError(
          "feature extension result feature collection pointer is null"
        )
    }
    return (0 ..< count).map { features[$0] }
  }
}
