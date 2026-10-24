import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../internal/callback/delivery_queue.dart';
import '../internal/callback/native_registration.dart';
import '../internal/c/maplibre_native_c.dart'
    show checkNativeCall, ensureAbiVersion, nativeDiagnostic;
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/lifecycle/lifecycle.dart';
import '../internal/lifecycle/native_handles.dart';
import '../internal/memory/memory.dart';
import '../internal/status/status.dart';
import '../render/native_pointer.dart';
import '../values.dart';

part 'generated_operations.dart';
part 'native_calls.dart';

/// Roots for process-global registrations, such as the log handler.
final _globalCallbackRoots = _CallbackRoots();

/// An owner that roots its callback registrations.
abstract interface class _CallbackRootOwner {
  // ignore: unused_element
  _CallbackRoots get _callbackRoots;
}

/// The callback registrations of one owner, which it keeps until native code
/// retires them: queued registrations through the isolate's delivery queue,
/// and wakes through their ports.
final class _CallbackRoots {
  final queued = <DeliveryRegistration>{};
  final wakes = <_WakeRegistration>{};

  DeliveryRegistration registerQueued(Map<int, DeliveryHandler> handlers) {
    final registration = isolateDeliveryQueue.register(handlers);
    queued.add(registration);
    registration.onRetired = () => queued.remove(registration);
    return registration;
  }

  /// Prepares a wake that posts to a port of this isolate, which runs
  /// [callback] for each wake and closes when native releases the wake.
  ///
  /// The port holds its registration weakly, and this owner holds it until
  /// the port closes, so a callback that captures its owner leaves the two
  /// collectible together. Like a registration, the port leaves the isolate
  /// free to finish.
  _NativeRegistration<raw.mln_wake> wake(void Function()? callback) {
    final arena = Arena();
    final result = arena<raw.mln_wake>();
    if (callback == null) {
      return _NativeRegistration(result, () {}, arena.releaseAll);
    }
    final registration = _WakeRegistration(callback);
    wakes.add(registration);
    final port = _wakePort(
      Zone.current,
      WeakReference(registration),
      WeakReference(this),
    );
    try {
      checkNativeCall(
        raw.mln_binding_dart_wake_create(
          NativeApi.postCObject.cast(),
          port.sendPort.nativePort,
          result,
          nativeDiagnostic,
        ),
      );
    } catch (_) {
      port.close();
      wakes.remove(registration);
      arena.releaseAll();
      rethrow;
    }
    // A rejected call leaves the wake with the caller, whose release posts the
    // message that closes the port.
    return _NativeRegistration(
      result,
      () => result.ref.release_user_data
          .asFunction<void Function(Pointer<Void>)>()(result.ref.user_data),
      arena.releaseAll,
    );
  }
}

final class _WakeRegistration {
  _WakeRegistration(this.callback);

  final void Function() callback;
}

/// The port of one wake. An open port is a root, so its handler reaches the
/// callback and its owner only weakly, and drops the owner's root once native
/// releases the wake.
RawReceivePort _wakePort(
  Zone zone,
  WeakReference<_WakeRegistration> registration,
  WeakReference<_CallbackRoots> owner,
) {
  final port = RawReceivePort()..keepIsolateAlive = false;
  port.handler = (dynamic message) {
    final target = registration.target;
    if (message != 0) {
      port.close();
      if (target != null) owner.target?.wakes.remove(target);
    } else if (target != null) {
      zone.runGuarded(target.callback);
    }
  };
  return port;
}

/// Starts a command, whose future fails unless native commits it.
Future<CommandCompletion> _startCommand(NativeCompletionStart start) =>
    startNativeCompletion(
      start: start,
      decode: (result) => CommandCompletion(
        generation: result.generation,
        renderUpdateGeneration: result.render_update_generation,
      ),
    );

/// The generations that a committed map command published. A command's
/// future fails with [MaplibreException] instead when native reports it
/// failed or cancelled.
final class CommandCompletion {
  const CommandCompletion({
    required this.generation,
    required this.renderUpdateGeneration,
  });

  /// The map snapshot generation that the command published.
  final int generation;

  /// The first map render update that includes the command.
  final int renderUpdateGeneration;

  @override
  bool operator ==(Object other) =>
      other is CommandCompletion &&
      other.generation == generation &&
      other.renderUpdateGeneration == renderUpdateGeneration;

  @override
  int get hashCode => Object.hash(generation, renderUpdateGeneration);

  @override
  String toString() =>
      'CommandCompletion(generation=$generation, '
      'renderUpdateGeneration=$renderUpdateGeneration)';
}

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
