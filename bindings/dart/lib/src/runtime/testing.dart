part of 'runtime.dart';

// Typed access for the binding's tests to private converters and callback
// roots, so a test runs the code that owners' calls run. The package library
// hides these names.

/// Reads a drained runtime event batch as a drain does.
@visibleForTesting
List<RuntimeEvent> readEventBatchViewForTesting(
  raw.mln_event_batch_view batch,
) => _readEventBatchView(batch);

/// Reads a completion's coordinates as a coordinate query's completion does.
@visibleForTesting
List<LatLng> readLatLngListForTesting(raw.mln_completion_result result) =>
    _resultLatLng.readList(result);

/// Callback roots that prepare wakes as an owner's call does, without a
/// render backend.
@visibleForTesting
final class WakeRootsForTesting {
  final _roots = _CallbackRoots();

  /// Prepares [wake] as a call that carries it does.
  WakeRegistrationForTesting register(Wake wake) =>
      WakeRegistrationForTesting._(_prepareWake(wake, _roots));
}

/// A wake that [WakeRootsForTesting] prepared.
@visibleForTesting
final class WakeRegistrationForTesting {
  WakeRegistrationForTesting._(this._registration);

  final _NativeRegistration<raw.mln_wake> _registration;

  /// The descriptor that native code would hold.
  Pointer<raw.mln_wake> get pointer => _registration.pointer;

  /// Releases the registration as a call that native rejected does.
  void release() {
    _registration.reject();
    _registration.releaseMemory?.call();
  }
}
