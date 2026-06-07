import Foundation
import Darwin

struct NativeHandleLeak: Equatable, Sendable {
  let typeName: String
  let address: UInt

  init(typeName: String, address: UInt) {
    self.typeName = typeName
    self.address = address
  }
}

enum NativeHandleLeakReporter {
  private static let lock = NSLock()
  private static let defaultHandler: @Sendable (NativeHandleLeak) -> Void = { leak in
    let message = "Leaked \(leak.typeName) native handle 0x\(String(leak.address, radix: 16)); close handles explicitly on their owner thread.\n"
    message.withCString { message in
      _ = Darwin.write(STDERR_FILENO, message, strlen(message))
    }
  }
  private nonisolated(unsafe) static var handler = defaultHandler

  static func report(_ leak: NativeHandleLeak) {
    let current = lock.withLock { handler }
    current(leak)
  }

  static func setHandler(_ replacement: @escaping @Sendable (NativeHandleLeak) -> Void) {
    lock.withLock {
      handler = replacement
    }
  }

  static func resetHandler() {
    setHandler(defaultHandler)
  }
}
