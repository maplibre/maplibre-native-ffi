internal import CMaplibreNativeC
import Foundation

struct NativeStatusFailure: Error, Equatable {
  let rawStatus: Int32
  let diagnostic: String
  let isNativeStatus: Bool

  init(rawStatus: Int32, diagnostic: String, isNativeStatus: Bool = true) {
    self.rawStatus = rawStatus
    self.diagnostic = diagnostic
    self.isNativeStatus = isNativeStatus
  }

  static func swiftNativeError(_ diagnostic: String) -> Self {
    Self(
      rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue,
      diagnostic: diagnostic,
      isNativeStatus: false
    )
  }
}

/// Calls a status-returning native function with a fresh diagnostic and throws
/// its failure.
///
/// The diagnostic stays uninitialized apart from `size` and the first message
/// byte, so a call skips zeroing the whole message buffer; the message is read
/// only when the call fails.
func checkStatus(
  _ call: (UnsafeMutablePointer<mln_diagnostic>) throws -> mln_status
) throws {
  try withUnsafeTemporaryAllocation(
    of: mln_diagnostic.self,
    capacity: 1
  ) { buffer in
    let diagnostic = buffer.baseAddress!
    diagnostic.pointee.size = UInt32(MemoryLayout<mln_diagnostic>.size)
    diagnostic.pointee.message.0 = 0
    let status = try call(diagnostic)
    if status == MLN_STATUS_OK { return }
    throw NativeStatusFailure(
      rawStatus: status.rawValue,
      diagnostic: message(of: diagnostic)
    )
  }
}

private func message(of diagnostic: UnsafeMutablePointer<mln_diagnostic>)
  -> String
{
  withUnsafeBytes(of: &diagnostic.pointee.message) { bytes in
    let text = bytes.prefix { $0 != 0 }
    return String(decoding: text, as: UTF8.self)
  }
}
