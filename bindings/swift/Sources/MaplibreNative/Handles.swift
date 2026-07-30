
/// Its only stored property is the lock-guarded `NativeHandleState`, so the box
/// itself is safe to share. This is what lets `MapAttachRef` be plainly
/// `Sendable` rather than `@unchecked`. The public handles that hold a box stay
/// non-`Sendable`, so this does not make any of them crossable.
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

  /// Runs `use` with release held off. See `NativeHandleState.withLive`.
  func withLive<T>(_ use: (Handle) throws -> T) throws -> T {
    do {
      return try state.withLive(use)
    } catch let failure as NativeStatusFailure {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: failure.diagnostic
      )
    }
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
