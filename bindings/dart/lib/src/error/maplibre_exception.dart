/// Stable status categories reported by the MapLibre Native C ABI.
final class MaplibreStatus {
  const MaplibreStatus._(this.name, this.nativeStatusCode);

  /// Native call completed successfully.
  static const ok = MaplibreStatus._('ok', 0);

  /// A pointer, size field, mask, handle, or language value was invalid.
  static const invalidArgument = MaplibreStatus._('invalidArgument', -1);

  /// The object is valid but not currently in a state that permits the call.
  static const invalidState = MaplibreStatus._('invalidState', -2);

  /// The handle is thread-affine and the call ran on the wrong native thread.
  static const wrongThread = MaplibreStatus._('wrongThread', -3);

  /// The requested entry point or behavior is unavailable in this build.
  static const unsupported = MaplibreStatus._('unsupported', -4);

  /// A native MapLibre error or C++ exception was converted to status.
  static const nativeError = MaplibreStatus._('nativeError', -5);

  /// The operation completed after cancellation was requested.
  static const cancelled = MaplibreStatus._('cancelled', -6);

  /// The call raced a driver call in flight; retry once it returns.
  static const busy = MaplibreStatus._('busy', -7);

  /// The render target was lost and the session cannot continue with it.
  static const targetLost = MaplibreStatus._('targetLost', -8);

  /// No result was ready yet; poll or wait for the wake and retry.
  static const notReady = MaplibreStatus._('notReady', -9);

  /// No object has the requested ID.
  static const notFound = MaplibreStatus._('notFound', -10);

  /// An unknown status value returned by a newer or incompatible native build.
  static MaplibreStatus unknown(int nativeStatusCode) =>
      MaplibreStatus._('unknown', nativeStatusCode);

  /// The loaded native library uses an incompatible C ABI version.
  static const abiVersionMismatch = MaplibreStatus._(
    'abiVersionMismatch',
    -1000,
  );

  /// Creates the stable status category for a native status code.
  static MaplibreStatus fromNativeStatusCode(int nativeStatusCode) =>
      switch (nativeStatusCode) {
        0 => ok,
        -1 => invalidArgument,
        -2 => invalidState,
        -3 => wrongThread,
        -4 => unsupported,
        -5 => nativeError,
        -6 => cancelled,
        -7 => busy,
        -8 => targetLost,
        -9 => notReady,
        -10 => notFound,
        _ => unknown(nativeStatusCode),
      };

  /// Human-readable category name.
  final String name;

  /// Raw native status code for this category.
  final int nativeStatusCode;

  @override
  bool operator ==(Object other) =>
      other is MaplibreStatus && other.nativeStatusCode == nativeStatusCode;

  @override
  int get hashCode => nativeStatusCode.hashCode;

  @override
  String toString() => name;
}

/// Base exception for errors reported by the native MapLibre C ABI or binding.
///
/// The hierarchy remains non-exhaustive so compatible binding releases can add
/// specialized exceptions without invalidating consumer switches.
abstract final class MaplibreException implements Exception {
  /// Creates a MapLibre exception.
  const MaplibreException(this.status, this.nativeStatusCode, this.diagnostic);

  /// Creates the stable exception subtype for a native status code.
  factory MaplibreException.forNativeStatusCode(
    int nativeStatusCode,
    String diagnostic,
  ) {
    final status = MaplibreStatus.fromNativeStatusCode(nativeStatusCode);
    return switch (status.nativeStatusCode) {
      -1 => InvalidArgumentException(nativeStatusCode, diagnostic),
      -2 => InvalidStateException(nativeStatusCode, diagnostic),
      -3 => WrongThreadException(nativeStatusCode, diagnostic),
      -4 => UnsupportedFeatureException(nativeStatusCode, diagnostic),
      -5 => NativeErrorException(nativeStatusCode, diagnostic),
      -6 => CancelledException(nativeStatusCode, diagnostic),
      -7 => BusyException(nativeStatusCode, diagnostic),
      -8 => TargetLostException(nativeStatusCode, diagnostic),
      -9 => NotReadyException(nativeStatusCode, diagnostic),
      -10 => NotFoundException(nativeStatusCode, diagnostic),
      _ => UnknownMaplibreException(status, nativeStatusCode, diagnostic),
    };
  }

  /// Creates a binding-side validation error before native code is called.
  factory MaplibreException.invalidArgument(String diagnostic) =>
      InvalidArgumentException(null, diagnostic);

  /// Creates a binding-side invalid-state error before native code is called.
  factory MaplibreException.invalidState(String diagnostic) =>
      InvalidStateException(null, diagnostic);

  /// Creates a binding-side ABI-version mismatch.
  factory MaplibreException.abiVersionMismatch(String diagnostic) =>
      AbiVersionMismatchException(diagnostic);

  /// Stable status category.
  final MaplibreStatus status;

  /// Raw native status code, or null for binding-side validation failures.
  final int? nativeStatusCode;

  /// Diagnostic copied immediately after a failing native call.
  final String diagnostic;

  @override
  String toString() {
    final detail = diagnostic.isEmpty
        ? 'No native diagnostic available.'
        : diagnostic;
    final raw = nativeStatusCode == null ? '' : ' (${nativeStatusCode!})';
    return '${status.name}$raw: $detail';
  }
}

/// Native invalid-argument failure.
final class InvalidArgumentException extends MaplibreException {
  /// Creates an invalid-argument exception.
  const InvalidArgumentException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.invalidArgument, nativeStatusCode, diagnostic);
}

/// Native invalid-state failure.
final class InvalidStateException extends MaplibreException {
  /// Creates an invalid-state exception.
  const InvalidStateException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.invalidState, nativeStatusCode, diagnostic);
}

/// Native wrong-thread failure.
final class WrongThreadException extends MaplibreException {
  /// Creates a wrong-thread exception.
  const WrongThreadException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.wrongThread, nativeStatusCode, diagnostic);
}

/// Native unsupported-feature failure.
final class UnsupportedFeatureException extends MaplibreException {
  /// Creates an unsupported-feature exception.
  const UnsupportedFeatureException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.unsupported, nativeStatusCode, diagnostic);
}

/// Native error or converted C++ exception.
final class NativeErrorException extends MaplibreException {
  /// Creates a native-error exception.
  const NativeErrorException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.nativeError, nativeStatusCode, diagnostic);
}

/// Native cancellation: the operation completed after cancellation.
final class CancelledException extends MaplibreException {
  /// Creates a cancelled exception.
  const CancelledException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.cancelled, nativeStatusCode, diagnostic);
}

/// Native busy failure: the call raced a driver call in flight.
final class BusyException extends MaplibreException {
  /// Creates a busy exception.
  const BusyException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.busy, nativeStatusCode, diagnostic);
}

/// Native target-loss failure: the render target can no longer be used.
final class TargetLostException extends MaplibreException {
  /// Creates a target-lost exception.
  const TargetLostException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.targetLost, nativeStatusCode, diagnostic);
}

/// Native not-ready failure: no result was ready yet.
final class NotReadyException extends MaplibreException {
  /// Creates a not-ready exception.
  const NotReadyException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.notReady, nativeStatusCode, diagnostic);
}

/// Native not-found failure: no object has the requested ID.
final class NotFoundException extends MaplibreException {
  /// Creates a not-found exception.
  const NotFoundException(int? nativeStatusCode, String diagnostic)
    : super(MaplibreStatus.notFound, nativeStatusCode, diagnostic);
}

/// Loaded native library uses a different C ABI contract version.
final class AbiVersionMismatchException extends MaplibreException {
  /// Creates an ABI-version mismatch exception.
  const AbiVersionMismatchException(String diagnostic)
    : super(MaplibreStatus.abiVersionMismatch, null, diagnostic);
}

/// Failure category returned by a newer native ABI.
final class UnknownMaplibreException extends MaplibreException {
  /// Creates an exception that preserves an unknown native status code.
  const UnknownMaplibreException(
    super.status,
    super.nativeStatusCode,
    super.diagnostic,
  );
}
