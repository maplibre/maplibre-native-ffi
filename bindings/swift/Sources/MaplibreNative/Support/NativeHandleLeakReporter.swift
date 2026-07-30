import Foundation

struct NativeHandleLeak: Equatable {
  let typeName: String
  /// The C API handle id the leak is about, or zero when the leaked resource is
  /// a texture frame rather than a C API handle. Backend-native addresses never
  /// appear here; they belong to ``NativePointer``.
  let handle: UInt64
  /// What the leak names when it is not a C API handle.
  let detail: String

  init(typeName: String, handle: UInt64, detail: String = "") {
    self.typeName = typeName
    self.handle = handle
    self.detail = detail
  }
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
      let subject = leak.handle == 0
        ? leak.detail
        : "native handle 0x\(String(leak.handle, radix: 16))"
      let message = "Leaked \(leak.typeName) \(subject); close handles explicitly on their owner thread.\n"
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
