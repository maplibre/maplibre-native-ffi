internal import CMaplibreNativeC
import Foundation

struct NativeStringError: Error, Equatable {
  let message: String

  init(_ message: String) {
    self.message = message
  }
}

enum NativeString {
  static func copyUTF8(data: UnsafePointer<CChar>?,
                       size: UInt) throws -> String
  {
    try copyUTF8(data: data, size: Int(size))
  }

  static func copyUTF8(data: UnsafePointer<CChar>?,
                       size: Int) throws -> String
  {
    guard size > 0 else { return "" }
    guard let data else {
      throw NativeStringError(
        "UTF-8 string view has nil data with non-zero size"
      )
    }
    let bytes = UnsafeBufferPointer(
      start: UnsafeRawPointer(data).assumingMemoryBound(to: UInt8.self),
      count: size
    )
    guard let text = String(bytes: bytes, encoding: .utf8) else {
      throw NativeStringError("UTF-8 string view contains invalid bytes")
    }
    return text
  }

  static func copyCString(_ data: UnsafePointer<CChar>?) -> String {
    data.map { String(cString: $0) } ?? ""
  }

  static func withOptionalCString<Result>(
    _ text: String?,
    _ body: (UnsafePointer<CChar>?) throws -> Result
  ) throws -> Result {
    guard let text else {
      return try body(nil)
    }
    return try withCString(text, body)
  }

  static func withCString<Result>(
    _ text: String,
    _ body: (UnsafePointer<CChar>) throws -> Result
  ) throws -> Result {
    if text.utf8.contains(0) {
      throw NativeStringError(
        "C string inputs cannot contain embedded NUL bytes"
      )
    }
    return try text.withCString(body)
  }

  static func withStringView<Result>(
    _ text: String,
    _ body: (mln_string_view) throws -> Result
  ) throws -> Result {
    let bytes = Array(text.utf8)
    return try bytes.withUnsafeBufferPointer { buffer in
      let pointer = buffer.baseAddress
        .map { UnsafeRawPointer($0).assumingMemoryBound(to: CChar.self) }
      return try body(mln_string_view(data: pointer, size: buffer.count))
    }
  }
}
