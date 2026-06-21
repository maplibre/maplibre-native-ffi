import Foundation

final class NativeHandleState: @unchecked Sendable {
  private enum State {
    case live(OpaquePointer)
    case closing(OpaquePointer)
    case closed
  }

  private let typeName: String
  private let lock = NSLock()
  private var state: State

  init(typeName: String, pointer: OpaquePointer?) throws {
    guard let pointer else {
      throw NativeStatusFailure(
        rawStatus: 0,
        diagnostic: "\(typeName) native handle is null"
      )
    }
    self.typeName = typeName
    state = .live(pointer)
  }

  deinit {
    if let pointer = lock.withLock({ leakPointer }) {
      NativeHandleLeakReporter.report(
        NativeHandleLeak(typeName: typeName, address: UInt(bitPattern: pointer))
      )
    }
  }

  var isClosed: Bool {
    lock.withLock {
      if case .closed = state { true } else { false }
    }
  }

  func requireLive() throws -> OpaquePointer {
    try lock.withLock {
      switch state {
      case let .live(pointer):
        return pointer
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

  func closeOnce(_ destroy: (OpaquePointer) throws -> Void) throws {
    let livePointer: OpaquePointer? = try lock.withLock {
      switch state {
      case let .live(pointer):
        state = .closing(pointer)
        return pointer
      case .closing:
        throw NativeStatusFailure(
          rawStatus: 0,
          diagnostic: "\(typeName) is closing"
        )
      case .closed:
        return nil
      }
    }
    guard let livePointer else { return }

    do {
      try destroy(livePointer)
      lock.withLock {
        state = .closed
      }
    } catch {
      lock.withLock {
        state = .live(livePointer)
      }
      throw error
    }
  }

  private var leakPointer: OpaquePointer? {
    switch state {
    case let .live(pointer), let .closing(pointer):
      pointer
    case .closed:
      nil
    }
  }
}
