import Foundation

final class NativeOwnedCallback<Value> {
  weak var owner: AnyObject?
  let value: Value
  var token: UInt = 0
  init(owner: AnyObject, value: Value) {
    self.owner = owner; self.value = value
  }

  deinit { NativeCallbackRegistry.shared.remove(token) }
}

final class NativeCallbackRegistry: @unchecked Sendable {
  static let shared = NativeCallbackRegistry()
  private final class Entry {
    weak var value: AnyObject?
    init(_ value: AnyObject) {
      self.value = value
    }
  }

  private let lock = NSLock()
  private var next: UInt = 1
  private var entries: [UInt: Entry] = [:]
  func insert<Value>(_ value: NativeOwnedCallback<Value>)
    -> UnsafeMutableRawPointer
  {
    lock.withLock {
      precondition(next != UInt.max, "callback token space exhausted")
      let token = next
      next += 1
      value.token = token
      entries[token] = Entry(value)
      return UnsafeMutableRawPointer(bitPattern: token)!
    }
  }

  func resolve(_ token: UnsafeMutableRawPointer?) -> AnyObject? {
    lock.withLock { entries[UInt(bitPattern: token)]?.value }
  }

  func remove(_ token: UInt) {
    _ = lock.withLock { entries.removeValue(forKey: token) }
  }
}
