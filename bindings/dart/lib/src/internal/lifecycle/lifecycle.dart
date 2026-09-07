import 'dart:async';
import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../c/maplibre_native_c.g.dart' as raw;
import '../status/status.dart';
import 'native_handles.dart';

final NativeFinalizer _leakReporter = NativeFinalizer(
  Native.addressOf<NativeFinalizerFunction>(raw.mln_adapter_handle_leak_report),
);

const _createLeakToken = raw.mln_adapter_handle_leak_token_create;

const _destroyLeakToken = raw.mln_adapter_handle_leak_token_destroy;

/// Close-once state for an owned native handle.
final class NativeHandleState<H extends NativeHandle> implements Finalizable {
  /// Creates state for a live native handle.
  NativeHandleState(this._handle, this.typeName, {bool leakReporting = true}) {
    if (_handle.isNull) {
      throwInvalidArgument('$typeName handle must not be the null handle');
    }
    if (leakReporting) {
      _attachLeakReporter(_handle);
    }
  }

  final H _handle;
  bool _closed = false;
  Future<void>? _closeFuture;
  final Object _finalizerDetachToken = Object();
  Pointer<Void>? _leakToken;

  /// Native handle type name used in diagnostics.
  final String typeName;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _closed;

  /// The issued handle ID.
  int get handleId => _handle.raw;

  /// Returns the live handle, or throws when it is closed.
  H get handle {
    if (_closed) {
      throwInvalidArgument('$typeName is closed');
    }
    return _handle;
  }

  /// Releases the native handle with [destroy] exactly once after success.
  void close(int Function(H) destroy, String Function() diagnostic) {
    if (_closed) {
      return;
    }

    final status = destroy(_handle);
    checkNativeStatus(status, diagnostic);
    _closed = true;
    _detachLeakReporter();
  }

  /// Releases the native handle asynchronously exactly once after success.
  ///
  /// [close] starts before this returns, and a release that fails reaches the
  /// caller as an error on the returned future and leaves this state open for
  /// a later attempt.
  Future<void> closeAsync(Future<void> Function(H) close) {
    if (_closed) {
      return Future.value();
    }
    final pending = _closeFuture;
    if (pending != null) {
      return pending;
    }
    // The completer holds the shared future before the release runs, because a
    // rejected release reports its failure while `close` is still on the
    // stack.
    final completer = Completer<void>();
    _closeFuture = completer.future;
    _runClose(close).then(
      (_) => completer.complete(),
      onError: (Object error, StackTrace stack) {
        _closeFuture = null;
        completer.completeError(error, stack);
      },
    );
    return completer.future;
  }

  Future<void> _runClose(Future<void> Function(H) close) async {
    await close(_handle);
    _closed = true;
    _detachLeakReporter();
  }

  void _attachLeakReporter(NativeHandle handle) {
    final nativeTypeName = typeName.toNativeUtf8().cast<Char>();
    try {
      final token = _createLeakToken(nativeTypeName, handle.raw);
      if (token == nullptr) {
        return;
      }
      _leakToken = token;
      _leakReporter.attach(this, token, detach: _finalizerDetachToken);
    } finally {
      calloc.free(nativeTypeName);
    }
  }

  void _detachLeakReporter() {
    final token = _leakToken;
    if (token == null) {
      return;
    }
    _leakReporter.detach(_finalizerDetachToken);
    _destroyLeakToken(token);
    _leakToken = null;
  }
}
