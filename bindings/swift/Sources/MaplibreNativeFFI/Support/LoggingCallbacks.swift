internal import CMaplibreNativeC
import Foundation

struct NativeLogRecord: Equatable {
  let severity: UInt32
  let event: UInt32
  let code: Int64
  let message: String
}

private final class LogCallbackBox: @unchecked Sendable {
  let callback: @Sendable (NativeLogRecord) -> Bool

  init(_ callback: @escaping @Sendable (NativeLogRecord) -> Bool) {
    self.callback = callback
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

private func releaseLogCallback(_ userData: UnsafeMutableRawPointer?) {
  guard let userData else { return }
  Unmanaged<LogCallbackBox>.fromOpaque(userData).release()
}

enum LoggingCallbackState {
  static func set(_ callback: @escaping @Sendable (NativeLogRecord)
    -> Bool) throws
  {
    let replacement = Unmanaged.passRetained(LogCallbackBox(callback))
    do {
      try checkStatus(mln_log_set_callback(
        logCallbackTrampoline,
        replacement.toOpaque(),
        releaseLogCallback
      ))
    } catch {
      replacement.release()
      throw error
    }
  }

  static func clear() throws {
    try checkStatus(mln_log_clear_callback())
  }

  static func invokeForTesting(
    _ callback: @escaping @Sendable (NativeLogRecord) -> Bool,
    record: NativeLogRecord
  ) -> Bool {
    let box = Unmanaged.passRetained(LogCallbackBox(callback))
    defer { box.release() }
    return record.message.withCString { message in
      logCallbackTrampoline(
        userData: box.toOpaque(),
        severity: record.severity,
        event: record.event,
        code: record.code,
        message: message
      ) != 0
    }
  }
}
