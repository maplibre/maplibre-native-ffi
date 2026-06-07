import Foundation

final class NativeResultGuard: @unchecked Sendable {
  private let typeName: String
  private let destroy: @Sendable (OpaquePointer) -> Void
  private let lock = NSLock()
  private var pointer: OpaquePointer?

  init(
    typeName: String,
    pointer: OpaquePointer?,
    destroy: @escaping @Sendable (OpaquePointer) -> Void
  ) throws {
    guard let pointer else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "\(typeName) native result is null")
    }
    self.typeName = typeName
    self.pointer = pointer
    self.destroy = destroy
  }

  deinit {
    close()
  }

  func requireLive() throws -> OpaquePointer {
    try lock.withLock {
      guard let pointer else {
        throw NativeStatusFailure(rawStatus: 0, diagnostic: "\(typeName) is closed")
      }
      return pointer
    }
  }

  func close() {
    let livePointer = lock.withLock {
      let livePointer = pointer
      pointer = nil
      return livePointer
    }
    if let livePointer {
      destroy(livePointer)
    }
  }
}
