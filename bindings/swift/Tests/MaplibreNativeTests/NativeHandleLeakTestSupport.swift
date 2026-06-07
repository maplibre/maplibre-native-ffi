import Foundation

@testable import MaplibreNative

enum NativeHandleLeakTestSupport {
  private static let lock = NSLock()

  static func withHandler(
    _ handler: @escaping @Sendable (NativeHandleLeak) -> Void,
    run body: () throws -> Void
  ) rethrows {
    try lock.withLock {
      NativeHandleLeakReporter.setHandler(handler)
      defer { NativeHandleLeakReporter.resetHandler() }
      try body()
    }
  }
}
