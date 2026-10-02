import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../error/maplibre_exception.dart';
import '../log/log.dart';
import '../internal/callback/completion.dart';
import '../internal/callback/retained.dart';
import '../internal/c/maplibre_native_c.dart'
    show checkNativeCall, ensureAbiVersion, nativeDiagnostic;
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/lifecycle/lifecycle.dart';
import '../internal/lifecycle/native_handles.dart';
import '../internal/memory/memory.dart';
import '../internal/status/status.dart';
import '../internal/value/uint64.dart';
import '../render/native_pointer.dart';

part 'generated_operations.dart';
part 'native_calls.dart';
part 'runtime_offline.dart';

/// Native release roots for the adapter rule contexts.
///
/// Native code retires each registration with a release message, which can
/// arrive after the owner that registered it is closed or collected. Nothing
/// here captures an owner, so one set serves the isolate.
final _callbackReleases = NativeCallbackReleases();

/// Port roots for process-global registrations, such as the log callback.
///
/// Native release retires each port, including from another isolate that
/// replaces or clears the registration.
final _globalCallbackPorts = _NativeCallbackPorts();

final class CallbackPortLifecycleProbe {
  CallbackPortLifecycleProbe._(this._port);
  final _NativeCallbackPort _port;
  bool get closed => _port.closed;

  /// Completes when native code releases the port, or the binding refuses it.
  Future<void> get released => _port._released.future;
}

/// Returns an owner's single pending port, or null after its release.
CallbackPortLifecycleProbe? singleCallbackPortProbeForTesting(Object owner) {
  final pending = switch (owner) {
    MapHandle() => owner._callbackPorts.pending,
    ResourceRequestHandle() => owner._callbackPorts.pending,
    _ => throw ArgumentError.value(owner, 'owner', 'has no callback ports'),
  };
  return pending.isEmpty ? null : CallbackPortLifecycleProbe._(pending.single);
}

/// Returns the most recently registered process-global port that is still
/// pending, such as the log callback's, or null when none is.
CallbackPortLifecycleProbe? globalCallbackPortProbeForTesting() {
  final pending = _globalCallbackPorts.pending;
  return pending.isEmpty ? null : CallbackPortLifecycleProbe._(pending.last);
}

final class _NativeCallbackPorts {
  final pending = <_NativeCallbackPort>{};
  _NativeCallbackPort register(
    Map<int, void Function(List<dynamic>)> callbacks,
  ) {
    final port = _NativeCallbackPort(
      callbacks,
      (port) => pending.remove(port),
      (nativePort) => raw.mln_adapter_dart_port_create(
        NativeApi.postCObject.cast(),
        nativePort,
      ),
      raw.mln_adapter_dart_port_release,
    );
    pending.add(port);
    return port;
  }

  /// Registers a port that receives copied calls of one deferred callback.
  ///
  /// Each message carries a native record that [deliver] must destroy. A
  /// message that arrives after this owner is collected is destroyed unread.
  _NativeCallbackPort registerDeferred(
    int callback,
    void Function(List<dynamic>) deliver,
  ) {
    final port = _NativeCallbackPort(
      {callback: deliver},
      (port) => pending.remove(port),
      (nativePort) => withNativeArena((arena) {
        final context = arena<Pointer<Void>>();
        _check(
          raw.mln_adapter_dart_deferred_callback_create(
            callback,
            NativeApi.postCObject.cast(),
            nativePort,
            context,
            nativeDiagnostic,
          ),
        );
        return context.value;
      }),
      raw.mln_adapter_deferred_callback_release,
      _destroyDeferredMessage,
    );
    pending.add(port);
    return port;
  }
}

void _destroyDeferredMessage(List<dynamic> message) =>
    raw.mln_adapter_deferred_call_record_destroy(
      Pointer<raw.mln_adapter_deferred_call_record>.fromAddress(
        message[1] as int,
      ),
    );

final class _NativeCallbackPort {
  _NativeCallbackPort(
    Map<int, void Function(List<dynamic>)> callbacks,
    this._onReleased,
    Pointer<Void> Function(int nativePort) create,
    this._release, [
    void Function(List<dynamic>)? discard,
  ]) {
    _callbacks = callbacks;
    // A registration lives as long as its owner, so its port leaves the
    // isolate free to finish. Only pending completions keep it alive.
    _port = RawReceivePort()..keepIsolateAlive = false;
    _port.handler = _callbackPortHandler(WeakReference(this), _port, discard);
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
  void Function(List<dynamic>)? discard,
) => (dynamic message) {
  final state = reference.target;
  if (state == null) {
    if (message == 0) {
      port.close();
    } else if (message is List<dynamic>) {
      discard?.call(message);
    }
    return;
  }
  state._deliver(message);
};

/// Decodes a synthetic native batch through the production generated converter.
List<RuntimeEvent> decodeRuntimeEventBatchForTesting(
  raw.mln_runtime_event_batch_view batch,
  RuntimeHandle runtime,
) => _readRuntimeEventBatchView(batch).events;

/// Starts a command and decodes its receipt, including failed dispositions.
Future<CommandCompletion> _startCommand(
  NativeCompletionStart start, {
  void Function()? onRejected,
}) => startNativeCompletion(
  copyKind: raw.MLN_ADAPTER_COMPLETION_COPY_FLAT,
  elementSize: 0,
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
