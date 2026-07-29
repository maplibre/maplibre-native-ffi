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

  /// Runs `use` with the handle held live for the duration of the call.
  ///
  /// `closeOnce` takes the same lock to move the handle out of `.live`, so a
  /// close waits rather than destroying the handle midway through `use`. Do
  /// not
  /// re-enter this handle from `use`; the lock is not recursive.
  func withLive<T>(_ use: (Handle) throws -> T) throws -> T {
    try lock.withLock {
      switch state {
      case let .live(handle):
        return try use(handle)
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
  }

  func requireLive() throws -> Handle {
    try lock.withLock {
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
  }

  func closeOnce(_ destroy: (Handle) throws -> Void) throws {
    let liveHandle: Handle? = try lock.withLock {
      switch state {
      case let .live(handle):
        state = .closing(handle)
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
