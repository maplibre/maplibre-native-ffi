import 'dart:convert';
import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../c/maplibre_native_c.g.dart' as raw;
import '../status/status.dart';

/// Runs [body] with native allocations that are released when [body] returns.
T withNativeArena<T>(T Function(Arena arena) body) => using(body);

/// Native UTF-8 string storage for a null-terminated C string argument.
final class NativeUtf8CString {
  const NativeUtf8CString(this.pointer, this.byteLength);

  /// Null-terminated UTF-8 storage.
  final Pointer<Utf8> pointer;

  /// UTF-8 byte length excluding the trailing NUL byte.
  final int byteLength;
}

/// Encodes [value] as a null-terminated UTF-8 string for one native call.
NativeUtf8CString nativeUtf8CString(String value, Allocator allocator) {
  if (value.contains('\u0000')) {
    throwInvalidArgument(
      'null-terminated strings must not contain embedded NUL',
    );
  }

  final bytes = utf8.encode(value);
  final data = allocator<Uint8>(bytes.length + 1);
  for (var index = 0; index < bytes.length; index += 1) {
    data[index] = bytes[index];
  }
  data[bytes.length] = 0;

  return NativeUtf8CString(data.cast<Utf8>(), bytes.length);
}

/// Native UTF-8 storage for an explicit-length C string view argument.
final class NativeStringView {
  const NativeStringView(this.value, this.byteLength);

  /// Native string view struct.
  final raw.mln_string_view value;

  /// UTF-8 byte length in [value].
  final int byteLength;
}

/// Encodes [value] as an explicit-length UTF-8 string view for one native call.
NativeStringView nativeStringView(String value, Allocator allocator) {
  final bytes = utf8.encode(value);
  final data = bytes.isEmpty
      ? nullptr.cast<Uint8>()
      : allocator<Uint8>(bytes.length);
  for (var index = 0; index < bytes.length; index += 1) {
    data[index] = bytes[index];
  }

  final view = allocator<raw.mln_string_view>();
  view.ref.data = data.cast<Char>();
  view.ref.size = bytes.length;
  return NativeStringView(view.ref, bytes.length);
}
