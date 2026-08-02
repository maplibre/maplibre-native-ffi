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
    case let .bool(value): .bool(value)
    case let .uint(value): .uint(value)
    case let .int(value): .int(value)
    case let .double(value): .double(value)
    case let .string(value): .string(value)
    case let .array(values): .array(values.map(\.nativeValue))
    case let .object(members): .object(members.map(\.nativeMember))
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
    case let .point(point): .point(point.nativeInput)
    case let .lineString(coordinates): .lineString(coordinates
        .map(\.nativeInput))
    case let .polygon(rings): .polygon(rings.map { $0.map(\.nativeInput) })
    case let .multiPoint(coordinates): .multiPoint(coordinates
        .map(\.nativeInput))
    case let .multiLineString(lines): .multiLineString(lines
        .map { $0.map(\.nativeInput) })
    case let .multiPolygon(polygons): .multiPolygon(polygons
        .map { $0.map { $0.map(\.nativeInput) } })
    case let .geometryCollection(geometries): .geometryCollection(geometries
        .map(\.nativeGeometry))
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
    case let .uint(value): .uint(value)
    case let .int(value): .int(value)
    case let .double(value): .double(value)
    case let .string(value): .string(value)
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
    case let .geometry(geometry): .geometry(geometry.nativeGeometry)
    case let .feature(feature): .feature(feature.nativeFeature)
    case let .featureCollection(features): .featureCollection(features
        .map(\.nativeFeature))
    }
  }
}
