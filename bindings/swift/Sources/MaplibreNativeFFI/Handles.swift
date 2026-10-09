/// Holds a public handle's native handle and reports its state failures as
/// ``MaplibreError``. Its only stored property is the lock-guarded
/// `NativeHandleState`, so the box itself is safe to share. Each public handle
/// chooses whether its API contract permits sharing.
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

  func borrow() throws -> NativeHandleRead<Handle> {
    do { return try state.borrow() }
    catch let failure as NativeStatusFailure {
      throw MaplibreError.invalidState(failure.diagnostic)
    }
  }

  func requireLive() throws -> Handle {
    do {
      return try state.requireLive()
    } catch let failure as NativeStatusFailure {
      throw MaplibreError.invalidState(failure.diagnostic)
    }
  }

  func closeOnce(_ destroy: @escaping (Handle) throws -> Void) throws {
    do {
      try state.closeOnce(destroy)
    } catch let failure as NativeStatusFailure {
      if failure.rawStatus == 0 {
        throw MaplibreError.invalidState(failure.diagnostic)
      }
      throw MaplibreError.fromNativeFailure(failure)
    }
  }

  func beginClaim() throws -> NativeClaim {
    try state.beginClaim()
  }

  func finishDecision(accepted: Bool) -> Bool {
    state.finishDecision(accepted: accepted)
  }
}
