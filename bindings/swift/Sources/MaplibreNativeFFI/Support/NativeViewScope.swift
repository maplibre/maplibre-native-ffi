import Foundation

final class NativeViewScope {
  private let lock = NSLock()
  private let thread = Thread.current
  private var active = true

  func check() throws {
    try lock.withLock {
      guard active, Thread.current === thread else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil,
                            diagnostic: "Native view access requires its active callback thread")
      }
    }
  }

  func expire() {
    lock.withLock { active = false }
  }
}
