import 'dart:async';
import 'dart:convert';
import 'dart:ffi';

import '../c/maplibre_native_c.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../memory/memory.dart';
import '../status/status.dart';

typedef NativeCompletionStart = int Function(Pointer<raw.mln_completion>);
typedef NativeCompletionDecoder<T> = T Function(raw.mln_completion_result);

abstract interface class _PendingCompletionBase {
  void finish(Pointer<raw.mln_adapter_completion_record> record);
  void fail(Object error);
}

final class _PendingCompletion<T> implements _PendingCompletionBase {
  _PendingCompletion(
    this.completer,
    this.decode,
    this.submissionStack,
    this.acceptErrorStatus,
  );

  final Completer<T> completer;
  final NativeCompletionDecoder<T> decode;
  final StackTrace submissionStack;
  final bool acceptErrorStatus;

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
      completer.complete(decode(result));
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

final NativeCallable<raw.mln_adapter_completion_listenerFunction>
_completionListener =
    NativeCallable<raw.mln_adapter_completion_listenerFunction>.listener((
      Pointer<Void> userData,
      Pointer<raw.mln_adapter_completion_record> record,
    ) {
      final pending = _pendingCompletions.remove(userData.address);
      if (pending == null) {
        if (record != nullptr) {
          raw.mln_adapter_completion_record_destroy(record);
        }
        return;
      }
      if (record == nullptr) {
        pending.fail(
          StateError('native completion adapter could not copy the result'),
        );
        return;
      }
      pending.finish(record);
    });

Future<T> startNativeCompletion<T>({
  required raw.mln_adapter_completion_copy_kind copyKind,
  required int elementSize,
  required NativeCompletionStart start,
  required NativeCompletionDecoder<T> decode,
  bool acceptErrorStatus = false,
  void Function()? onRejected,
}) {
  final completer = Completer<T>();
  final token = _nextCompletionToken++;
  final userData = Pointer<Void>.fromAddress(token);
  _pendingCompletions[token] = _PendingCompletion<T>(
    completer,
    decode,
    StackTrace.current,
    acceptErrorStatus,
  );

  try {
    withNativeArena((arena) {
      final completion = arena<raw.mln_completion>();
      checkNativeStatus(
        raw.mln_adapter_completion_create(
          copyKind.value,
          elementSize,
          _completionListener.nativeFunction,
          userData,
          completion,
        ),
        threadLastErrorMessage,
      );
      var rejected = false;
      try {
        final status = start(completion);
        if (status != nativeStatusOk) {
          // The rejection clears the thread-local diagnostic, so read the
          // submission's message before rejecting.
          final diagnostic = threadLastErrorMessage();
          raw.mln_adapter_completion_reject(completion);
          rejected = true;
          checkNativeStatus(status, () => diagnostic);
        }
      } catch (_) {
        if (!rejected) raw.mln_adapter_completion_reject(completion);
        rethrow;
      }
    });
  } catch (_) {
    _pendingCompletions.remove(token);
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
