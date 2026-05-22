public protocol NativeDescriptor {
  associatedtype NativeValue

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<NativeValue>) throws -> Result
  ) throws -> Result
}

public struct NativeDescriptorMaterializer<NativeValue>: NativeDescriptor {
  private let nativeValue: NativeValue

  public init(_ nativeValue: NativeValue) {
    self.nativeValue = nativeValue
  }

  public func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<NativeValue>) throws -> Result
  ) throws -> Result {
    var value = nativeValue
    return try withUnsafePointer(to: &value, body)
  }
}
