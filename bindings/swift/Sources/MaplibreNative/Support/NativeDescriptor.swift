protocol NativeDescriptor {
  associatedtype NativeValue

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<NativeValue>) throws -> Result
  ) throws -> Result
}

struct NativeDescriptorMaterializer<NativeValue>: NativeDescriptor {
  private let nativeValue: NativeValue

  init(_ nativeValue: NativeValue) {
    self.nativeValue = nativeValue
  }

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<NativeValue>) throws -> Result
  ) throws -> Result {
    var value = nativeValue
    return try withUnsafePointer(to: &value, body)
  }
}
