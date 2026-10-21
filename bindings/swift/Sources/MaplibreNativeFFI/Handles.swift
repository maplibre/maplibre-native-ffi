import Foundation

/// Shares ownership state between the public handle and its calls.
class NativeHandleBox<Handle: NativeHandle>: @unchecked Sendable {
  private let state: NativeHandleState<Handle>
  private let copyLock: NSRecursiveLock?
  private var copying = false

  init(
    typeName: String,
    handle: Handle,
    parent: AnyObject? = nil,
    protectsCopies: Bool = false
  ) throws {
    state = try NativeHandleState(
      typeName: typeName,
      handle: handle,
      parent: parent
    )
    copyLock = protectsCopies ? NSRecursiveLock() : nil
  }

  var issued: Handle {
    state.issued
  }

  /// Batch getters lend memory beyond the native call, until the copy ends.
  func withHandle<T>(_ body: (Handle) throws -> T) rethrows -> T {
    guard let copyLock else { return try body(issued) }
    return try copyLock.withLock {
      copying = true
      defer { copying = false }
      return try body(issued)
    }
  }

  func closeOnce(_ destroy: (Handle) throws -> Void) throws {
    guard let copyLock else { return try state.closeOnce(destroy) }
    try copyLock.withLock {
      guard !copying
      else { throw MaplibreError.invalidState("Batch is in use") }
      try state.closeOnce(destroy)
    }
  }

  func handOff() {
    state.handOff()
  }

  func retire() {
    state.retire()
  }
}
