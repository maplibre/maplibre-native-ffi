import 'dart:ffi';
import 'dart:isolate';

import 'package:ffi/ffi.dart';

import '../c/maplibre_native_c.g.dart' as raw;
import '../status/status.dart';
import 'native_handles.dart';

final NativeFinalizer _leakReporter = NativeFinalizer(
  Native.addressOf<NativeFinalizerFunction>(raw.mln_adapter_handle_leak_report),
);

const _createLeakToken = raw.mln_adapter_handle_leak_token_create;

/// Opaque token for the calling native thread, stable for its life.
const _threadToken = raw.mln_thread_token;

const _destroyLeakToken = raw.mln_adapter_handle_leak_token_destroy;

/// Attaches the binding's non-owning native-handle leak diagnostic to [owner].
final class NativeLeakReporter {
  /// Creates a live leak report token.
  NativeLeakReporter(Finalizable owner, String typeName, NativeHandle handle) {
    final nativeTypeName = typeName.toNativeUtf8().cast<Char>();
    try {
      final token = _createLeakToken(nativeTypeName, handle.raw);
      if (token == nullptr) {
        return;
      }
      _token = token;
      _leakReporter.attach(owner, token, detach: _detachToken);
    } finally {
      calloc.free(nativeTypeName);
    }
  }

  final Object _detachToken = Object();
  Pointer<Void>? _token;

  /// Marks the native lifetime as explicitly released.
  void close() {
    final token = _token;
    if (token == null) {
      return;
    }
    _leakReporter.detach(_detachToken);
    _destroyLeakToken(token);
    _token = null;
  }
}

/// Close-once state for an owned native handle.
final class NativeHandleState<H extends NativeHandle> implements Finalizable {
  /// Creates state for a live native handle.
  NativeHandleState(
    this._handle,
    this.typeName, {
    int? ownerIsolateHash,
    int? ownerThreadToken,
    bool leakReporting = true,
    this.threadAffine = true,
  }) : _ownerIsolateHash = ownerIsolateHash ?? Isolate.current.hashCode,
       _ownerThreadToken = ownerThreadToken ?? _threadToken() {
    if (_handle.isNull) {
      throwInvalidArgument('$typeName handle must not be the null handle');
    }
    if (leakReporting) {
      _attachLeakReporter(_handle);
    }
  }

  final H _handle;
  bool _closed = false;
  final int _ownerIsolateHash;

  /// The native thread this handle was created on. The C API keys its
  /// owner-thread checks on that thread, not on the isolate, and the Dart VM
  /// moves an isolate between threads when it resumes from awaited I/O.
  final int _ownerThreadToken;
  final Object _finalizerDetachToken = Object();
  Pointer<Void>? _leakToken;

  /// Native handle type name used in diagnostics.
  final String typeName;

  /// Whether the C API pins this handle kind to its creating native thread.
  ///
  /// A non-affine handle is callable from any native thread, so its owner
  /// isolate keeps using it after the Dart VM moves the isolate to another
  /// thread on resuming from awaited I/O.
  final bool threadAffine;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _closed;

  /// The issued handle id without owner-isolate validation.
  int get handleId => _handle.raw;

  /// Returns the live handle, or throws when it is closed.
  H get handle {
    _checkOwnerIsolate();
    if (_closed) {
      throwInvalidArgument('$typeName is closed');
    }
    return _handle;
  }

  /// Releases the native handle with [destroy] exactly once after success.
  void close(int Function(H) destroy, String Function() diagnostic) {
    _checkOwnerIsolate();
    if (_closed) {
      return;
    }

    final status = destroy(_handle);
    checkNativeStatus(status, diagnostic);
    _closed = true;
    _detachLeakReporter();
  }

  void _checkOwnerIsolate() {
    if (Isolate.current.hashCode != _ownerIsolateHash) {
      throwWrongThread('$typeName belongs to a different Dart isolate');
    }
    if (threadAffine && _threadToken() != _ownerThreadToken) {
      // Same isolate, different native thread: every native call on this
      // handle would now fail, including close, which leaks it permanently.
      throwWrongThread(
        '$typeName is owned by a native thread its isolate has since left. '
        'The Dart VM moves an isolate between native threads when it resumes '
        'from awaited I/O, so do not await I/O on an isolate that holds a '
        'runtime, map, or render session. See '
        'https://github.com/maplibre/maplibre-native-ffi/issues/412',
      );
    }
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
