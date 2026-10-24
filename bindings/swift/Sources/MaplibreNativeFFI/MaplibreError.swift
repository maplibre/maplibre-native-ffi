import Foundation

/// A native status and the diagnostic that native copied for it.
public struct MaplibreError: LocalizedError, Sendable, Equatable,
  CustomStringConvertible
{
  public let status: Status
  public let diagnostic: String
  /// The map snapshot generation that a failed or cancelled map command
  /// published, and zero for any other failure.
  public let generation: UInt64

  public init(status: Status, diagnostic: String, generation: UInt64 = 0) {
    self.status = status
    self.diagnostic = diagnostic
    self.generation = generation
  }

  public var description: String {
    diagnostic.isEmpty ? "\(status)" : "\(status): \(diagnostic)"
  }

  public var errorDescription: String? {
    description
  }

  static func invalidArgument(_ diagnostic: String) -> Self {
    Self(status: .invalidArgument, diagnostic: diagnostic)
  }

  static func invalidState(_ diagnostic: String) -> Self {
    Self(status: .invalidState, diagnostic: diagnostic)
  }
}
