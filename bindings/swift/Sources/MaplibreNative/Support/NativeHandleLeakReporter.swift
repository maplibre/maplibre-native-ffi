import Foundation

struct NativeHandleLeak: Equatable {
  let typeName: String
  let address: UInt
}

private func writeStandardError(_ message: String) {
  if let data = message.data(using: .utf8) {
    FileHandle.standardError.write(data)
  }
}

enum NativeHandleLeakReporter {
  private static let lock = NSLock()
  private static let defaultHandler: @Sendable (NativeHandleLeak)
    -> Void = { leak in
      let message = "Leaked \(leak.typeName) native handle 0x\(String(leak.address, radix: 16)); close handles explicitly on their owner thread.\n"
      writeStandardError(message)
    }

  private nonisolated(unsafe) static var handler = defaultHandler

  static func report(_ leak: NativeHandleLeak) {
    let current = lock.withLock { handler }
    current(leak)
  }

  static func setHandler(_ replacement: @escaping @Sendable (NativeHandleLeak)
    -> Void)
  {
    lock.withLock {
      handler = replacement
    }
  }

  static func resetHandler() {
    setHandler(defaultHandler)
  }
}
