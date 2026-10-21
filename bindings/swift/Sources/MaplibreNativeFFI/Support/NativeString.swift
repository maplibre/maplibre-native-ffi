internal import MaplibreNativeC
import Foundation

enum NativeString {
  static func copyData(data: UnsafeRawPointer?, size: Int) throws -> Data {
    guard size >= 0
    else { throw MaplibreError.invalidArgument("negative native buffer size") }
    guard size > 0 else { return Data() }
    guard let data
    else {
      throw MaplibreError
        .invalidArgument("null native buffer with nonzero size")
    }
    return Data(bytes: data, count: size)
  }

  static func copyUTF8(data: UnsafeRawPointer?, size: Int) throws -> String {
    try copyCUTF8(
      data: data?.assumingMemoryBound(to: CChar.self),
      size: size
    )
  }

  private static func copyCUTF8(data: UnsafePointer<CChar>?,
                                size: Int) throws -> String
  {
    guard size >= 0
    else { throw MaplibreError.invalidArgument("negative native string size") }
    guard size > 0 else { return "" }
    guard let data else {
      throw MaplibreError.invalidArgument(
        "UTF-8 string view has nil data with non-zero size"
      )
    }
    let bytes = UnsafeBufferPointer(
      start: UnsafeRawPointer(data).assumingMemoryBound(to: UInt8.self),
      count: size
    )
    guard let text = String(bytes: bytes, encoding: .utf8) else {
      throw MaplibreError
        .invalidArgument("UTF-8 string view contains invalid bytes")
    }
    return text
  }

  static func copyCString(_ data: UnsafePointer<CChar>?) throws -> String {
    guard let data, let value = String(validatingCString: data) else {
      throw MaplibreError
        .invalidArgument("native C string is null or invalid UTF-8")
    }
    return value
  }
}
