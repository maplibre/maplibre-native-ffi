import Foundation

public final class NativeHandleState: @unchecked Sendable {
  private let typeName: String
  private let lock = NSLock()
  private var pointer: OpaquePointer?

  public init(typeName: String, pointer: OpaquePointer?) throws {
    guard let pointer else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "\(typeName) native handle is null")
    }
    self.typeName = typeName
    self.pointer = pointer
  }

  public var isClosed: Bool {
    lock.withLock { pointer == nil }
  }

  public func requireLive() throws -> OpaquePointer {
    try lock.withLock {
      guard let pointer else {
        throw NativeStatusFailure(rawStatus: 0, diagnostic: "\(typeName) is closed")
      }
      return pointer
    }
  }

  public func closeOnce(_ destroy: (OpaquePointer) throws -> Void) throws {
    let livePointer = lock.withLock {
      guard let pointer else { return nil as OpaquePointer? }
      return pointer
    }
    guard let livePointer else { return }

    try destroy(livePointer)

    lock.withLock {
      if pointer == livePointer {
        pointer = nil
      }
    }
  }
}
