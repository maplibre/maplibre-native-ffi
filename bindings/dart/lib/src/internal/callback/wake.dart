import 'dart:async';
import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../c/maplibre_native_c.g.dart' as raw;

final _wakeStates = <int, NativeWakeState>{};

final NativeCallable<raw.mln_wake_releaseFunction> _wakeReleaseListener =
    NativeCallable<raw.mln_wake_releaseFunction>.listener((
      Pointer<Void> userData,
    ) {
      _wakeStates.remove(userData.address)?._releasedByNative();
    });

/// Owns one Dart wake callback until native code reports it quiescent.
final class NativeWakeState {
  NativeWakeState(void Function() callback) {
    _token = calloc<Uint8>();
    _listener = NativeCallable<raw.mln_wake_callbackFunction>.listener((
      Pointer<Void> _,
    ) {
      callback();
    });
    _wakeStates[_token.address] = this;
  }

  late final Pointer<Uint8> _token;
  late final NativeCallable<raw.mln_wake_callbackFunction> _listener;
  final _released = Completer<void>();
  var _retired = false;

  /// Completes after native code can no longer invoke the wake callback.
  Future<void> get released => _released.future;

  /// Writes this callback into an owning C wake descriptor.
  void writeTo(raw.mln_wake wake) {
    wake.size = sizeOf<raw.mln_wake>();
    wake.callback = _listener.nativeFunction;
    wake.user_data = _token.cast<Void>();
    wake.release_user_data = _wakeReleaseListener.nativeFunction;
  }

  /// Releases a descriptor that its owning C call rejected.
  void reject() {
    if (_retired) return;
    _retired = true;
    _wakeStates.remove(_token.address);
    _listener.close();
    calloc.free(_token);
    _released.complete();
  }

  void _releasedByNative() {
    if (_retired) return;
    _retired = true;
    // A NativeCallable.listener returns to C after queueing the Dart upcall.
    // Defer disposal by one event turn so any earlier queued wake can finish.
    Timer.run(() {
      _listener.close();
      calloc.free(_token);
      _released.complete();
    });
  }
}
