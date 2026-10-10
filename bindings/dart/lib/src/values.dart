/// The public value types of the C API.
///
/// `generated_values.dart` declares each value, enum, and callback type; this
/// library supplies the equality they share.
library;

import 'dart:typed_data';

import 'render/native_pointer.dart';
import 'runtime/runtime.dart';

part 'generated_values.dart';

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
}

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
  return left == right;
}

int _valueHash(Object? value) =>
    value is List ? Object.hashAll(value.map(_valueHash)) : value.hashCode;
