part of 'runtime.dart';

_LogCallbackState? _logCallbackState;

final class _LogCallbackState extends RetainedCallbackState {
  _LogCallbackState(LogCallback callback, {required bool consume})
    : _callback = callback {
    var wakeCreated = false;
    var queueAccepted = false;
    try {
      wake = NativeWakeState(() => runUpcall(_drain));
      wakeCreated = true;
      withNativeArena((temporary) {
        final outQueue = temporary<Uint64>();
        final descriptor = temporary<raw.mln_wake>();
        wake.writeTo(descriptor.ref);
        _check(raw.mln_adapter_log_queue_create(descriptor, outQueue));
        queueAccepted = true;
        queue = outQueue.value;
        arena.adoptHandle(queue);
      });
      pointer = arena<raw.mln_adapter_log_callback_state>();
      pointer.ref.queue = queue;
      pointer.ref.consume = consume ? 1 : 0;
      pointer.ref.release_user_data =
          Native.addressOf<
            NativeFunction<raw.mln_log_callback_releaseFunction>
          >(raw.mln_adapter_dart_release);
      pointer.ref.release_context = pointer.cast();
    } catch (_) {
      if (wakeCreated && !queueAccepted) wake.reject();
      arena.releaseAll();
      rethrow;
    }
  }

  final LogCallback _callback;
  final arena = NativeOwnedArena();
  late final Pointer<raw.mln_adapter_log_callback_state> pointer;
  late final NativeWakeState wake;
  late final int queue;

  void _drain() {
    withNativeArena((arena) {
      final outRecord = arena<Pointer<raw.mln_adapter_log_record>>();
      while (true) {
        outRecord.value = nullptr;
        final status = raw.mln_adapter_log_queue_acquire(queue, outRecord);
        if (status == raw.mln_status.MLN_STATUS_INVALID_ARGUMENT) return;
        _check(status);
        final record = outRecord.value;
        if (record == nullptr) {
          return;
        }
        try {
          _callback(_copyLogRecord(record.ref));
        } catch (_) {
          // An exception must not escape into notification delivery.
        } finally {
          raw.mln_adapter_log_record_destroy(record.cast<Void>());
        }
      }
    });
  }

  @override
  void closeResources() {
    if (identical(_logCallbackState, this)) _logCallbackState = null;
    arena.releaseAll();
  }
}

LogRecord _copyLogRecord(raw.mln_adapter_log_record record) {
  return LogRecord(
    severity: LogSeverity.fromRawValue(record.severity),
    event: LogEvent.fromRawValue(record.event),
    code: record.code,
    message: record.message == nullptr
        ? ''
        : record.message.cast<Utf8>().toDartString(),
  );
}

/// Returns the log callback state native code currently dispatches through, or
/// `nullptr` when no callback is registered. Lifecycle tests use this to drive
/// native log dispatch directly.
Pointer<raw.mln_adapter_log_callback_state> logCallbackStateForTesting() =>
    _logCallbackState?.pointer ?? nullptr;
