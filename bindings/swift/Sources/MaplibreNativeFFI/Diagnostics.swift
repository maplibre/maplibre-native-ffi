import Foundation

/// A failure that the binding contained because no caller could receive it.
public enum MaplibreDiagnostic {
  /// A handle that its owner dropped without closing, and that the binding
  /// could not dispose. The handle stays live. `handle` is zero when the
  /// leaked resource is a texture frame rather than a C API handle, which
  /// `detail` then names.
  case leakedHandle(typeName: String, handle: UInt64, detail: String)
  /// An error that a callback threw, or that the binding met while decoding
  /// the callback's arguments. `callback` names the C callback type, such as
  /// `mln_resource_provider_callback`. Native received the callback's failure
  /// value instead.
  case callbackError(callback: String, error: any Error)
}

public extension Maplibre {
  /// Installs the handler for each `MaplibreDiagnostic`, replacing the
  /// previous one, or restores the default, which writes each diagnostic to
  /// standard error.
  ///
  /// The handler may run on any thread, including a native callback thread
  /// where native calls are refused, so it should return quickly.
  static func setDiagnosticHandler(
    _ handler: (@Sendable (MaplibreDiagnostic) -> Void)?
  ) {
    NativeDiagnostics.setHandler(handler)
  }
}

enum NativeDiagnostics {
  private static let lock = NSLock()
  private nonisolated(unsafe) static var handler: (@Sendable (
    MaplibreDiagnostic
  )
    -> Void)?

  static func report(_ diagnostic: MaplibreDiagnostic) {
    let current = lock.withLock { handler }
    if let current { current(diagnostic) }
    else { writeStandardError(describe(diagnostic)) }
  }

  static func setHandler(_ replacement: (@Sendable (MaplibreDiagnostic)
      -> Void)?)
  {
    lock.withLock { handler = replacement }
  }

  private static func describe(_ diagnostic: MaplibreDiagnostic) -> String {
    switch diagnostic {
    case let .leakedHandle(typeName, handle, detail):
      let subject = handle == 0
        ? detail
        : "native handle 0x\(String(handle, radix: 16))"
        + (detail.isEmpty ? "" : " (\(detail))")
      return "Leaked \(typeName) \(subject); close handles explicitly.\n"
    case let .callbackError(callback, error):
      return "\(callback) threw and native received its fallback: \(error)\n"
    }
  }

  private static func writeStandardError(_ message: String) {
    if let data = message.data(using: .utf8) {
      FileHandle.standardError.write(data)
    }
  }
}
