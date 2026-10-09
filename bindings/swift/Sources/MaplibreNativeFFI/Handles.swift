/// Holds a public handle's native handle and reports its lifecycle and close
/// failures as ``MaplibreError``. Its only stored property is the lock-guarded
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
    state = try NativeHandleState(
      typeName: typeName,
      handle: handle,
      parent: parent,
      pendingDecision: pendingDecision
    )
  }

  var isClosed: Bool {
    state.isClosed
  }

  var issued: Handle {
    state.issued
  }

  func borrow() throws -> NativeHandleRead<Handle> {
    try state.borrow()
  }

  func requireLive() throws -> Handle {
    try state.requireLive()
  }

  func closeOnce(_ destroy: @escaping (Handle) throws -> Void) throws {
    try mapNativeFailure { try state.closeOnce(destroy) }
  }

  func beginClaim() throws -> NativeClaim {
    try state.beginClaim()
  }

  func finishDecision(accepted: Bool) -> Bool {
    state.finishDecision(accepted: accepted)
  }
}
