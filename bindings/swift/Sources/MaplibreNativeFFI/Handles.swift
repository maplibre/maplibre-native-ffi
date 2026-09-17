
/// Its only stored property is the lock-guarded `NativeHandleState`, so the box
/// itself is safe to share. Each public handle chooses whether its API contract
/// permits sharing.
class NativeHandleBox<Handle: NativeHandle>: @unchecked Sendable {
  private let state: NativeHandleState<Handle>

  init(typeName: String, handle: Handle) throws {
    do {
      state = try NativeHandleState(typeName: typeName, handle: handle)
    } catch let failure as NativeStatusFailure {
      throw MaplibreError.invalidArgument(failure.diagnostic)
    }
  }

  var isClosed: Bool {
    state.isClosed
  }

  /// Runs `use` after checking that this wrapper still owns the handle. The C
  /// API leases its native object for each entry point, so concurrent release
  /// does not need a second binding-side active-use lease.
  func withLive<T>(_ use: (Handle) throws -> T) throws -> T {
    try use(requireLive())
  }

  func requireLive() throws -> Handle {
    do {
      return try state.requireLive()
    } catch let failure as NativeStatusFailure {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: failure.diagnostic
      )
    }
  }

  func closeOnce(_ destroy: (Handle) throws -> Void) throws {
    do {
      try state.closeOnce(destroy)
    } catch let failure as NativeStatusFailure {
      if failure.rawStatus == 0 {
        throw MaplibreError(
          kind: .invalidState,
          rawStatus: nil,
          diagnostic: failure.diagnostic
        )
      }
      throw MaplibreError.fromNativeFailure(failure)
    }
  }
}
