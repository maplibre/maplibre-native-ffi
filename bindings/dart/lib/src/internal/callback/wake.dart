import 'dart:async';
import 'dart:ffi';
import 'dart:isolate';

import 'package:ffi/ffi.dart';

import '../c/maplibre_native_c.g.dart' as raw;
import '../c/maplibre_native_c.dart';
import '../status/status.dart';

/// Keeps wake delivery independent of isolate-owned executable callbacks.
final class NativeWakeState {
  NativeWakeState(void Function() callback) {
    if (NativeApi.majorVersion != 2) {
      throw UnsupportedError('Dart native API version 2 is required');
    }
    _deliver = Zone.current.bindCallbackGuarded(callback);
    _port = RawReceivePort();
    _port.handler = _wakePortHandler(WeakReference(this), _port);
    final descriptor = calloc<raw.mln_wake>();
    try {
      checkNativeStatus(
        raw.mln_adapter_dart_wake_create(
          NativeApi.postCObject.cast<Void>(),
          _port.sendPort.nativePort,
          descriptor,
        ),
        threadLastErrorMessage,
      );
      _callback = descriptor.ref.callback;
      _userData = descriptor.ref.user_data;
      _release = descriptor.ref.release_user_data;
    } catch (_) {
      _port.close();
      rethrow;
    } finally {
      calloc.free(descriptor);
    }
  }

  late final RawReceivePort _port;
  late final void Function() _deliver;
  void _receive(dynamic message) {
    if (message == 0 && !_retired) {
      _deliver();
    } else if (message == 1) {
      _retired = true;
      _port.close();
    }
  }

  late final raw.mln_wake_callback _callback;
  late final Pointer<Void> _userData;
  late final raw.mln_wake_release _release;
  var _retired = false;

  /// Writes the descriptor into an owning native call.
  void writeTo(raw.mln_wake wake) {
    wake.size = sizeOf<raw.mln_wake>();
    wake.callback = _callback;
    wake.user_data = _userData;
    wake.release_user_data = _release;
  }

  /// Releases a descriptor that the native call rejected.
  void reject() {
    if (_retired) return;
    _retired = true;
    _port.close();
    _release.asFunction<void Function(Pointer<Void>)>()(_userData);
  }
}

void Function(dynamic) _wakePortHandler(
  WeakReference<NativeWakeState> reference,
  RawReceivePort port,
) => (dynamic message) {
  final state = reference.target;
  if (state == null) {
    if (message == 1) port.close();
    return;
  }
  state._receive(message);
};
