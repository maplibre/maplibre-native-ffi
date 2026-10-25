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
    leakReportObserver?.call(message);
    stderr.writeln(message);
  } catch (_) {
    // A report has nowhere to go once standard error fails.
  }
});

/// Sees each leak report before it is written to standard error. Tests read
/// reports here instead of overriding standard error.
void Function(String message)? leakReportObserver;

const _createOwnerToken = raw.mln_binding_owner_token_create;

const _destroyOwnerToken = raw.mln_binding_owner_token_destroy;

final class NativeHandleState<H extends NativeHandle> implements Finalizable {
  NativeHandleState(this._handle, this.typeName) {
    if (_handle.isNull) {
      throwInvalidArgument('$typeName handle must not be the null handle');
    }
    _attachOwnerFinalizer(_handle);
  }

  final H _handle;
  // Roots remain reachable through the owner until native retires them.
  // ignore: unused_field
  Object? _callbackRoots;

  /// Keeps callback registrations reachable through this owner's lifetime.
  void retain(Object value) {
    _callbackRoots = value;
  }

  bool _closed = false;
  final Object _finalizerDetachToken = Object();
  Pointer<Void>? _ownerToken;

  final String typeName;

  int get handleId => _handle.raw;

  bool get isClosed => _closed;

  H get handle => _handle;

  /// Releases the native handle once native accepts [destroy].
  void close(int Function(H) destroy) {
    if (_closed) return;
    answer(destroy);
  }

  /// Runs [answer], which native consumes the handle with when it accepts the
  /// call. A repeated answer still reaches native, which reports the handle
  /// stale, where a second [close] does nothing.
  void answer(int Function(H) answer) {
    checkNativeCall(answer(_handle));
    _closed = true;
    _detachOwnerFinalizer();
  }

  /// Submits a release and detaches cleanup once native accepts it.
  /// [submit] throws if native rejects the release, which leaves the owner
  /// open and fails the returned future.
  Future<void> closeAsync(Future<void> Function(H) submit) {
    final Future<void> completion;
    try {
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
