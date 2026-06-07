public enum NetworkStatus: Sendable, Equatable {
  case online
  case offline
  case unknown(UInt32)

  static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 1: .online
    case 2: .offline
    default: .unknown(rawValue)
    }
  }

  var nativeValue: UInt32 {
    get throws {
      switch self {
      case .online: 1
      case .offline: 2
      case .unknown(let rawValue):
        throw MaplibreError.invalidArgument("unknown network status \(rawValue) cannot be set")
      }
    }
  }
}
