import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../c/maplibre_native_c.g.dart' as raw;

/// Runs [body] with zeroed native memory that it must not use after it
/// returns.
T withNativeArena<T>(T Function(Allocator arena) body) {
  final arena = _CallArena();
  try {
    return body(arena);
  } finally {
    arena._release();
  }
}

const _blockSize = 1024;

// Matches malloc, since an allocator call carries no alignment.
const _alignment = 16;

// The outermost call of the isolate bumps through this block, so a small call
// allocates nothing. A call made while it is leased, as from a synchronous
// callback, allocates its own blocks.
final _scratch = _ScratchBlock();

final _freeBlock = NativeFinalizer(calloc.nativeFree);

// Frees its block when the isolate ends.
final class _ScratchBlock implements Finalizable {
  _ScratchBlock() : pointer = calloc<Uint8>(_blockSize) {
    _freeBlock.attach(this, pointer.cast());
  }

  final Pointer<Uint8> pointer;
  bool leased = false;
}

final class _CallArena implements Allocator {
  int _cursor = 0;
  int _limit = 0;
  bool _leased = false;
  // Where the scratch block's allocations end, once this arena has left it.
  int _scratchEnd = 0;
  List<Pointer<Uint8>>? _blocks;

  @override
  Pointer<T> allocate<T extends NativeType>(int byteCount, {int? alignment}) {
    final start = (_cursor + _alignment - 1) & -_alignment;
    if (_cursor != 0 && start + byteCount <= _limit) {
      _cursor = start + byteCount;
      return Pointer.fromAddress(start);
    }
    if (byteCount > _blockSize ~/ 4) return _block(byteCount).cast();
    final Pointer<Uint8> block;
    if (_cursor == 0 && !_scratch.leased) {
      _scratch.leased = _leased = true;
      block = _scratch.pointer;
    } else {
      if (_leased && _scratchEnd == 0) _scratchEnd = _cursor;
      block = _block(_blockSize);
    }
    _cursor = block.address + byteCount;
    _limit = block.address + _blockSize;
    return block.cast();
  }

  Pointer<Uint8> _block(int byteCount) {
    final block = calloc<Uint8>(byteCount);
    (_blocks ??= []).add(block);
    return block;
  }

  @override
  void free(Pointer<NativeType> pointer) {}

  // The scratch block returns zeroed, so the next call reads no stale bytes.
  // Word stores are several times faster here than Uint8List.fillRange.
  void _release() {
    if (_leased) {
      final end = _scratchEnd == 0 ? _cursor : _scratchEnd;
      final words = _scratch.pointer.cast<Uint64>();
      final count = (end - _scratch.pointer.address + 7) >> 3;
      for (var index = 0; index < count; index++) {
        words[index] = 0;
      }
      _scratch.leased = false;
    }
    final blocks = _blocks;
    if (blocks != null) {
      for (final block in blocks) {
        calloc.free(block);
      }
    }
  }
}

final class NativeStringView {
  const NativeStringView(this.value, this.byteLength);

  final raw.mln_buffer_view value;

  final int byteLength;
}

raw.mln_buffer_view nativeBufferView(Uint8List value, Allocator allocator) {
  final data = allocator<Uint8>(value.isEmpty ? 1 : value.length);
  if (value.isNotEmpty) data.asTypedList(value.length).setAll(0, value);
  final view = allocator<raw.mln_buffer_view>();
  view.ref.data = data.cast<Void>();
  view.ref.size = value.length;
  return view.ref;
}

NativeStringView nativeStringView(String value, Allocator allocator) {
  final bytes = utf8.encode(value);
  final data = allocator<Uint8>(bytes.isEmpty ? 1 : bytes.length);
  data.asTypedList(bytes.length).setAll(0, bytes);

  final view = allocator<raw.mln_buffer_view>();
  view.ref.data = data.cast<Void>();
  view.ref.size = bytes.length;
  return NativeStringView(view.ref, bytes.length);
}
