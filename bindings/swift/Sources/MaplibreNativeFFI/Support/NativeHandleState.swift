import Foundation

final class NativeHandleState<Handle: NativeHandle>: @unchecked Sendable {
  private enum State {
    case live(Handle)
    case closing(Handle)
    case closed
  }

  private let typeName: String
  private let lock = NSCondition()
  private var state: State
  private var activeUses = 0

  init(typeName: String, handle: Handle) throws {
    guard !handle.isNull else {
      throw NativeStatusFailure(
        rawStatus: 0,
        diagnostic: "\(typeName) native handle is the null handle"
      )
    }
    self.typeName = typeName
    state = .live(handle)
  }

  deinit {
    if let handle = lock.withLock({ leakedHandle }) {
      NativeHandleLeakReporter.report(
        NativeHandleLeak(typeName: typeName, handle: handle.raw)
      )
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

  private func requireLiveLocked() throws -> Handle {
    switch state {
    case let .live(handle):
      return handle
    case .closing:
      throw NativeStatusFailure(
        rawStatus: 0,
        diagnostic: "\(typeName) is closing"
      )
    case .closed:
      throw NativeStatusFailure(
        rawStatus: 0,
        diagnostic: "\(typeName) is closed"
      )
    }
  }

  /// Runs `use` with the handle and with release held off until it returns.
  ///
  /// Handles whose release is confined to one thread get this ordering from the
  /// owner-thread rule
  /// and can call native directly after `requireLive()`. Handles the host may
  /// use and release from
  /// different threads use this instead, so a release that begins mid-call
  /// waits for the call to
  /// finish. That is what keeps a losing race reporting this wrapper's own
  /// closed-handle error
  /// instead of the C API's rejection of an id retired underneath it.
  ///
  /// `use` runs outside the lock, so concurrent uses proceed together. Calling
  /// `closeOnce` from
  /// inside `use` on the same thread would wait on itself.
  func withLive<T>(_ use: (Handle) throws -> T) throws -> T {
    let handle = try lock.withLock {
      let handle = try requireLiveLocked()
      activeUses += 1
      return handle
    }
    defer {
      lock.withLock {
        activeUses -= 1
        lock.broadcast()
      }
    }
    return try use(handle)
  }

  func closeOnce(_ destroy: (Handle) throws -> Void) throws {
    let liveHandle: Handle? = try lock.withLock {
      switch state {
      case let .live(handle):
        state = .closing(handle)
        // Closing turns new uses away from here on. Uses that already passed
        // their liveness check
        // still hold the handle, so wait for them before destroying it.
        while activeUses > 0 {
          lock.wait()
        }
        return handle
      case .closing:
        throw NativeStatusFailure(
          rawStatus: 0,
          diagnostic: "\(typeName) is closing"
        )
      case .closed:
        return nil
      }
    }
    guard let liveHandle else { return }

    do {
      try destroy(liveHandle)
      lock.withLock {
        state = .closed
      }
    } catch {
      lock.withLock {
        state = .live(liveHandle)
      }
      throw error
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
