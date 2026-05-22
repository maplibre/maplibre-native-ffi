import CMaplibreNativeC

public struct NativeStringError: Error, Equatable, Sendable {
  public let message: String

  public init(_ message: String) {
    self.message = message
  }
}

public enum NativeString {
  public static func withCString<Result>(
    _ text: String,
    _ body: (UnsafePointer<CChar>) throws -> Result
  ) throws -> Result {
    if text.utf8.contains(0) {
      throw NativeStringError("C string inputs cannot contain embedded NUL bytes")
    }
    return try text.withCString(body)
  }

  public static func withStringView<Result>(
    _ text: String,
    _ body: (mln_string_view) throws -> Result
  ) throws -> Result {
    let bytes = Array(text.utf8)
    return try bytes.withUnsafeBufferPointer { buffer in
      let pointer = buffer.baseAddress.map { UnsafeRawPointer($0).assumingMemoryBound(to: CChar.self) }
      return try body(mln_string_view(data: pointer, size: buffer.count))
    }
  }
}
