/// The public value types of the C API.
///
/// `generated_values.dart` declares each value, enum, and callback type; this
/// library supplies the equality and descriptions they share.
library;

import 'dart:typed_data';

import 'internal/callback/native_registration.dart';
import 'render/native_pointer.dart';
import 'runtime/runtime.dart';

part 'generated_values.dart';

/// The default of a `copyWith` parameter that takes any JSON value, where null
/// is a value of its own: it means the member is absent.
const Object _unchanged = _Unchanged();

final class _Unchanged {
  const _Unchanged();
}

/// A value whose equality compares its members, and lists element by element.
abstract base class _Value {
  const _Value();

  /// The members that equality and hashing compare, in declaration order.
  List<Object?> get _members;

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other.runtimeType == runtimeType &&
          _valueEquals((other as _Value)._members, _members);

  @override
  int get hashCode => _valueHash(_members);

  /// Describes this value as `type(name=member, ...)`, pairing [names] with
  /// [_members]. Byte members print only their length.
  String _describe(String type, List<String> names) {
    final members = _members;
    final described = [
      for (var index = 0; index < names.length; index++)
        '${names[index]}=${switch (members[index]) {
          final Uint8List bytes => 'Uint8List(${bytes.length} bytes)',
          final member => member,
        }}',
    ];
    return '$type(${described.join(', ')})';
  }
}

/// A C enum value, which equals another of its type with the same raw value.
abstract base class _Enum {
  const _Enum(this.rawValue);

  /// The value's C representation, which may be one this binding predates.
  final int rawValue;

  @override
  bool operator ==(Object other) =>
      other.runtimeType == runtimeType && (other as _Enum).rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// A C bitmask enum value, which combines with others of its type.
abstract base class _Flags<T extends _Flags<T>> extends _Enum {
  const _Flags(super.rawValue);

  T _of(int rawValue);

  T operator |(T other) => _of(rawValue | other.rawValue);

  T operator &(T other) => _of(rawValue & other.rawValue);

  /// Whether every bit of [other] is set in this value.
  bool contains(T other) => (rawValue & other.rawValue) == other.rawValue;

  /// Describes this value as the named members whose bits it sets, joined by
  /// `|`, then any unnamed bits in hex.
  String _describe(String type, Map<int, String> names) {
    final exact = names[rawValue];
    if (exact != null) return '$type.$exact';
    final described = <String>[];
    var rest = rawValue;
    for (final MapEntry(key: bits, value: name) in names.entries) {
      if (bits != 0 && rest & bits == bits) {
        described.add('$type.$name');
        rest &= ~bits;
      }
    }
    if (rest != 0) described.add('0x${rest.toUnsigned(64).toRadixString(16)}');
    return described.join(' | ');
  }
}

/// Compares lists element by element and maps, such as JSON objects, entry by
/// entry.
bool _valueEquals(Object? left, Object? right) {
  if (left is List && right is List) {
    if (left.length != right.length) {
      return false;
    }
    for (var index = 0; index < left.length; index++) {
      if (!_valueEquals(left[index], right[index])) {
        return false;
      }
    }
    return true;
  }
  if (left is Map && right is Map) {
    if (left.length != right.length) {
      return false;
    }
    for (final MapEntry(:key, :value) in left.entries) {
      if (!right.containsKey(key) || !_valueEquals(value, right[key])) {
        return false;
      }
    }
    return true;
  }
  return left == right;
}

int _valueHash(Object? value) => switch (value) {
  final List<Object?> list => Object.hashAll(list.map(_valueHash)),
  final Map<Object?, Object?> map => Object.hashAllUnordered(
    map.entries.map((entry) => Object.hash(entry.key, _valueHash(entry.value))),
  ),
  _ => value.hashCode,
};
