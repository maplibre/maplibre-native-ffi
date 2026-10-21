import Foundation

final class NativeHandleState<Handle: NativeHandle>: @unchecked Sendable {
  private let typeName: String
  private let lock = NSLock()
  private var closed = false
  private var closing = false
  private let parent: AnyObject?
  let issued: Handle

  init(typeName: String, handle: Handle, parent: AnyObject? = nil) throws {
    guard !handle.isNull else {
      throw MaplibreError
        .invalidArgument("\(typeName) native handle is the null handle")
    }
    self.typeName = typeName
    self.parent = parent
    issued = handle
  }

  deinit {
    if !closed {
      NativeRetirement(
        handle: issued,
        parent: parent,
        typeName: typeName
      ).schedule()
    }
  }

  func retire() {
    let retire = lock.withLock {
      guard !closed, !closing else { return false }
      closed = true
      return true
    }
    if retire {
      disposeUnreceived()
    }
  }

  func handOff() {
    lock.withLock { closed = true }
  }

  func closeOnce(_ destroy: (Handle) throws -> Void) throws {
    let release = try lock.withLock {
      guard !closed else { return false }
      guard !closing
      else { throw MaplibreError.invalidState("\(typeName) is closing") }
      closing = true
      return true
    }
    guard release else { return }
    do {
      try destroy(issued)
      lock.withLock { closed = true; closing = false }
    } catch {
      lock.withLock { closing = false }
      throw error
    }
    withExtendedLifetime(parent) {}
  }

  private func disposeUnreceived() {
    defer { withExtendedLifetime(parent) {} }
    let disposed = issued.disposeAbandoned()
    if !disposed {
      NativeDiagnostics.report(.leakedHandle(
        typeName: typeName,
        handle: issued.raw,
        detail: "native disposal failed"
      ))
    }
  }
}

private final class NativeRetirement<Handle: NativeHandle>: @unchecked Sendable {
  let handle: Handle
  let parent: AnyObject?
  let typeName: String

  init(handle: Handle, parent: AnyObject?, typeName: String) {
    self.handle = handle
    self.parent = parent
    self.typeName = typeName
  }

  /// A leaked owner's last reference can disappear inside an unsafe callback.
  func schedule() {
    DispatchQueue.global().async {
      defer { withExtendedLifetime(self.parent) {} }
      let disposed = self.handle.disposeAbandoned()
      NativeDiagnostics.report(.leakedHandle(
        typeName: self.typeName,
        handle: self.handle.raw,
        detail: disposed ? "" : "native disposal failed"
      ))
    }
  }
}
