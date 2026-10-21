/// The loaded C library and this binding have different ABI versions.
public struct AbiVersionMismatchError: Error, Sendable, Equatable {
  public let expected: UInt32
  public let actual: UInt32
}
