import 'dart:async';
import 'dart:ffi';
import 'dart:isolate';
import 'package:ffi/ffi.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../c/maplibre_native_c.dart';
import '../memory/memory.dart';

final class _NativeArenaAllocator implements Allocator {
  _NativeArenaAllocator() : pointer = raw.mln_adapter_arena_create() {
    if (pointer == nullptr) {
      throw StateError('native callback arena allocation failed');
    }
  }
  Pointer<Void> pointer;

  @override
  Pointer<T> allocate<T extends NativeType>(int byteCount, {int? alignment}) {
    if (pointer == nullptr) {
      throw StateError('native callback arena was transferred');
    }
    final result = raw.mln_adapter_arena_allocate(
      pointer,
      byteCount,
      alignment ?? 16,
    );
    if (result == nullptr) {
      throw ArgumentError('native callback allocation failed');
    }
    return result.cast<T>();
  }

  @override
  void free(Pointer<NativeType> pointer) {}
}

final class NativeOwnedArena extends Arena {
  factory NativeOwnedArena() => NativeOwnedArena._(_NativeArenaAllocator());
  NativeOwnedArena._(this._allocator) : super(_allocator);
  final _NativeArenaAllocator _allocator;

  Pointer<Void> take() {
    final result = _allocator.pointer;
    _allocator.pointer = nullptr;
    return result;
  }

  /// Runs [release] with [context] when native code destroys this arena.
  void adoptRelease(
    Pointer<NativeFunction<raw.mln_runtime_callback_releaseFunction>> release,
    Pointer<Void> context,
  ) => checkNativeCall(
    raw.mln_adapter_arena_adopt_release(
      _allocator.pointer,
      release,
      context,
      nativeDiagnostic,
    ),
  );
  @override
  void releaseAll({bool reuse = false}) {
    super.releaseAll();
    raw.mln_adapter_arena_destroy(take());
  }
}

final class NativeCallbackReleases {
  RawReceivePort? _port;
  final _zone = Zone.current;
  final _callbacks = <int, void Function()>{};

  void reject(Pointer<Void> userData) => raw.mln_adapter_dart_release(userData);

  void release(int registration) {
    _callbacks.remove(registration)?.call();
    if (_callbacks.isEmpty) {
      _port?.close();
      _port = null;
    }
  }

  void register(
    Pointer<Void> userData,
    void Function() dispose, {
    NativeOwnedArena? arena,
  }) {
    var accepted = false;
    try {
      // Releases follow registrations that outlive any awaited work, so
      // their port leaves the isolate free to finish, as theirs do.
      final port = _port ??= (RawReceivePort()..keepIsolateAlive = false);
      port.handler = _releasePortHandler(WeakReference(this), port);
      withNativeArena((temporary) {
        final registration = temporary<Uint64>();
        checkNativeCall(
          raw.mln_adapter_dart_release_register(
            NativeApi.postCObject.cast<Void>(),
            port.sendPort.nativePort,
            userData,
            arena?.take() ?? nullptr,
            registration,
            nativeDiagnostic,
          ),
        );
        accepted = true;
        _callbacks[registration.value] = dispose;
      });
    } catch (_) {
      if (accepted) raw.mln_adapter_dart_release(userData);
      dispose();
      if (_callbacks.isEmpty) {
        _port?.close();
        _port = null;
      }
      rethrow;
    }
  }
}

void Function(dynamic) _releasePortHandler(
  WeakReference<NativeCallbackReleases> reference,
  RawReceivePort port,
) => (dynamic registration) {
  final state = reference.target;
  if (state == null) {
    port.close();
    return;
  }
  state._zone.runGuarded(() => state.release(registration as int));
};
