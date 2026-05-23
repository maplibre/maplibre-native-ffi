internal import CMaplibreNativeC
import Foundation

struct NativeStatusFailure: Error, Equatable, Sendable {
  let rawStatus: Int32
  let diagnostic: String

  init(rawStatus: Int32, diagnostic: String) {
    self.rawStatus = rawStatus
    self.diagnostic = diagnostic
  }
}

func captureThreadDiagnostic() -> String {
  guard let message = mln_thread_last_error_message() else { return "" }
  return String(cString: message)
}

func checkStatus(_ status: mln_status) throws {
  if status == MLN_STATUS_OK { return }
  throw NativeStatusFailure(rawStatus: status.rawValue, diagnostic: captureThreadDiagnostic())
}
