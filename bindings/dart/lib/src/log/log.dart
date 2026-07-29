/// Log configuration and copied native log records.
library;

/// Log severity emitted by MapLibre Native.
final class LogSeverity {
  const LogSeverity._(this.rawValue, this.name);

  /// Informational log record.
  static const info = LogSeverity._(1, 'info');

  /// Warning log record.
  static const warning = LogSeverity._(2, 'warning');

  /// Error log record.
  static const error = LogSeverity._(3, 'error');

  /// Creates a log severity from a raw native value.
  factory LogSeverity.fromRawValue(int rawValue) => switch (rawValue) {
    1 => info,
    2 => warning,
    3 => error,
    _ => LogSeverity._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Log event category emitted by MapLibre Native.
final class LogEvent {
  const LogEvent._(this.rawValue, this.name);

  /// General category.
  static const general = LogEvent._(0, 'general');

  /// Setup category.
  static const setup = LogEvent._(1, 'setup');

  /// Shader category.
  static const shader = LogEvent._(2, 'shader');

  /// Parse-style category.
  static const parseStyle = LogEvent._(3, 'parseStyle');

  /// Parse-tile category.
  static const parseTile = LogEvent._(4, 'parseTile');

  /// Render category.
  static const render = LogEvent._(5, 'render');

  /// Style category.
  static const style = LogEvent._(6, 'style');

  /// Database category.
  static const database = LogEvent._(7, 'database');

  /// HTTP request category.
  static const httpRequest = LogEvent._(8, 'httpRequest');

  /// Sprite category.
  static const sprite = LogEvent._(9, 'sprite');

  /// Image category.
  static const image = LogEvent._(10, 'image');

  /// OpenGL category.
  static const openGl = LogEvent._(11, 'openGl');

  /// JNI category.
  static const jni = LogEvent._(12, 'jni');

  /// Android category.
  static const android = LogEvent._(13, 'android');

  /// Crash category.
  static const crash = LogEvent._(14, 'crash');

  /// Glyph category.
  static const glyph = LogEvent._(15, 'glyph');

  /// Timing category.
  static const timing = LogEvent._(16, 'timing');

  /// Creates a log event from a raw native value.
  factory LogEvent.fromRawValue(int rawValue) => switch (rawValue) {
    0 => general,
    1 => setup,
    2 => shader,
    3 => parseStyle,
    4 => parseTile,
    5 => render,
    6 => style,
    7 => database,
    8 => httpRequest,
    9 => sprite,
    10 => image,
    11 => openGl,
    12 => jni,
    13 => android,
    14 => crash,
    15 => glyph,
    16 => timing,
    _ => LogEvent._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Copied native log record.
final class LogRecord {
  /// Creates a copied native log record.
  const LogRecord({
    required this.severity,
    required this.event,
    required this.code,
    required this.message,
  });

  /// Record severity.
  final LogSeverity severity;

  /// Event category.
  final LogEvent event;

  /// Native log code.
  final int code;

  /// Copied message.
  final String message;
}

/// Log callback run asynchronously on Dart's owner isolate.
typedef LogCallback = void Function(LogRecord record);

/// Log severity mask for asynchronous native log dispatch.
final class LogSeverityMask {
  /// Creates a log severity mask from raw C bits.
  const LogSeverityMask(this.bits);

  /// Info severity bit.
  static const info = LogSeverityMask(1 << 1);

  /// Warning severity bit.
  static const warning = LogSeverityMask(1 << 2);

  /// Error severity bit.
  static const error = LogSeverityMask(1 << 3);

  /// Native default mask: info and warning may be asynchronous.
  static const defaultMask = LogSeverityMask((1 << 1) | (1 << 2));

  /// All known severity bits.
  static const all = LogSeverityMask((1 << 1) | (1 << 2) | (1 << 3));

  /// Raw native mask bits.
  final int bits;

  /// Returns true when all [severity] bits are present in this mask.
  bool contains(LogSeverityMask severity) =>
      (bits & severity.bits) == severity.bits;
}
