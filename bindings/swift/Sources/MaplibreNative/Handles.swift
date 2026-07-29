
/// Its only stored property is the lock-guarded `NativeHandleState`, so the box
/// itself is safe to share. This is what lets `MapAttachRef` be plainly
/// `Sendable` rather than `@unchecked`. The public handles that hold a box stay
/// non-`Sendable`, so this does not make any of them crossable.
class NativeHandleBox: @unchecked Sendable {
  private let state: NativeHandleState

  init(typeName: String, pointer: OpaquePointer?) throws {
    do {
      state = try NativeHandleState(typeName: typeName, pointer: pointer)
    } catch let failure as NativeStatusFailure {
      throw MaplibreError.invalidArgument(failure.diagnostic)
    }
  }

  var isClosed: Bool {
    state.isClosed
  }

  func withLive<T>(_ use: (OpaquePointer) throws -> T) throws -> T {
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

  func requireLive() throws -> OpaquePointer {
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

  func closeOnce(_ destroy: (OpaquePointer) throws -> Void) throws {
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
