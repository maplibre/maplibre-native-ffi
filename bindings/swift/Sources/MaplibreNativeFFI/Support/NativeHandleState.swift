import Foundation

final class NativeHandleState<Handle: NativeHandle>: @unchecked Sendable {
  private let typeName: String
  private let lock = NSLock()
  private var closed = false
  private let parent: AnyObject?
  /// Whether a callback borrows the handle, which the binding then never
  /// releases or disposes.
  private let borrowed: Bool
  let value: Handle

  init(
    typeName: String,
    handle: Handle,
    parent: AnyObject? = nil,
    borrowed: Bool = false
  ) throws {
    guard !handle.isNull else {
      throw MaplibreError
        .invalidArgument("\(typeName) native handle is the null handle")
    }
    self.typeName = typeName
    self.parent = parent
    self.borrowed = borrowed
    value = handle
  }

  deinit {
    if !closed, !borrowed {
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

  /// Closes a handle whose `destroy` can't fail. A borrowed handle stays
  /// open.
  func closeInfallibly(_ destroy: (Handle) -> Void) {
    guard !borrowed else { return }
    try! closeOnce(destroy)
  }

  func closeOnce(
    idempotent: Bool = true,
    _ destroy: (Handle) throws -> Void
  ) throws {
    guard !borrowed else {
      throw MaplibreError.invalidState(
        "\(typeName) is borrowed by a callback, which can't release it"
      )
    }
    let shouldClose = lock.withLock { !(idempotent && closed) }
    guard shouldClose else { return }
    defer { withExtendedLifetime(parent) {} }
    try destroy(value)
    lock.withLock { closed = true }
  }

  /// Runs an answer, which ends the handle whatever it returns. Native ends a
  /// live handle on every outcome, and a failure releases the handle in case
  /// the failure came before native saw it. A repeated answer reaches native,
  /// which reports the handle stale.
  func answer(_ call: (Handle) throws -> Void) throws {
    guard !borrowed else {
      throw MaplibreError.invalidState(
        "\(typeName) is borrowed by a callback, which can't release it"
      )
    }
    defer { withExtendedLifetime(parent) {} }
    do {
      try call(value)
    } catch {
      _ = value.disposeAbandoned()
      lock.withLock { closed = true }
      throw error
    }
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
