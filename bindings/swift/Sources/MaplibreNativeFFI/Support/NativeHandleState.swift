import Foundation

final class NativeHandleState<Handle: NativeHandle>: @unchecked Sendable {
  private let typeName: String
  private let lock = NSLock()
  private var closed = false
  private let parent: AnyObject?
  let value: Handle

  init(
    typeName: String,
    handle: Handle,
    parent: AnyObject? = nil
  ) throws {
    guard !handle.isNull else {
      throw MaplibreError
        .invalidArgument("\(typeName) native handle is the null handle")
    }
    self.typeName = typeName
    self.parent = parent
    value = handle
  }

  deinit {
    if !closed {
      NativeRetirement(
        handle: value,
        parent: parent,
        typeName: typeName
      ).schedule()
    }
  }

  var isClosed: Bool {
    lock.withLock { closed }
  }

  func withHandle<T>(_ body: (Handle) throws -> T) throws -> T {
    try body(value)
  }

  var description: String {
    let closed = lock.withLock { self.closed }
    return "\(typeName)(id=0x\(String(value.raw, radix: 16)), closed=\(closed))"
  }

  /// Closes a handle whose `destroy` can't fail.
  func closeInfallibly(_ destroy: (Handle) -> Void) {
    try! closeOnce(destroy)
  }

  func closeOnce(
    idempotent: Bool = true,
    _ destroy: (Handle) throws -> Void
  ) throws {
    let shouldClose = lock.withLock { !(idempotent && closed) }
    guard shouldClose else { return }
    defer { withExtendedLifetime(parent) {} }
    try destroy(value)
    lock.withLock { closed = true }
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
      self.dispose()
    }
  }

  private func dispose() {
    defer { withExtendedLifetime(parent) {} }
    let disposed = handle.disposeAbandoned()
    let detail = disposed ? "" : " (native disposal failed)"
    reportToStandardError(
      "Leaked \(typeName) native handle 0x\(String(handle.raw, radix: 16))\(detail); close handles explicitly.\n"
    )
  }
}
