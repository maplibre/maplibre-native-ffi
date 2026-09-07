public enum MaplibreErrorKind: Sendable, Equatable {
  case invalidArgument
  case invalidState
  case wrongThread
  case unsupported
  case nativeError
  /// The operation reached its terminal cancelled disposition.
  case cancelled
  /// A conflicting driver call or lifecycle transition is in flight.
  case busy
  /// The render target or graphics receiver was irreversibly lost.
  case targetLost
  /// A nonblocking acquisition or service call has no result yet.
  case notReady
  /// A command or operation named an ID with no live object behind it.
  case notFound
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
    Self(
      kind: kind(forRawStatus: failure.rawStatus),
      rawStatus: failure.isNativeStatus ? failure.rawStatus : nil,
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
    case -6: .cancelled
    case -7: .busy
    case -8: .targetLost
    case -9: .notReady
    case -10: .notFound
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

func mapNativeFailure<T>(_ body: () async throws -> T) async throws -> T {
  do {
    return try await body()
  } catch let failure as NativeStatusFailure {
    throw MaplibreError.fromNativeFailure(failure)
  } catch let error as NativeStringError {
    throw MaplibreError.invalidArgument(error.message)
  }
}

/// Starts native work and awaits its completion, reporting both halves as a
/// ``MaplibreError``.
///
/// The synchronous start and the awaited completion each raise the binding's
/// internal failure type, so both have to cross the same translation; awaiting
/// outside it would surface an internal error to the host.
func awaitNative<T: Sendable>(
  _ start: () throws -> NativeFuture<T>
) async throws -> T {
  let future = try mapNativeFailure(start)
  return try await mapNativeFailure { try await future.value() }
}
