import Foundation

private final class NativeCallbackPolicy {
  let owner: AnyObject?
  let operations: Set<String>
  init(owner: AnyObject?, operations: Set<String>) {
    self.owner = owner
    self.operations = operations
  }
}

enum NativeCallbackGuard {
  private static let key = "MaplibreNativeFFI.callbackPolicy"

  static func enter(owner: AnyObject?, operations: Set<String>) -> Scope {
    let previous = Thread.current.threadDictionary[key]
    let enclosing = previous as? NativeCallbackPolicy
    let allowed = enclosing
      .map { $0.owner === owner ? $0.operations.intersection(operations) : []
      } ??
      operations
    Thread.current.threadDictionary[key] = NativeCallbackPolicy(
      owner: owner,
      operations: allowed
    )
    return Scope(previous: previous)
  }

  static var isActive: Bool {
    Thread.current.threadDictionary[key] != nil
  }

  static func check(owner: AnyObject?, operation: String) throws {
    guard let policy = Thread.current
      .threadDictionary[key] as? NativeCallbackPolicy else { return }
    guard owner === policy.owner, policy.operations.contains(operation) else {
      throw MaplibreError(kind: .invalidState, rawStatus: nil,
                          diagnostic: "Native operation is forbidden inside this callback")
    }
  }

  struct Scope {
    fileprivate let previous: Any?
    func end() {
      if let previous { Thread.current.threadDictionary[key] = previous }
      else { Thread.current.threadDictionary.removeObject(forKey: key) }
    }
  }
}
