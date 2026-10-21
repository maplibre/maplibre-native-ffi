import '../values.dart';

/// A native status and the diagnostic copied when its call returns.
class MaplibreException implements Exception {
  const MaplibreException(this.status, this.diagnostic);

  final Status status;
  final String diagnostic;

  @override
  String toString() => '$status: $diagnostic';
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
