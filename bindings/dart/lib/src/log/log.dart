/// Log configuration and copied native log records.
library;

import '../generated_values.dart';
export '../generated_values.dart';

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

/// Log callback run asynchronously on its receiver isolate.
typedef LogCallback = void Function(LogRecord record);
