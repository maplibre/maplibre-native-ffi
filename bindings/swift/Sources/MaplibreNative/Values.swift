import Foundation

public enum JSONValue: Equatable, Sendable {
  case null
  case bool(Bool)
  case uint(UInt64)
  case int(Int64)
  case double(Double)
  case string(String)
  case array([JSONValue])
  case object([JSONMember])

  var nativeValue: NativeJSONValue {
    switch self {
    case .null: .null
    case .bool(let value): .bool(value)
    case .uint(let value): .uint(value)
    case .int(let value): .int(value)
    case .double(let value): .double(value)
    case .string(let value): .string(value)
    case .array(let values): .array(values.map(\.nativeValue))
    case .object(let members): .object(members.map(\.nativeMember))
    }
  }
}

public struct JSONMember: Equatable, Sendable {
  public let key: String
  public let value: JSONValue

  public init(key: String, value: JSONValue) {
    self.key = key
    self.value = value
  }

  var nativeMember: NativeJSONMember {
    NativeJSONMember(key: key, value: value.nativeValue)
  }
}

public enum Geometry: Equatable, Sendable {
  case empty
  case point(LatLng)
  case lineString([LatLng])
  case polygon([[LatLng]])
  case multiPoint([LatLng])
  case multiLineString([[LatLng]])
  case multiPolygon([[[LatLng]]])
  case geometryCollection([Geometry])

  var nativeGeometry: NativeGeometry {
    switch self {
    case .empty: .empty
    case .point(let point): .point(point.nativeInput)
    case .lineString(let coordinates): .lineString(coordinates.map(\.nativeInput))
    case .polygon(let rings): .polygon(rings.map { $0.map(\.nativeInput) })
    case .multiPoint(let coordinates): .multiPoint(coordinates.map(\.nativeInput))
    case .multiLineString(let lines): .multiLineString(lines.map { $0.map(\.nativeInput) })
    case .multiPolygon(let polygons): .multiPolygon(polygons.map { $0.map { $0.map(\.nativeInput) } })
    case .geometryCollection(let geometries): .geometryCollection(geometries.map(\.nativeGeometry))
    }
  }
}

public enum FeatureIdentifier: Equatable, Sendable {
  case none
  case uint(UInt64)
  case int(Int64)
  case double(Double)
  case string(String)

  var nativeIdentifier: NativeFeatureIdentifier {
    switch self {
    case .none: .none
    case .uint(let value): .uint(value)
    case .int(let value): .int(value)
    case .double(let value): .double(value)
    case .string(let value): .string(value)
    }
  }
}

public struct Feature: Equatable, Sendable {
  public let geometry: Geometry
  public let properties: [JSONMember]
  public let identifier: FeatureIdentifier

  public init(
    geometry: Geometry,
    properties: [JSONMember] = [],
    identifier: FeatureIdentifier = .none
  ) {
    self.geometry = geometry
    self.properties = properties
    self.identifier = identifier
  }

  var nativeFeature: NativeFeature {
    NativeFeature(
      geometry: geometry.nativeGeometry,
      properties: properties.map(\.nativeMember),
      identifier: identifier.nativeIdentifier
    )
  }
}

public enum GeoJSON: Equatable, Sendable {
  case geometry(Geometry)
  case feature(Feature)
  case featureCollection([Feature])

  var nativeGeoJSON: NativeGeoJSON {
    switch self {
    case .geometry(let geometry): .geometry(geometry.nativeGeometry)
    case .feature(let feature): .feature(feature.nativeFeature)
    case .featureCollection(let features): .featureCollection(features.map(\.nativeFeature))
    }
  }
}
