enum NativeMemory {
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
