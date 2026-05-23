import CMaplibreNativeC
import Foundation

public struct NativeLogRecord: Equatable, Sendable {
  public let severity: UInt32
  public let event: UInt32
  public let code: Int64
  public let message: String

  public init(severity: UInt32, event: UInt32, code: Int64, message: String) {
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

public enum LoggingCallbackState {
  private static let lock = NSLock()
  private nonisolated(unsafe) static var retainedBox: Unmanaged<LogCallbackBox>?
  private nonisolated(unsafe) static var boxDeinitHandlerForTesting: (@Sendable () -> Void)?

  public static func set(_ callback: @escaping @Sendable (NativeLogRecord) -> Bool) throws {
    let replacement = Unmanaged.passRetained(LogCallbackBox(callback))
    do {
      try CAPI.setLogCallback(logCallbackTrampoline, userData: replacement.toOpaque())
    } catch {
      replacement.release()
      throw error
    }

    let previous = lock.withLock {
      let previous = retainedBox
      retainedBox = replacement
      return previous
    }
    previous?.release()
  }

  public static func clear() throws {
    try CAPI.clearLogCallback()
    let previous = lock.withLock {
      let previous = retainedBox
      retainedBox = nil
      return previous
    }
    previous?.release()
  }

  public static func invokeForTesting(_ record: NativeLogRecord) -> Bool? {
    let box = lock.withLock { retainedBox?.takeUnretainedValue() }
    return box?.invoke(record)
  }

  public static func setBoxDeinitHandlerForTesting(_ handler: (@Sendable () -> Void)?) {
    lock.withLock {
      boxDeinitHandlerForTesting = handler
    }
  }

  fileprivate static func reportBoxDeinitForTesting() {
    let handler = lock.withLock { boxDeinitHandlerForTesting }
    handler?()
  }
}
