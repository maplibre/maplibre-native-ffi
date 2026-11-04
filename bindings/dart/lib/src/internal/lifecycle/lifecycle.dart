import 'dart:async';
import 'dart:ffi';
import 'dart:io';

import '../c/maplibre_native_c.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../status/status.dart';
import '../../error/maplibre_exception.dart';
import '../../values.dart';
import 'native_handles.dart';

final NativeFinalizer _ownerFinalizer = NativeFinalizer(
  Native.addressOf<NativeFinalizerFunction>(raw.mln_binding_owner_finalize),
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

const _createOwnerToken = raw.mln_binding_owner_token_create;

const _destroyOwnerToken = raw.mln_binding_owner_token_destroy;

final class NativeHandleState<H extends NativeHandle> implements Finalizable {
  /// Owns [_handle], unless [borrowed]. The binding never releases or
  /// disposes a handle that a callback borrows.
  NativeHandleState(this._handle, this.typeName, {this.borrowed = false}) {
    if (_handle.isNull) {
      throwInvalidArgument('$typeName handle must not be the null handle');
    }
    if (!borrowed) {
      _attachOwnerFinalizer(_handle);
    }
  }

  final bool borrowed;

  void _checkReleasable() {
    if (borrowed) {
      throw MaplibreException(
        Status.invalidState,
        "$typeName is borrowed by a callback, which can't release it",
      );
    }
  }

  final H _handle;
  // AOT compilation removes a field that is only ever assigned, along with
  // the reference it held, so retain reads this list to add to it.
  final _retained = <Object>[];

  /// Keeps [value], such as a parent owner or callback registrations,
  /// reachable through this owner's lifetime.
  void retain(Object value) => _retained.add(value);

  bool _closed = false;
  final Object _finalizerDetachToken = Object();
  Pointer<Void>? _ownerToken;

  final String typeName;

  int get handleId => _handle.raw;

  bool get isClosed => _closed;

  H get handle => _handle;

  /// Releases the native handle once native accepts [destroy].
  void close(int Function(H) destroy) {
    _checkReleasable();
    if (_closed) return;
    checkNativeCall(destroy(_handle));
    _closed = true;
    _detachOwnerFinalizer();
  }

  /// Runs [answer], which ends the handle whatever it returns. Unlike a
  /// second [close], a repeated answer still calls native, which reports the
  /// handle stale.
  void answer(int Function(H) answer) {
    _checkReleasable();
    final int status;
    try {
      status = answer(_handle);
    } catch (_) {
      // The answer failed before native saw it, so the handle is still live.
      _closed = true;
      _disposeNow();
      rethrow;
    }
    // Native ends a live handle on every outcome.
    _closed = true;
    _detachOwnerFinalizer();
    checkNativeCall(status);
  }

  /// Submits a release and detaches cleanup once native accepts it.
  /// [submit] throws if native rejects the release, which leaves the owner
  /// open and fails the returned future.
  Future<void> closeAsync(Future<void> Function(H) submit) {
    final Future<void> completion;
    try {
      _checkReleasable();
      completion = submit(_handle);
    } catch (error, stack) {
      return Future.error(error, stack);
    }
    _closed = true;
    _detachOwnerFinalizer();
    return completion;
  }

  void _attachOwnerFinalizer(NativeHandle handle) {
    final token = _createOwnerToken(handle.raw);
    if (token == nullptr) {
      throw MaplibreException(
        Status.nativeError,
        'Could not allocate $typeName finalizer state',
      );
    }
    try {
      _ownerFinalizer.attach(this, token, detach: _finalizerDetachToken);
      _ownerToken = token;
      _leakReporter.attach(
        this,
        'Leaked $typeName native handle 0x${handle.raw.toRadixString(16)}; '
        'release it explicitly.',
        detach: _finalizerDetachToken,
      );
    } catch (_) {
      _destroyOwnerToken(token);
      rethrow;
    }
  }

  void _disposeNow() {
    final token = _ownerToken;
    if (token == null) {
      return;
    }
    _ownerFinalizer.detach(_finalizerDetachToken);
    _leakReporter.detach(_finalizerDetachToken);
    _ownerToken = null;
    raw.mln_binding_owner_finalize(token);
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
