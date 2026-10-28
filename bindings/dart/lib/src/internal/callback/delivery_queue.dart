import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:isolate';

import '../../error/maplibre_exception.dart';
import '../../values.dart';
import '../c/maplibre_native_c.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../memory/memory.dart';
import '../status/status.dart';

/// Submits an operation and returns the status of the native call, which
/// writes its message to [nativeDiagnostic].
typedef NativeCompletionStart = int Function(Pointer<raw.mln_completion>);
typedef NativeCompletionDecoder<T> = T Function(raw.mln_completion_result);

abstract interface class _PendingCompletionBase {
  void finish(raw.mln_completion_result result);
  Zone get zone;
}

final class _PendingCompletion<T> implements _PendingCompletionBase {
  _PendingCompletion(this.completer, this.decode, this.submissionStack);

  @override
  final zone = Zone.current;
  final Completer<T> completer;
  final NativeCompletionDecoder<T> decode;
  final StackTrace submissionStack;

  @override
  void finish(raw.mln_completion_result result) {
    try {
      if (result.status != nativeStatusOk) {
        throw MaplibreException(
          Status.fromRawValue(result.status),
          copyCompletionDiagnostic(result.diagnostic),
          generation: result.generation,
        );
      }
      final value = decode(result);
      completer.complete(value);
    } catch (error) {
      completer.completeError(error, submissionStack);
    }
  }
}

/// The callbacks of one queued registration, which its user data token names.
///
/// The queue holds a registration weakly, and the owner that registered it
/// holds it strongly until native code retires it, so a callback that captures
/// its owner leaves the two collectible together. A call that arrives once its
/// owner is gone finds no callback.
final class DeliveryRegistration {
  DeliveryRegistration._(this._queue, this.token, this.callbacks);
  final DeliveryQueue _queue;

  /// The registration's user data.
  final int token;
  final Zone zone = Zone.current;

  /// The registration's callbacks, by the C callback type that runs each.
  final Map<Symbol, Function> callbacks;

  /// Runs once native code retires the registration, or the binding retires
  /// one that native code refused.
  void Function()? onRetired;

  /// The user data that names this registration.
  Pointer<Void> get userData => Pointer.fromAddress(token);

  /// Retires a registration that native code refused.
  void reject() => _queue._retire(token);

  void _finish() {
    onRetired?.call();
  }
}

final NativeFinalizer _queueFinalizer = NativeFinalizer(
  Native.addressOf<NativeFinalizerFunction>(raw.mln_binding_owner_finalize),
);

/// The function pointer of [callable], which only this isolate's dispatch of
/// its queue calls, so it leaves the isolate free to finish.
Pointer<NativeFunction<T>> isolateCallable<T extends Function>(
  NativeCallable<T> callable,
) => (callable..keepIsolateAlive = false).nativeFunction;

final _completionCallback = isolateCallable(
  NativeCallable<raw.mln_completion_callbackFunction>.isolateLocal(_complete),
);

void _complete(
  Pointer<Void> userData,
  Pointer<raw.mln_completion_result> result,
) {
  final pending = isolateDeliveryQueue._pendingCompletions.remove(
    userData.address,
  );
  if (pending == null) return;
  pending.zone.runGuarded(() => pending.finish(result.ref));
}

/// The release_user_data of every queued registration, which retires it.
final dispatchedRelease = isolateCallable(
  NativeCallable<raw.mln_user_data_releaseFunction>.isolateLocal(
    (Pointer<Void> userData) => isolateDeliveryQueue._retire(userData.address),
  ),
);

/// Runs [call] in the zone of the registration that [userData] names, with
/// its callback for the C callback type [type], or with null when its owner
/// is gone.
void runDispatched<C extends Function>(
  Pointer<Void> userData,
  Symbol type,
  void Function(C? callback) call,
) {
  final registration =
      isolateDeliveryQueue._registrations[userData.address]?.target;
  final callback = registration?.callbacks[type] as C?;
  (registration?.zone ?? Zone.root).runGuarded(() => call(callback));
}

/// The delivery queue that every completion and queued callback of this
/// isolate names.
///
/// Native code queues each call and wakes this isolate's port, whose handler
/// dispatches the queue, which runs the calls on this isolate's thread. The
/// port keeps the isolate alive while any completion is pending, so an
/// awaited future resolves before the isolate finishes. Registrations leave
/// the isolate free to finish.
///
/// When the isolate ends, a finalizer releases the queue, which abandons what
/// the isolate never dispatched.
final class DeliveryQueue implements Finalizable {
  DeliveryQueue._();

  RawReceivePort? _port;
  int _handle = 0;
  var _nextToken = 1;
  final _pendingCompletions = <int, _PendingCompletionBase>{};
  final _registrations = <int, WeakReference<DeliveryRegistration>>{};

  /// The queue's native handle, which this call creates on first use.
  int get handle {
    if (_handle == 0) _create();
    return _handle;
  }

  void _create() {
    final port = RawReceivePort()..keepIsolateAlive = false;
    port.handler = (dynamic message) {
      // A message of 1 reports that native code released the wake.
      if (message == 0) _dispatch();
    };
    try {
      _handle = withNativeArena((arena) {
        final wake = arena<raw.mln_wake>();
        checkNativeCall(
          raw.mln_binding_dart_wake_create(
            NativeApi.postCObject.cast(),
            port.sendPort.nativePort,
            wake,
            nativeDiagnostic,
          ),
        );
        final options = arena<raw.mln_delivery_queue_options>();
        options.ref.size = sizeOf<raw.mln_delivery_queue_options>();
        options.ref.wake = wake.ref;
        final queue = arena<Uint64>();
        final status = raw.mln_delivery_queue_create(
          options,
          queue,
          nativeDiagnostic,
        );
        if (status != nativeStatusOk) {
          // A rejected wake stays with the caller, which releases it.
          wake.ref.release_user_data.asFunction<void Function(Pointer<Void>)>()(
            wake.ref.user_data,
          );
          checkNativeCall(status);
        }
        return queue.value;
      });
    } catch (_) {
      port.close();
      rethrow;
    }
    final token = raw.mln_binding_owner_token_create(_handle);
    if (token == nullptr) {
      // Token allocation failure released the queue.
      _handle = 0;
      port.close();
      throw StateError('native delivery queue token allocation failed');
    }
    _queueFinalizer.attach(this, token);
    _port = port;
    _updateLiveness();
  }

  /// Registers the callbacks of one queued registration, which its owner must
  /// keep until it retires.
  DeliveryRegistration register(Map<Symbol, Function> callbacks) {
    final registration = DeliveryRegistration._(this, _nextToken++, callbacks);
    _registrations[registration.token] = WeakReference(registration);
    return registration;
  }

  void _retire(int token) => _registrations.remove(token)?.target?._finish();

  int _addCompletion(_PendingCompletionBase pending) {
    final token = _nextToken++;
    _pendingCompletions[token] = pending;
    _updateLiveness();
    return token;
  }

  void _removeCompletion(int token) {
    _pendingCompletions.remove(token);
    _updateLiveness();
  }

  void _updateLiveness() {
    _port?.keepIsolateAlive = _pendingCompletions.isNotEmpty;
  }

  void _dispatch() {
    try {
      // Only this handler dispatches the queue, and handlers do not nest, so
      // the call runs what is queued or finds nothing.
      raw.mln_delivery_queue_dispatch(_handle, nativeDiagnostic);
    } finally {
      _updateLiveness();
    }
  }
}

/// This isolate's delivery queue.
final isolateDeliveryQueue = DeliveryQueue._();

/// Starts a native completion and returns its future, which fails if native
/// rejects the call or completes it with an error status.
Future<T> startNativeCompletion<T>({
  required NativeCompletionStart start,
  required NativeCompletionDecoder<T> decode,
}) {
  try {
    return submitNativeCompletion(start: start, decode: decode);
  } catch (error, stack) {
    return Future.error(error, stack);
  }
}

/// Starts a native completion like [startNativeCompletion], but throws at the
/// call if native rejects it, for a caller that acts on acceptance.
Future<T> submitNativeCompletion<T>({
  required NativeCompletionStart start,
  required NativeCompletionDecoder<T> decode,
}) {
  final queue = isolateDeliveryQueue;
  final completer = Completer<T>();
  final token = queue._addCompletion(
    _PendingCompletion<T>(completer, decode, StackTrace.current),
  );
  try {
    withNativeArena((arena) {
      final completion = arena<raw.mln_completion>();
      completion.ref.size = sizeOf<raw.mln_completion>();
      completion.ref.callback = _completionCallback;
      completion.ref.user_data = Pointer.fromAddress(token);
      completion.ref.queue = queue.handle;
      // A rejected submission queues nothing and needs no cleanup.
      checkNativeCall(start(completion));
    });
  } catch (_) {
    queue._removeCompletion(token);
    rethrow;
  }
  return completer.future;
}

/// Copies a completion result's diagnostic, which is empty on success.
String copyCompletionDiagnostic(raw.mln_buffer_view diagnostic) {
  if (diagnostic.size == 0 || diagnostic.data == nullptr) {
    return '';
  }
  return utf8.decode(
    diagnostic.data.cast<Uint8>().asTypedList(diagnostic.size),
    allowMalformed: true,
  );
}
