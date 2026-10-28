import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
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
/// retires them through the isolate's delivery queue.
final class _CallbackRoots {
  final queued = <DeliveryRegistration>{};

  DeliveryRegistration registerQueued(Map<Symbol, Function> callbacks) {
    final registration = isolateDeliveryQueue.register(callbacks);
    queued.add(registration);
    registration.onRetired = () => queued.remove(registration);
    return registration;
  }
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

/// The generations that a committed command published. A command's future
/// fails with [MaplibreException] instead when native reports it failed or
/// cancelled.
final class CommandCompletion {
  const CommandCompletion({
    required this.generation,
    required this.renderUpdateGeneration,
  });

  /// The map snapshot generation that a map command published, or zero for a
  /// runtime command.
  final int generation;

  /// The first map render update that includes the command, or zero when
  /// [generation] is zero.
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

/// Copies a drained batch with [copy], then releases the batch [handle]
/// through [release], even when [copy] throws.
T _drain<T>(int handle, void Function(int) release, T Function() copy) {
  try {
    return copy();
  } finally {
    release(handle);
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
