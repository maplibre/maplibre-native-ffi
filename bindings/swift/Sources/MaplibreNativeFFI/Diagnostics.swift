import Foundation

/// Sees each report before it is written to standard error. Tests read reports
/// here instead of redirecting the process's standard error.
let reportObserver = ReportObserver()

final class ReportObserver: @unchecked Sendable {
  private let lock = NSLock()
  private var handler: (@Sendable (String) -> Void)?

  /// Replaces the handler, or removes it when `handler` is nil.
  func set(_ handler: (@Sendable (String) -> Void)?) {
    lock.withLock { self.handler = handler }
  }

  fileprivate func observe(_ message: String) {
    lock.withLock { handler }?(message)
  }
}

func reportToStandardError(_ message: String) {
  reportObserver.observe(message)
  if let data = message.data(using: .utf8) {
    FileHandle.standardError.write(data)
  }
}
