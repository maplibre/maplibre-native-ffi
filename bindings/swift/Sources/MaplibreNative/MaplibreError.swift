
public enum MaplibreErrorKind: Sendable, Equatable {
  case invalidArgument
  case invalidState
  case wrongThread
  case unsupported
  case nativeError
  case unknownStatus
}

public struct MaplibreError: Error, Sendable, Equatable,
  CustomStringConvertible
{
  public let kind: MaplibreErrorKind
  public let rawStatus: Int32?
  public let diagnostic: String

  public init(kind: MaplibreErrorKind, rawStatus: Int32?, diagnostic: String) {
    self.kind = kind
    self.rawStatus = rawStatus
    self.diagnostic = diagnostic
  }

  public var description: String {
    if let rawStatus {
      return "MapLibre Native status \(rawStatus): \(diagnostic)"
    }
    return diagnostic
  }

  static func invalidArgument(_ diagnostic: String) -> Self {
    Self(kind: .invalidArgument, rawStatus: nil, diagnostic: diagnostic)
  }

  static func fromNativeFailure(_ failure: NativeStatusFailure) -> Self {
    if !failure.isNativeStatus {
      return Self(
        kind: kind(forRawStatus: failure.rawStatus),
        rawStatus: nil,
        diagnostic: failure.diagnostic
      )
    }
    return Self(
      kind: kind(forRawStatus: failure.rawStatus),
      rawStatus: failure.rawStatus,
      diagnostic: failure.diagnostic
    )
  }

  private static func kind(forRawStatus rawStatus: Int32) -> MaplibreErrorKind {
    switch rawStatus {
    case -1: .invalidArgument
    case -2: .invalidState
    case -3: .wrongThread
    case -4: .unsupported
    case -5: .nativeError
    default: .unknownStatus
    }
  }
}

func mapNativeFailure<T>(_ body: () throws -> T) throws -> T {
  do {
    return try body()
  } catch let failure as NativeStatusFailure {
    throw MaplibreError.fromNativeFailure(failure)
  } catch let error as NativeStringError {
    throw MaplibreError.invalidArgument(error.message)
  }
}
