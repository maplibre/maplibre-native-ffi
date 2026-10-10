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

  deinit {
    guard let handle = leakedHandle else { return }
    if NativeCallbackGuard.isActive {
      let retirement = NativeRetirement(
        handle: handle,
        parent: parent,
        typeName: typeName
      )
      DispatchQueue.global().async { retirement.run() }
    } else {
      defer { withExtendedLifetime(parent) {} }
      if !handle.disposeAbandoned() {
        NativeDiagnostics.report(.leakedHandle(
          typeName: typeName,
          handle: handle.raw,
          detail: ""
        ))
      }
    }
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
  init(handle: Handle, parent: AnyObject?, typeName: String) {
    self.handle = handle; self.parent = parent; self.typeName = typeName
  }

  func run() {
    defer { withExtendedLifetime(parent) {} }
    if !handle.disposeAbandoned() {
      NativeDiagnostics.report(.leakedHandle(
        typeName: typeName,
        handle: handle.raw,
        detail: ""
      ))
    }
  }
}
