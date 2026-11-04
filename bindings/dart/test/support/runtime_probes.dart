import 'dart:ffi';

import 'package:maplibre_native_ffi/src/runtime/runtime.dart';
import 'package:maplibre_native_ffi/src/values.dart';

/// Prepares wakes as an owner's call would, without a render backend.
final class WakeRootsProbe {
  final _roots = WakeRootsForTesting();

  WakeProbe register(void Function() callback) =>
      WakeProbe._(_roots.register(Wake(callback: callback)));
}

final class WakeProbe {
  WakeProbe._(this._registration);

  final WakeRegistrationForTesting _registration;

  /// Calls the wake as native code does.
  void notify() {
    final wake = _registration.pointer.ref;
    wake.callback.asFunction<void Function(Pointer<Void>)>()(wake.user_data);
  }

  void release() => _registration.release();
}
