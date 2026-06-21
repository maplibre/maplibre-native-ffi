
internal import CMaplibreNativeC

public enum LogSeverity: Sendable, Hashable {
  case info
  case warning
  case error
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 1: .info
    case 2: .warning
    case 3: .error
    default: .unknown(rawValue)
    }
  }
}

public struct LogSeverityMask: OptionSet, Sendable, Hashable {
  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }

  public static let info = Self(rawValue: 1 << 1)
  public static let warning = Self(rawValue: 1 << 2)
  public static let error = Self(rawValue: 1 << 3)
  public static let `default`: Self = [.info, .warning]
  public static let all: Self = [.info, .warning, .error]
}

public enum LogEvent: Sendable, Hashable {
  case general
  case setup
  case shader
  case parseStyle
  case parseTile
  case render
  case style
  case database
  case httpRequest
  case sprite
  case image
  case openGL
  case jni
  case android
  case crash
  case glyph
  case timing
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .general
    case 1: .setup
    case 2: .shader
    case 3: .parseStyle
    case 4: .parseTile
    case 5: .render
    case 6: .style
    case 7: .database
    case 8: .httpRequest
    case 9: .sprite
    case 10: .image
    case 11: .openGL
    case 12: .jni
    case 13: .android
    case 14: .crash
    case 15: .glyph
    case 16: .timing
    default: .unknown(rawValue)
    }
  }
}

public struct LogRecord: Sendable, Equatable {
  public let severity: LogSeverity
  public let event: LogEvent
  public let code: Int64
  public let message: String

  public init(
    severity: LogSeverity,
    event: LogEvent,
    code: Int64,
    message: String
  ) {
    self.severity = severity
    self.event = event
    self.code = code
    self.message = message
  }

  init(native: NativeLogRecord) {
    self.init(
      severity: LogSeverity.fromNative(native.severity),
      event: LogEvent.fromNative(native.event),
      code: native.code,
      message: native.message
    )
  }
}

public typealias LogCallback = @Sendable (LogRecord) -> Bool

public extension Maplibre {
  static func setLogCallback(_ callback: @escaping LogCallback) throws {
    try mapNativeFailure {
      try LoggingCallbackState.set { nativeRecord in
        callback(LogRecord(native: nativeRecord))
      }
    }
  }

  static func clearLogCallback() throws {
    try mapNativeFailure {
      try LoggingCallbackState.clear()
    }
  }

  static func setAsyncLogSeverityMask(_ mask: LogSeverityMask) throws {
    let unknownBits = mask.rawValue & ~LogSeverityMask.all.rawValue
    if unknownBits != 0 {
      throw MaplibreError.invalidArgument(
        "unknown log severity mask bits 0x\(String(unknownBits, radix: 16)) cannot be set"
      )
    }
    try mapNativeFailure {
      try checkStatus(mln_log_set_async_severity_mask(mask.rawValue))
    }
  }

  static func restoreDefaultAsyncLogSeverityMask() throws {
    try setAsyncLogSeverityMask(.default)
  }
}
