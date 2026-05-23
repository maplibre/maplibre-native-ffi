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
}

public struct NativeJSONMember: Equatable, Sendable {
  public let key: String
  public let value: NativeJSONValue

  public init(key: String, value: NativeJSONValue) {
    self.key = key
    self.value = value
  }
}

public final class NativeJSONArena {
  private var strings: [ContiguousArray<UInt8>] = []
  private var values: [UnsafeMutablePointer<mln_json_value>] = []
  private var arrays: [UnsafeMutableBufferPointer<mln_json_value>] = []
  private var members: [UnsafeMutableBufferPointer<mln_json_member>] = []

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
}

public enum NativeGeometry: Equatable, Sendable {
  case empty
  case point(NativeLatLng)
  case lineString([NativeLatLng])
  case polygon([[NativeLatLng]])
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
}

public enum NativeFeatureExtensionResult: Equatable, Sendable {
  case value(NativeJSONValue)
  case featureCollection([NativeFeature])
}
