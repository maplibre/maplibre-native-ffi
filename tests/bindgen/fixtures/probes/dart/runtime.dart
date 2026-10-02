// The part of the Dart runtime that generated operations call, reduced to what
// the protocol fixture needs. The probe adds the generated operations as a part.
library;

import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'values.dart';
import 'raw.dart' as raw;

part 'generated_operations.dart';

final _process = DynamicLibrary.process();
final _calloc = _process.lookupFunction<
    Pointer<Void> Function(Size, Size), Pointer<Void> Function(int, int)>(
  'calloc',
);
final _free = _process.lookupFunction<Void Function(Pointer<Void>),
    void Function(Pointer<Void>)>('free');

/// Zeroed allocations that live until the enclosing call returns.
final class Arena implements Allocator {
  final _allocations = <Pointer<Void>>[];

  @override
  Pointer<T> allocate<T extends NativeType>(int byteCount, {int? alignment}) {
    final pointer = _calloc(1, byteCount == 0 ? 1 : byteCount);
    _allocations.add(pointer);
    return pointer.cast();
  }

  @override
  void free(Pointer pointer) {}

  void releaseAll() {
    _allocations.forEach(_free);
    _allocations.clear();
  }
}

T withNativeArena<T>(T Function(Arena arena) body) {
  final arena = Arena();
  try {
    return body(arena);
  } finally {
    arena.releaseAll();
  }
}

void ensureAbiVersion() {}

final Pointer<raw.mln_diagnostic> nativeDiagnostic = () {
  final diagnostic = _calloc(1, sizeOf<raw.mln_diagnostic>()).cast<raw.mln_diagnostic>();
  diagnostic.ref.size = sizeOf<raw.mln_diagnostic>();
  return diagnostic;
}();

final class NativeFailure implements Exception {
  NativeFailure(this.status, this.message);
  final int status;
  final String message;
}

void _check(int status) {
  if (status == 0) {
    return;
  }
  final message = <int>[];
  for (var index = 0; nativeDiagnostic.ref.message[index] != 0; index++) {
    message.add(nativeDiagnostic.ref.message[index]);
  }
  throw NativeFailure(status, utf8.decode(message));
}

final class NativeStringView {
  NativeStringView(this.value);
  final raw.mln_buffer_view value;
}

NativeStringView nativeStringView(String value, Allocator allocator) {
  final bytes = utf8.encode(value);
  final data = allocator<Uint8>(bytes.isEmpty ? 1 : bytes.length);
  data.asTypedList(bytes.length).setAll(0, bytes);
  final view = allocator<raw.mln_buffer_view>();
  view.ref.data = data.cast();
  view.ref.size = bytes.length;
  return NativeStringView(view.ref);
}

Uint8List _copyBufferView(raw.mln_buffer_view view) => view.size == 0
    ? Uint8List(0)
    : Uint8List.fromList(view.data.cast<Uint8>().asTypedList(view.size));
