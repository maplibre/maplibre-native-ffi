import Foundation

final class NativeHandleState<Handle: NativeHandle>: @unchecked Sendable {
  private enum State {
    case live(Handle)
    case closing(Handle)
    case closed
  }

  private let typeName: String
  private let lock = NSLock()
  private var state: State
  private var readers = 0
  private var claims = 0
  private var claimed = false
  private var pendingDecision: Bool
  private let protocolOwner: Bool
  private var deferredClose: ((Handle) throws -> Void)?
  private let parent: AnyObject?
  let issued: Handle

  init(
    typeName: String,
    handle: Handle,
    parent: AnyObject? = nil,
    pendingDecision: Bool = false
  ) throws {
    guard !handle.isNull else {
      throw MaplibreError.invalidArgument(
        "\(typeName) native handle is the null handle"
      )
    }
    self.typeName = typeName
    self.parent = parent
    self.pendingDecision = pendingDecision
    protocolOwner = pendingDecision
    issued = handle
    state = .live(handle)
  }

  /// Explicit close is the contract, so an open handle that its last owner
  /// releases is a leak: deinit disposes it and reports it. A handle that the
  /// binding drops itself, such as a creation that arrives after its wait is
  /// cancelled, goes through retire() instead and is not reported.
  deinit {
    guard let handle = leakedHandle else { return }
    NativeRetirement(
      handle: handle,
      parent: parent,
      typeName: typeName,
      reported: true
    ).schedule()
  }

  /// Disposes a live handle that no caller ever received. Disposal reports
  /// only a failure, since the binding, not its caller, dropped the handle.
  func retire() {
    let handle = lock.withLock { () -> Handle? in
      guard case let .live(handle) = state, !pendingDecision, readers == 0,
            claims == 0 else { return nil }
      state = .closed
      return handle
    }
    guard let handle else { return }
    NativeRetirement(
      handle: handle,
      parent: parent,
      typeName: typeName,
      reported: false
    ).schedule()
  }

  var isClosed: Bool {
    lock.withLock {
      if case .closed = state { true } else { false }
    }
  }

  func requireLive() throws -> Handle {
    try lock.withLock { try requireLiveLocked() }
  }

  func borrow() throws -> NativeHandleRead<Handle> {
    try lock.withLock {
      let handle = try requireLiveLocked()
      readers += 1
      return NativeHandleRead(handle: handle) { [self] in
        lock.withLock { readers -= 1 }
        finishDeferredClose()
      }
    }
  }

  private func requireLiveLocked() throws -> Handle {
    switch state {
    case let .live(handle):
      return handle
    case .closing:
      throw MaplibreError.invalidState("\(typeName) is closing")
    case .closed:
      throw MaplibreError.invalidState("\(typeName) is closed")
    }
  }

  func closeOnce(_ destroy: @escaping (Handle) throws -> Void) throws {
    let liveHandle: Handle? = try lock.withLock {
      switch state {
      case let .live(handle):
        if pendingDecision || (protocolOwner && (claims > 0 || readers > 0)) {
          if pendingDecision { claimed = true }
          deferredClose = destroy
          return nil
        }
        guard readers == 0 else {
          throw MaplibreError.invalidState("\(typeName) is in use")
        }
        state = .closing(handle)
        return handle
      case .closing:
        throw MaplibreError.invalidState("\(typeName) is closing")
      case .closed:
        return nil
      }
    }
    guard let liveHandle else { return }

    do {
      try destroy(liveHandle)
      lock.withLock { state = .closed }
    } catch {
      lock.withLock {
        state = .live(liveHandle)
      }
      throw error
    }
  }

  func beginClaim() throws -> NativeClaim {
    try lock.withLock {
      _ = try requireLiveLocked()
      claims += 1
    }
    return NativeClaim { [self] accepted in
      lock.withLock {
        if accepted { claimed = true }
        claims -= 1
      }
      finishDeferredClose()
    }
  }

  func finishDecision(accepted: Bool) -> Bool {
    let owns = lock.withLock {
      let owns = accepted || claimed || claims > 0
      pendingDecision = false
      if !owns {
        state = .closed
        deferredClose = nil
      }
      return owns
    }
    finishDeferredClose()
    return owns
  }

  private func finishDeferredClose() {
    let pending = lock.withLock { () -> ((Handle) throws -> Void)? in
      guard !pendingDecision, claims == 0, readers == 0 else { return nil }
      let pending = deferredClose
      deferredClose = nil
      return pending
    }
    guard let pending else { return }
    do { try closeOnce(pending) }
    catch {
      NativeDiagnostics.report(.leakedHandle(
        typeName: typeName,
        handle: issued.raw,
        detail: "deferred release failed: \(error)"
      ))
    }
  }

  private var leakedHandle: Handle? {
    switch state {
    case let .live(handle), let .closing(handle):
      handle
    case .closed:
      nil
    }
  }
}

final class NativeClaim {
  private var accepted = false
  private var finish: ((Bool) -> Void)?
  init(_ finish: @escaping (Bool) -> Void) {
    self.finish = finish
  }

  func accept() {
    accepted = true
  }

  func end() {
    let callback = finish
    finish = nil
    callback?(accepted)
  }

  deinit { end() }
}

final class NativeHandleRead<Handle: NativeHandle> {
  let handle: Handle
  private var release: (() -> Void)?

  init(handle: Handle, release: @escaping () -> Void) {
    self.handle = handle
    self.release = release
  }

  func end() {
    let callback = release
    release = nil
    callback?()
  }

  deinit { end() }
}

private final class NativeRetirement<Handle: NativeHandle>: @unchecked Sendable {
  let handle: Handle
  let parent: AnyObject?
  let typeName: String
  /// Whether a disposal that succeeds is still reported as a leak.
  let reported: Bool
  init(handle: Handle, parent: AnyObject?, typeName: String, reported: Bool) {
    self.handle = handle; self.parent = parent; self.typeName = typeName
    self.reported = reported
  }

  /// Disposes the handle now, or off the stack of the callback that is
  /// running.
  func schedule() {
    if NativeCallbackGuard.isActive {
      DispatchQueue.global().async { self.run() }
    } else {
      run()
    }
  }

  private func run() {
    defer { withExtendedLifetime(parent) {} }
    let disposed = handle.disposeAbandoned()
    guard reported || !disposed else { return }
    NativeDiagnostics.report(.leakedHandle(
      typeName: typeName,
      handle: handle.raw,
      detail: disposed ? "" : "native disposal failed"
    ))
  }
}
