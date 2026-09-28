
/// Its only stored property is the lock-guarded `NativeHandleState`, so the box
/// itself is safe to share. Each public handle chooses whether its API contract
/// permits sharing.
class NativeHandleBox<Handle: NativeHandle>: @unchecked Sendable {
  private let state: NativeHandleState<Handle>

  init(
    typeName: String,
    handle: Handle,
    parent: AnyObject? = nil,
    pendingDecision: Bool = false
  ) throws {
    do {
      state = try NativeHandleState(
        typeName: typeName,
        handle: handle,
        parent: parent,
        pendingDecision: pendingDecision
      )
    } catch let failure as NativeStatusFailure {
      throw MaplibreError.invalidArgument(failure.diagnostic)
    }
  }

  var isClosed: Bool {
    state.isClosed
  }

  var issued: Handle {
    state.issued
  }

  /// Holds the owner through the native call and any borrowed-data copy.
  func withLive<T>(_ use: (Handle) throws -> T) throws -> T {
    let access = try borrow()
    defer { access.end() }
    return try use(access.handle)
  }

  func borrow() throws -> NativeHandleRead<Handle> {
    do { return try state.borrow() }
    catch let failure as NativeStatusFailure {
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

  func closeOnce(_ destroy: @escaping (Handle) throws -> Void) throws {
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

  func retainCallback(_ callback: AnyObject) {
    state.retainCallback(callback)
  }

  func retireCallback(_ callback: AnyObject) {
    state.retireCallback(callback)
  }

  func beginClaim() throws -> NativeClaim {
    try state.beginClaim()
  }

  func finishDecision(accepted: Bool) -> Bool {
    state.finishDecision(accepted: accepted)
  }
}
