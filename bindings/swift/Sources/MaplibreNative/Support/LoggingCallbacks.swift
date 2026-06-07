internal import CMaplibreNativeC
import Foundation

struct NativeLogRecord: Equatable, Sendable {
  let severity: UInt32
  let event: UInt32
  let code: Int64
  let message: String

  init(severity: UInt32, event: UInt32, code: Int64, message: String) {
    self.severity = severity
    self.event = event
    self.code = code
    self.message = message
  }
}

private final class LogCallbackBox: @unchecked Sendable {
  let callback: @Sendable (NativeLogRecord) -> Bool

  init(_ callback: @escaping @Sendable (NativeLogRecord) -> Bool) {
    self.callback = callback
  }

  deinit {
    LoggingCallbackState.reportBoxDeinitForTesting()
  }

  func invoke(_ record: NativeLogRecord) -> Bool {
    callback(record)
  }
}

private func logCallbackTrampoline(
  userData: UnsafeMutableRawPointer?,
  severity: UInt32,
  event: UInt32,
  code: Int64,
  message: UnsafePointer<CChar>?
) -> UInt32 {
  guard let userData else { return 0 }
  let box = Unmanaged<LogCallbackBox>.fromOpaque(userData).takeUnretainedValue()
  let record = NativeLogRecord(
    severity: severity,
    event: event,
    code: code,
    message: message.map { String(cString: $0) } ?? ""
  )
  return box.invoke(record) ? 1 : 0
}

enum LoggingCallbackState {
  private static let lock = NSLock()
  private nonisolated(unsafe) static var retainedBox: Unmanaged<LogCallbackBox>?
  private nonisolated(unsafe) static var boxDeinitHandlerForTesting: (@Sendable () -> Void)?

  static func set(_ callback: @escaping @Sendable (NativeLogRecord) -> Bool) throws {
    let replacement = Unmanaged.passRetained(LogCallbackBox(callback))
    let previous: Unmanaged<LogCallbackBox>?
    do {
      previous = try lock.withLock {
        try checkStatus(mln_log_set_callback(logCallbackTrampoline, replacement.toOpaque()))
        let previous = retainedBox
        retainedBox = replacement
        return previous
      }
    } catch {
      replacement.release()
      throw error
    }
    previous?.release()
  }

  static func clear() throws {
    let previous = try lock.withLock {
      try checkStatus(mln_log_clear_callback())
      let previous = retainedBox
      retainedBox = nil
      return previous
    }
    previous?.release()
  }

  static func invokeForTesting(_ record: NativeLogRecord) -> Bool? {
    let box = lock.withLock { retainedBox?.takeUnretainedValue() }
    return box?.invoke(record)
  }

  static func setBoxDeinitHandlerForTesting(_ handler: (@Sendable () -> Void)?) {
    lock.withLock {
      boxDeinitHandlerForTesting = handler
    }
  }

  fileprivate static func reportBoxDeinitForTesting() {
    let handler = lock.withLock { boxDeinitHandlerForTesting }
    handler?()
  }
}
