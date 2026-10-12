import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../c/maplibre_native_c.g.dart' as raw;
import '../status/status.dart';

T withNativeArena<T>(T Function(Arena arena) body) => using(body);

final class NativeUtf8CString {
  const NativeUtf8CString(this.pointer, this.byteLength);

  final Pointer<Utf8> pointer;

  final int byteLength;
}

NativeUtf8CString nativeUtf8CString(String value, Allocator allocator) {
  if (value.contains('\u0000')) {
    throwInvalidArgument(
      'null-terminated strings must not contain embedded NUL',
    );
  }

  final bytes = utf8.encode(value);
  final data = allocator<Uint8>(bytes.length + 1);
  data.asTypedList(bytes.length).setAll(0, bytes);
  data[bytes.length] = 0;

  return NativeUtf8CString(data.cast<Utf8>(), bytes.length);
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
