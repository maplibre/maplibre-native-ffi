import Foundation

/// A failure that the binding contained because no caller could receive it.
enum MaplibreDiagnostic {
  /// A handle that its last owner released without closing it. The binding
  /// disposed it unless `detail` reports that the disposal failed, in which
  /// case the handle stays live.
  case leakedHandle(typeName: String, handle: UInt64, detail: String)
  /// An error that the binding met while decoding callback arguments.
  /// `callback` names the C callback type, such as
  /// `mln_resource_provider_callback`. Native received the callback's failure
  /// value instead.
  case callbackError(callback: String, error: any Error)
}

enum NativeDiagnostics {
  static func report(_ diagnostic: MaplibreDiagnostic) {
    writeStandardError(describe(diagnostic))
  }

  private static func describe(_ diagnostic: MaplibreDiagnostic) -> String {
    switch diagnostic {
    case let .leakedHandle(typeName, handle, detail):
      let subject = "native handle 0x\(String(handle, radix: 16))"
        + (detail.isEmpty ? "" : " (\(detail))")
      return "Leaked \(typeName) \(subject); close handles explicitly.\n"
    case let .callbackError(callback, error):
      return "\(callback) failed and native received its fallback: \(error)\n"
    }
  }

  private static func writeStandardError(_ message: String) {
    if let data = message.data(using: .utf8) {
      FileHandle.standardError.write(data)
    }
  }
}
