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

/// Handles one drained delivery of the batch it arrived in. A handler that
/// keeps an owned handle takes it from the batch with
/// `mln_delivery_batch_take` before the batch is released.
typedef DeliveryHandler =
    void Function(int batch, int index, Pointer<raw.mln_delivery> delivery);

abstract interface class _PendingCompletionBase {
  void finish(int batch, int index, raw.mln_completion_result result);
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
  void finish(int batch, int index, raw.mln_completion_result result) {
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

/// The handlers of one queued callback registration, which its user data
/// token names.
///
/// The queue holds a registration weakly, and the owner that registered it
/// holds it strongly until native code retires it, so a callback that captures
/// its owner leaves the two collectible together. A delivery that arrives once
/// its owner is gone is released unread.
final class DeliveryRegistration {
  DeliveryRegistration._(this._queue, this.token, this.handlers);
  final DeliveryQueue _queue;

  /// The registration's user data.
  final int token;
  final Zone zone = Zone.current;
  final Map<int, DeliveryHandler> handlers;

  /// Runs once native code retires the registration, or the binding retires
  /// one that native code refused.
  void Function()? onRetired;

  /// Retires a registration that native code refused.
  void reject() => _queue._retire(token);

  void _finish() {
    onRetired?.call();
  }
}

final NativeFinalizer _queueFinalizer = NativeFinalizer(
  Native.addressOf<NativeFinalizerFunction>(raw.mln_binding_owner_finalize),
);

/// The delivery queue that every completion and queued callback of this
/// isolate arrives on.
///
/// Native code copies every completion and queued callback delivery into
/// the queue and wakes this isolate's port, whose handler drains the
/// queue and dispatches each delivery by its user data token. The port keeps
/// the isolate alive while any completion is pending, so an awaited future
/// resolves before the isolate finishes. Registrations leave the isolate free
/// to finish.
///
/// When the isolate ends, a finalizer releases the queue, which disposes what
/// the isolate never drained.
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
      if (message == 0) _drain();
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

  /// Registers the handlers of one queued callback registration, which its
  /// owner must keep until it retires.
  DeliveryRegistration register(Map<int, DeliveryHandler> handlers) {
    final registration = DeliveryRegistration._(this, _nextToken++, handlers);
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

  void _drain() {
    try {
      while (true) {
        final drained = withNativeArena((arena) {
          final batch = arena<Uint64>();
          final view = arena<raw.mln_delivery_batch_view>();
          view.ref.size = sizeOf<raw.mln_delivery_batch_view>();
          final status = raw.mln_delivery_queue_drain(
            _handle,
            batch,
            view,
            nativeDiagnostic,
          );
          if (status == nativeStatusNativeError) {
            // The deliveries stay queued. A pending completion may get no
            // later push to wake the queue. Back off so persistent allocation
            // failure leaves the isolate free to run other work.
            Timer(const Duration(milliseconds: 50), _drain);
            return false;
          }
          if (status != nativeStatusOk) return false;
          try {
            final stride = view.ref.delivery_size;
            if (stride < sizeOf<raw.mln_delivery>()) {
              throw StateError('native delivery stride is too small');
            }
            final base = view.ref.deliveries.cast<Uint8>();
            for (var index = 0; index < view.ref.delivery_count; index++) {
              _dispatch(
                batch.value,
                index,
                (base + index * stride).cast<raw.mln_delivery>(),
              );
            }
          } finally {
            raw.mln_delivery_batch_release(batch.value);
          }
          return true;
        });
        if (!drained) return;
      }
    } finally {
      _updateLiveness();
    }
  }

  void _dispatch(int batch, int index, Pointer<raw.mln_delivery> delivery) {
    final token = delivery.ref.user_data.address;
    switch (delivery.ref.type) {
      case raw.MLN_DELIVERY_TYPE_COMPLETION:
        final pending = _pendingCompletions.remove(token);
        if (pending == null) return;
        pending.zone.runGuarded(
          () => pending.finish(batch, index, delivery.ref.payload.completion),
        );
      case raw.MLN_DELIVERY_TYPE_RETIRED:
        _retire(token);
      case final type:
        // A delivery for a registration this isolate no longer holds is
        // released unread with its batch.
        final registration = _registrations[token]?.target;
        final handler = registration?.handlers[type];
        if (registration == null || handler == null) return;
        registration.zone.runGuarded(() => handler(batch, index, delivery));
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
