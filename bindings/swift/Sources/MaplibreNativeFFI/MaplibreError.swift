public struct MaplibreError: Error, Sendable, Equatable,
  CustomStringConvertible
{
  public let status: Status
  public let diagnostic: String

  public init(status: Status, diagnostic: String) {
    self.status = status
    self.diagnostic = diagnostic
  }

  public var description: String {
    "\(status): \(diagnostic)"
  }

  static func invalidArgument(_ diagnostic: String) -> Self {
    Self(status: .invalidArgument, diagnostic: diagnostic)
  }

  static func invalidState(_ diagnostic: String) -> Self {
    Self(status: .invalidState, diagnostic: diagnostic)
  }
}
