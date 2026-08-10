import 'dart:typed_data';

/// Copies bytes into read-only binding-owned storage.
Uint8List copyBytes(Uint8List value) =>
    Uint8List.fromList(value).asUnmodifiableView();

/// Copies optional bytes into read-only binding-owned storage.
Uint8List? copyOptionalBytes(Uint8List? value) =>
    value == null ? null : copyBytes(value);

/// Compares optional byte values by content.
bool optionalBytesEqual(Uint8List? left, Uint8List? right) {
  if (identical(left, right)) return true;
  if (left == null || right == null || left.length != right.length) {
    return false;
  }
  for (var index = 0; index < left.length; index += 1) {
    if (left[index] != right[index]) {
      return false;
    }
  }
  return true;
}

/// Hashes optional byte values by content.
int optionalBytesHash(Uint8List? value) => Object.hashAll(value ?? const []);
