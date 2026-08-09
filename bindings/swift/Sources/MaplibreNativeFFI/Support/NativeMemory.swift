internal import CMaplibreNativeC
import Foundation

enum NativeMemory {
  static func copyBuffer(_ buffer: NativeBufferHandle) throws -> Data {
    defer { mln_buffer_destroy(buffer.raw) }
    var view = mln_buffer_view()
    try checkStatus(mln_buffer_get(buffer.raw, &view))
    guard view.size > 0 else { return Data() }
    guard let data = view.data else {
      throw NativeStatusFailure.swiftNativeError(
        "buffer view has nil data with non-zero size"
      )
    }
    return Data(bytes: data, count: view.size)
  }

  static func withTemporary<Value, Result>(
    _ initialValue: Value,
    _ body: (UnsafeMutablePointer<Value>) throws -> Result
  ) throws -> (value: Value, result: Result) {
    var value = initialValue
    let result = try withUnsafeMutablePointer(to: &value, body)
    return (value, result)
  }

  static func withTemporaryArray<Element, Result>(
    _ values: [Element],
    _ body: (UnsafePointer<Element>?, Int) throws -> Result
  ) throws -> Result {
    try values.withUnsafeBufferPointer { buffer in
      try body(buffer.baseAddress, buffer.count)
    }
  }

  static func withTemporaryMutableBytes<Result>(
    _ bytes: inout [UInt8],
    _ body: (UnsafeMutablePointer<UInt8>?, Int) throws -> Result
  ) throws -> Result {
    try bytes.withUnsafeMutableBufferPointer { buffer in
      try body(buffer.baseAddress, buffer.count)
    }
  }
}
