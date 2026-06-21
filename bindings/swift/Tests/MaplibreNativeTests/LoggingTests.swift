import Foundation
@testable import MaplibreNative
import Testing

private final class LogRecords: @unchecked Sendable {
  private let lock = NSLock()
  private var records: [LogRecord] = []

  func append(_ record: LogRecord) {
    lock.withLock {
      records.append(record)
    }
  }

  func snapshot() -> [LogRecord] {
    lock.withLock { records }
  }
}

@Test func logCallbackInstallReplaceAndClear() throws {
  let first = LogRecords()
  let second = LogRecords()
  defer {
    try? Maplibre.clearLogCallback()
  }

  try Maplibre.setLogCallback { record in
    first.append(record)
    return true
  }
  #expect(
    LoggingCallbackState.invokeForTesting(
      NativeLogRecord(severity: 1, event: 0, code: 7, message: "first")
    ) == true
  )

  try Maplibre.setLogCallback { record in
    second.append(record)
    return false
  }
  #expect(
    LoggingCallbackState.invokeForTesting(
      NativeLogRecord(severity: 2, event: 3, code: 8, message: "second")
    ) == false
  )

  try Maplibre.clearLogCallback()
  #expect(
    LoggingCallbackState.invokeForTesting(
      NativeLogRecord(severity: 3, event: 4, code: 9, message: "cleared")
    ) == nil
  )

  #expect(first.snapshot() == [LogRecord(
    severity: .info,
    event: .general,
    code: 7,
    message: "first"
  )])
  #expect(second.snapshot() == [LogRecord(
    severity: .warning,
    event: .parseStyle,
    code: 8,
    message: "second"
  )])
}

@Test func asyncLogSeverityMaskRejectsUnknownBitsBeforeCallingC() throws {
  do {
    try Maplibre.setAsyncLogSeverityMask(LogSeverityMask(rawValue: 1 << 30))
    Issue.record("unknown mask bits should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func asyncLogSeverityMaskAcceptsKnownBits() throws {
  try Maplibre.setAsyncLogSeverityMask([.info, .error])
  try Maplibre.restoreDefaultAsyncLogSeverityMask()
}
