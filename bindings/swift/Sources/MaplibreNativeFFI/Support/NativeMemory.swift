enum NativeMemory {
  static func withTemporary<Value, Result>(
    _ initialValue: Value,
    _ body: (UnsafeMutablePointer<Value>) throws -> Result
  ) throws -> (value: Value, result: Result) {
    var value = initialValue
    let result = try withUnsafeMutablePointer(to: &value, body)
    return (value, result)
  }
}
