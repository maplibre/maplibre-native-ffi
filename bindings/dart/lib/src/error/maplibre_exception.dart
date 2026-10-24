import '../values.dart';

/// A native status and the diagnostic that native copied for it.
///
/// A `Future`-returning method reports every native status through its
/// future: a call that native rejects, and a completion that fails. A command
/// fails this way when native reports it failed or cancelled.
class MaplibreException implements Exception {
  const MaplibreException(this.status, this.diagnostic, {this.generation = 0});

  final Status status;
  final String diagnostic;

  /// The map snapshot generation that a failed or cancelled map command
  /// published, or zero.
  final int generation;

  @override
  String toString() => diagnostic.isEmpty ? '$status' : '$status: $diagnostic';
}

/// The native library and binding use different C ABI versions.
final class AbiVersionMismatchException extends MaplibreException {
  const AbiVersionMismatchException(this.expected, this.actual)
    : super(Status.unsupported, 'incompatible MapLibre Native C ABI version');

  final int expected;
  final int actual;

  @override
  String toString() => '$diagnostic: expected $expected, got $actual';
}
