import Foundation

/// The loaded C library and this binding have different ABI versions.
public struct AbiVersionMismatchError: LocalizedError, Sendable, Equatable,
  CustomStringConvertible
{
  public let expected: UInt32
  public let actual: UInt32

  public var description: String {
    "unsupported MapLibre Native C ABI version \(actual); this binding supports \(expected)"
  }

  public var errorDescription: String? {
    description
  }
}
