import Foundation

final class NativeHandleState<Handle: NativeHandle>: @unchecked Sendable {
  private let typeName: String
  private let lock = NSLock()
  private var closed = false
  private let parent: AnyObject?
  let value: Handle
  private let protectsCopies: Bool
  private var readers = 0
  private var releasing = false

  init(
    typeName: String,
    handle: Handle,
    parent: AnyObject? = nil,
    protectsCopies: Bool = false
  ) throws {
    guard !handle.isNull else {
      throw MaplibreError
        .invalidArgument("\(typeName) native handle is the null handle")
    }
    self.typeName = typeName
    self.parent = parent
    value = handle
    self.protectsCopies = protectsCopies
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

  func retire() {
    guard !lock.withLock({ closed }) else { return }
    if NativeRetirement(handle: value, parent: parent, typeName: typeName)
      .dispose(reportLeak: false)
    {
      lock.withLock { closed = true }
    }
  }

  var isClosed: Bool {
    lock.withLock { closed }
  }

  func handOff() {
    lock.withLock { closed = true }
  }

  /// Batch getters lend memory beyond the native call, until the copy ends.
  func withHandle<T>(_ body: (Handle) throws -> T) throws -> T {
    if protectsCopies {
      try lock.withLock {
        guard !releasing
        else { throw MaplibreError.invalidState("Batch is in use") }
        readers += 1
      }
    }
    defer {
      if protectsCopies {
        lock.withLock { readers -= 1 }
      }
    }
    return try body(value)
  }

  var description: String {
    let closed = lock.withLock { self.closed }
    return "\(typeName)(id=0x\(String(value.raw, radix: 16)), closed=\(closed))"
  }

  /// Closes a handle that guards no copies, so only `destroy` could fail.
  func closeUnguarded(_ destroy: (Handle) -> Void) {
    precondition(!protectsCopies, "\(typeName) guards copies")
    try! closeOnce(destroy)
  }

  func closeOnce(
    idempotent: Bool = true,
    _ destroy: (Handle) throws -> Void
  ) throws {
    let shouldClose = try lock.withLock {
      if protectsCopies, readers != 0 || releasing {
        throw MaplibreError.invalidState("Batch is in use")
      }
      if idempotent, closed {
        return false
      }
      releasing = protectsCopies
      return true
    }
    guard shouldClose else { return }
    defer {
      lock.withLock { releasing = false }
      withExtendedLifetime(parent) {}
    }
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
      self.dispose(reportLeak: true)
    }
  }

  @discardableResult
  func dispose(reportLeak: Bool) -> Bool {
    defer { withExtendedLifetime(parent) {} }
    let disposed = handle.disposeAbandoned()
    if reportLeak || !disposed {
      let detail = disposed ? "" : " (native disposal failed)"
      reportToStandardError(
        "Leaked \(typeName) native handle 0x\(String(handle.raw, radix: 16))\(detail); close handles explicitly.\n"
      )
    }
    return disposed
  }
}
