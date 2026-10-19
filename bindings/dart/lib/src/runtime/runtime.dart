import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../error/maplibre_exception.dart';
import '../internal/callback/delivery_queue.dart';
import '../internal/c/maplibre_native_c.dart'
    show checkNativeCall, ensureAbiVersion, nativeDiagnostic;
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/lifecycle/lifecycle.dart';
import '../internal/lifecycle/native_handles.dart';
import '../internal/memory/memory.dart';
import '../internal/status/status.dart';
import '../internal/value/uint64.dart';
import '../render/native_pointer.dart';
import '../values.dart';

part 'generated_operations.dart';
part 'native_calls.dart';

/// Port roots for process-global registrations. The log callback arrives
/// through the isolate's delivery queue instead, which needs no port roots.
final _globalCallbackPorts = _NativeCallbackPorts();

final class CallbackPortLifecycleProbe {
  CallbackPortLifecycleProbe._(this._port);
  final _NativeCallbackPort _port;
  bool get closed => _port.closed;

  /// Completes when native code releases the port, or the binding refuses it.
  Future<void> get released => _port._released.future;
}

/// An owner that roots the ports of its callback registrations.
abstract interface class _CallbackPortOwner {
  _NativeCallbackPorts get _callbackPorts;
}

/// Returns an owner's single pending port, or null after its release.
CallbackPortLifecycleProbe? singleCallbackPortProbeForTesting(Object owner) {
  final pending = owner is _CallbackPortOwner
      ? owner._callbackPorts.pending
      : throw ArgumentError.value(owner, 'owner', 'has no callback ports');
  return pending.isEmpty ? null : CallbackPortLifecycleProbe._(pending.single);
}

/// Returns the most recent log callback registration that native code has
/// not yet retired, or null when none is pending.
DeliveryRegistration? logRegistrationProbeForTesting() => isolateDeliveryQueue
    .latestRegistrationForTesting(raw.MLN_DELIVERY_TYPE_LOG_RECORD);

final class _NativeCallbackPorts {
  final pending = <_NativeCallbackPort>{};

  /// Queued registrations, which this owner keeps until they retire.
  final queued = <DeliveryRegistration>{};

  DeliveryRegistration registerQueued(Map<int, DeliveryHandler> handlers) {
    final registration = isolateDeliveryQueue.register(handlers);
    queued.add(registration);
    registration.onRetired = () => queued.remove(registration);
    return registration;
  }

  _NativeCallbackPort register(
    Map<int, void Function(List<dynamic>)> callbacks,
  ) {
    final port = _NativeCallbackPort(
      callbacks,
      (port) => pending.remove(port),
      (nativePort) => raw.mln_binding_dart_port_create(
        NativeApi.postCObject.cast(),
        nativePort,
      ),
      raw.mln_binding_dart_port_release,
    );
    pending.add(port);
    return port;
  }
}

final class _NativeCallbackPort {
  _NativeCallbackPort(
    Map<int, void Function(List<dynamic>)> callbacks,
    this._onReleased,
    Pointer<Void> Function(int nativePort) create,
    this._release,
  ) {
    _callbacks = callbacks;
    // A registration lives as long as its owner, so its port leaves the
    // isolate free to finish. Only pending completions keep it alive.
    _port = RawReceivePort()..keepIsolateAlive = false;
    _port.handler = _callbackPortHandler(WeakReference(this), _port);
    try {
      context = create(_port.sendPort.nativePort);
    } catch (_) {
      _port.close();
      rethrow;
    }
    if (context == nullptr) {
      _port.close();
      throw StateError('native callback port allocation failed');
    }
  }
  final void Function(_NativeCallbackPort) _onReleased;
  final void Function(Pointer<Void>) _release;
  final _zone = Zone.current;
  late final Map<int, void Function(List<dynamic>)> _callbacks;
  void _deliver(dynamic message) {
    _zone.runGuarded(() {
      if (message == 0) {
        _finish();
        return;
      }
      final values = message as List<dynamic>;
      _callbacks[values[0]]?.call(values);
    });
  }

  var _closed = false;
  final _released = Completer<void>();
  late final RawReceivePort _port;
  late final Pointer<Void> context;
  bool get closed => _closed;
  void _finish() {
    if (closed) return;
    _closed = true;
    _port.close();
    _callbacks.clear();
    _onReleased(this);
    _released.complete();
  }

  void reject() {
    _release(context);
    _finish();
  }
}

void Function(dynamic) _callbackPortHandler(
  WeakReference<_NativeCallbackPort> reference,
  RawReceivePort port,
) => (dynamic message) {
  final state = reference.target;
  if (state == null) {
    if (message == 0) port.close();
    return;
  }
  state._deliver(message);
};

/// Decodes a synthetic native batch through the production generated converter.
List<RuntimeEvent> decodeRuntimeEventBatchForTesting(
  raw.mln_event_batch_view batch,
) => _readEventBatchView(batch).events;

/// Decodes a synthetic array completion of coordinates through the production
/// element reader.
List<LatLng> decodeLatLngListForTesting(raw.mln_completion_result result) =>
    _resultLatLng.readList(result);

/// Starts a command and decodes its receipt, including failed dispositions.
Future<CommandCompletion> _startCommand(
  NativeCompletionStart start, {
  void Function()? onRejected,
}) => startNativeCompletion(
  start: start,
  onRejected: onRejected,
  acceptErrorStatus: true,
  decode: (result) => CommandCompletion(
    disposition: CommandDisposition.fromRawValue(result.disposition),
    generation: uint64FromNative(result.generation),
    status: MaplibreStatus.fromNativeStatusCode(result.status),
    diagnostic: copyCompletionDiagnostic(result.diagnostic),
  ),
);

final class CommandCompletion {
  const CommandCompletion({
    required this.disposition,
    required this.generation,
    required this.status,
    required this.diagnostic,
  });

  final CommandDisposition disposition;
  final BigInt generation;
  final MaplibreStatus status;
  final String diagnostic;
}

/// Runs the owned-output adoption that generated operations use, for tests of
/// its failure paths.
T adoptOwnedForTesting<T>(
  int handle,
  T Function() adopt,
  void Function(int) dispose,
) => _adoptOwned(handle, adopt, dispose);

T _adoptOwned<T>(int handle, T Function() adopt, void Function(int) dispose) {
  try {
    return adopt();
  } catch (error, stack) {
    try {
      dispose(handle);
    } catch (cleanup) {
      throw NativeAdoptionFailure(error, cleanup, () => dispose(handle));
    }
    Error.throwWithStackTrace(error, stack);
  }
}

final class NativeAdoptionFailure implements Exception {
  NativeAdoptionFailure(
    this.adoptionError,
    this.cleanupError,
    this.retryCleanup,
  );
  final Object adoptionError;
  final Object cleanupError;
  final void Function() retryCleanup;
}

void _check(int status) => checkNativeCall(status);

/// Checks [status] like [_check], except that [absent] reports that native
/// published no output. Returns whether the output holds a value.
bool _present(int status, int absent) {
  if (status == absent) return false;
  _check(status);
  return true;
}
