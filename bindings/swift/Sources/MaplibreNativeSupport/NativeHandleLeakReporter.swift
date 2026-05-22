import Foundation

public struct NativeHandleLeak: Equatable, Sendable {
  public let typeName: String
  public let address: UInt

  public init(typeName: String, address: UInt) {
    self.typeName = typeName
    self.address = address
  }
}

public enum NativeHandleLeakReporter {
  private static let lock = NSLock()
  private nonisolated(unsafe) static var handler: @Sendable (NativeHandleLeak) -> Void = { leak in
    fputs(
      "Leaked \(leak.typeName) native handle 0x\(String(leak.address, radix: 16)); close handles explicitly on their owner thread.\n",
      stderr
    )
  }

  public static func report(_ leak: NativeHandleLeak) {
    let current = lock.withLock { handler }
    current(leak)
  }

  public static func setHandler(_ replacement: @escaping @Sendable (NativeHandleLeak) -> Void) {
    lock.withLock {
      handler = replacement
    }
  }

  public static func resetHandler() {
    setHandler { leak in
      fputs(
        "Leaked \(leak.typeName) native handle 0x\(String(leak.address, radix: 16)); close handles explicitly on their owner thread.\n",
        stderr
      )
    }
  }
}
