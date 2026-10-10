import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:isolate';

import '../c/maplibre_native_c.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../memory/memory.dart';
import '../status/status.dart';

/// Submits an operation and returns the status of the native call, which
/// writes its message to [nativeDiagnostic].
typedef NativeCompletionStart = int Function(Pointer<raw.mln_completion>);
typedef NativeCompletionDecoder<T> = T Function(raw.mln_completion_result);

abstract interface class _PendingCompletionBase {
  void finish(Pointer<raw.mln_adapter_completion_record> record);
  void fail(Object error);
  Zone get zone;
}

final class _PendingCompletion<T> implements _PendingCompletionBase {
  _PendingCompletion(
    this.completer,
    this.decode,
    this.submissionStack,
    this.acceptErrorStatus,
    this.claimBeforeDecode,
  );

  @override
  final zone = Zone.current;
  final Completer<T> completer;
  final NativeCompletionDecoder<T> decode;
  final StackTrace submissionStack;
  final bool acceptErrorStatus;
  final bool claimBeforeDecode;

  @override
  void finish(Pointer<raw.mln_adapter_completion_record> record) {
    try {
      final result = record.ref.result;
      if (!acceptErrorStatus) {
        checkNativeStatus(result.status, () {
          final diagnostic = copyCompletionDiagnostic(result.diagnostic);
          return diagnostic.isEmpty ? 'native operation failed' : diagnostic;
        });
      }
      if (claimBeforeDecode) raw.mln_adapter_completion_record_adopt(record);
      final value = decode(result);
      raw.mln_adapter_completion_record_adopt(record);
      completer.complete(value);
    } catch (error) {
      completer.completeError(error, submissionStack);
    } finally {
      raw.mln_adapter_completion_record_destroy(record);
    }
  }

  @override
  void fail(Object error) => completer.completeError(error, submissionStack);
}

final _pendingCompletions = <int, _PendingCompletionBase>{};
var _nextCompletionToken = 1;

/// The port every pending completion of this isolate arrives on.
///
/// It keeps the isolate alive while any completion is pending, so an awaited
/// future resolves before the isolate finishes, and closes once none is.
/// Callback registrations use ports that leave the isolate free to finish.
RawReceivePort? _completionListener;

RawReceivePort _createCompletionListener() => RawReceivePort((dynamic message) {
  final values = message as List<dynamic>;
  final pending = _pendingCompletions.remove(values[0] as int);
  final record = Pointer<raw.mln_adapter_completion_record>.fromAddress(
    values[1] as int,
  );
  try {
    if (pending == null) {
      if (record != nullptr) raw.mln_adapter_completion_record_destroy(record);
      return;
    }
    pending.zone.runGuarded(() {
      if (record == nullptr) {
        pending.fail(
          StateError('native completion adapter could not copy the result'),
        );
      } else {
        pending.finish(record);
      }
    });
  } finally {
    _closeIdleCompletionListener();
  }
});

void _closeIdleCompletionListener() {
  if (_pendingCompletions.isEmpty) {
    _completionListener?.close();
    _completionListener = null;
  }
}

Future<T> startNativeCompletion<T>({
  required int copyKind,
  required NativeCompletionStart start,
  required NativeCompletionDecoder<T> decode,
  bool acceptErrorStatus = false,
  bool claimBeforeDecode = false,
  void Function()? onRejected,
}) {
  final completer = Completer<T>();
  final token = _nextCompletionToken++;
  _pendingCompletions[token] = _PendingCompletion<T>(
    completer,
    decode,
    StackTrace.current,
    acceptErrorStatus,
    claimBeforeDecode,
  );

  try {
    withNativeArena((arena) {
      final completion = arena<raw.mln_completion>();
      checkNativeCall(
        raw.mln_adapter_dart_completion_create(
          copyKind,
          NativeApi.postCObject.cast(),
          (_completionListener ??= _createCompletionListener())
              .sendPort
              .nativePort,
          token,
          completion,
          nativeDiagnostic,
        ),
      );
      var rejected = false;
      try {
        final status = start(completion);
        if (status != nativeStatusOk) {
          raw.mln_adapter_completion_reject(completion);
          rejected = true;
          checkNativeCall(status);
        }
      } catch (_) {
        if (!rejected) raw.mln_adapter_completion_reject(completion);
        rethrow;
      }
    });
  } catch (_) {
    _pendingCompletions.remove(token);
    _closeIdleCompletionListener();
    onRejected?.call();
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
