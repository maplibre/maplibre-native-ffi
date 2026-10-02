import 'dart:async';
import 'dart:ffi';
import 'dart:io';

import '../c/maplibre_native_c.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../status/status.dart';
import '../../error/maplibre_exception.dart';
import 'native_handles.dart';

final NativeFinalizer _ownerFinalizer = NativeFinalizer(
  Native.addressOf<NativeFinalizerFunction>(raw.mln_adapter_owner_finalize),
);

/// Reports an owner the collector reclaimed while it was still open.
///
/// The native finalizer disposes the handle; this only warns, on standard
/// error, that the program leaked it. A Dart finalizer runs after the
/// collection, from the isolate's event loop, and not at all once the isolate
/// shuts down, so the warning is a best-effort report.
final Finalizer<String> _leakReporter = Finalizer((message) {
  try {
    stderr.writeln(message);
  } catch (_) {
    // A report has nowhere to go once standard error fails.
  }
});

const _createOwnerToken = raw.mln_adapter_owner_token_create;

const _destroyOwnerToken = raw.mln_adapter_owner_token_destroy;

/// Close-once state for an owned native handle.
final class NativeHandleState<H extends NativeHandle> implements Finalizable {
  /// Creates state for a live native handle.
  NativeHandleState(this._handle, this.typeName) {
    if (_handle.isNull) {
      throwInvalidArgument('$typeName handle must not be the null handle');
    }
    _attachOwnerFinalizer(_handle);
  }

  final H _handle;
  Object? callbackStorage;

  /// Keeps callback registrations reachable through this owner's lifetime.
  void retain(Object value) {
    callbackStorage = value;
  }

  bool _closed = false;
  Future<void>? _closeFuture;
  final Object _finalizerDetachToken = Object();
  Pointer<Void>? _ownerToken;

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
  ///
  /// [destroy] returns the status of a call that wrote [nativeDiagnostic].
  void close(int Function(H) destroy) {
    if (_closed) {
      return;
    }

    checkNativeCall(destroy(_handle));
    _closed = true;
    _detachOwnerFinalizer();
  }

  /// Releases the native handle asynchronously exactly once after success.
  ///
  /// Rejected admission throws and preserves the owner. Accepted admission
  /// consumes it immediately; the returned future reports native quiescence.
  Future<void> closeAsync(Future<void> Function(H) close) {
    if (_closed) return _closeFuture ?? Future.value();
    final completion = close(_handle);
    _closed = true;
    _detachOwnerFinalizer();
    return _closeFuture = completion;
  }

  void _attachOwnerFinalizer(NativeHandle handle) {
    final token = _createOwnerToken(handle.raw);
    if (token == nullptr) {
      throw MaplibreException.forNativeStatusCode(
        -5,
        'Could not allocate $typeName finalizer state',
      );
    }
    try {
      _ownerFinalizer.attach(this, token, detach: _finalizerDetachToken);
      _ownerToken = token;
      _leakReporter.attach(
        this,
        'Leaked $typeName native handle 0x${handle.raw.toRadixString(16)}; '
        'close it explicitly.',
        detach: _finalizerDetachToken,
      );
    } catch (_) {
      _destroyOwnerToken(token);
      rethrow;
    }
  }

  void _detachOwnerFinalizer() {
    final token = _ownerToken;
    if (token == null) {
      return;
    }
    _ownerFinalizer.detach(_finalizerDetachToken);
    _leakReporter.detach(_finalizerDetachToken);
    _destroyOwnerToken(token);
    _ownerToken = null;
  }
}
