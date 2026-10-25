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

  /// Makes a call that writes UTF-8 text into storage the caller sizes, and
  /// copies the text into `text`. A call reports the text's length whether or
  /// not it fit, and reports the same text for the same inputs, so a call
  /// that did not fit is made again with enough storage.
  static func callerSized(
    _ text: inout String,
    _ call: (UnsafeMutablePointer<CChar>, Int, UnsafeMutablePointer<Int>) throws
      -> mln_status
  ) throws -> mln_status {
    var storage = [CChar](repeating: 0, count: 256)
    while true {
      var length = 0
      let status = try storage.withUnsafeMutableBufferPointer {
        try call($0.baseAddress!, $0.count, &length)
      }
      guard status == MLN_STATUS_OK else { return status }
      if length < storage.count {
        text = try storage.withUnsafeBytes {
          try copyUTF8(data: $0.baseAddress, size: length)
        }
        return status
      }
      storage = [CChar](repeating: 0, count: length + 1)
    }
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
}
