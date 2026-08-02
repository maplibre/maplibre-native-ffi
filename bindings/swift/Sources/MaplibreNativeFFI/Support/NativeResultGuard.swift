import Foundation

final class NativeResultGuard<Handle: NativeHandle>: @unchecked Sendable {
  private let typeName: String
  private let destroy: @Sendable (Handle) -> Void
  private let lock = NSLock()
  private var handle: Handle?

  init(
    typeName: String,
    handle: Handle,
    destroy: @escaping @Sendable (Handle) -> Void
  ) throws {
    guard !handle.isNull else {
      throw NativeStatusFailure(
        rawStatus: 0,
        diagnostic: "\(typeName) native result is the null handle"
      )
    }
    self.typeName = typeName
    self.handle = handle
    self.destroy = destroy
  }

  deinit {
    close()
  }

  func requireLive() throws -> Handle {
    try lock.withLock {
      guard let handle else {
        throw NativeStatusFailure(
          rawStatus: 0,
          diagnostic: "\(typeName) is closed"
        )
      }
      return handle
    }
  }

  func close() {
    let livePointer = lock.withLock {
      let livePointer = handle
      handle = nil
      return livePointer
    }
    if let livePointer {
      destroy(livePointer)
    }
  }
}
