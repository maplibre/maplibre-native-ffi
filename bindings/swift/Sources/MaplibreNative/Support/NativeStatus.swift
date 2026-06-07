internal import CMaplibreNativeC
import Foundation

struct NativeStatusFailure: Error, Equatable, Sendable {
  let rawStatus: Int32
  let diagnostic: String
  let isNativeStatus: Bool

  init(rawStatus: Int32, diagnostic: String, isNativeStatus: Bool = true) {
    self.rawStatus = rawStatus
    self.diagnostic = diagnostic
    self.isNativeStatus = isNativeStatus
  }

  static func swiftInvalidArgument(_ diagnostic: String) -> Self {
    Self(rawStatus: MLN_STATUS_INVALID_ARGUMENT.rawValue, diagnostic: diagnostic, isNativeStatus: false)
  }

  static func swiftNativeError(_ diagnostic: String) -> Self {
    Self(rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue, diagnostic: diagnostic, isNativeStatus: false)
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
