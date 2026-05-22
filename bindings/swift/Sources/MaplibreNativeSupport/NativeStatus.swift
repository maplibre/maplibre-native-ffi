import CMaplibreNativeC
import Foundation

public struct NativeStatusFailure: Error, Equatable, Sendable {
  public let rawStatus: Int32
  public let diagnostic: String

  public init(rawStatus: Int32, diagnostic: String) {
    self.rawStatus = rawStatus
    self.diagnostic = diagnostic
  }
}

public func captureThreadDiagnostic() -> String {
  guard let message = mln_thread_last_error_message() else { return "" }
  return String(cString: message)
}

public func checkStatus(_ status: mln_status) throws {
  if status == MLN_STATUS_OK { return }
  throw NativeStatusFailure(rawStatus: status.rawValue, diagnostic: captureThreadDiagnostic())
}
