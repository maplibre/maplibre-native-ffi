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

  static func fromNativeFailure(_ failure: NativeStatusFailure) -> Self {
    Self(
      status: Status(rawValue: failure.rawStatus),
      diagnostic: failure.diagnostic
    )
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
