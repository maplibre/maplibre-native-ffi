import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../json/json.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../memory/memory.dart';
import '../status/status.dart';

/// Maximum nested JSON descriptor depth accepted by the binding.
const int maxJsonDescriptorDepth = 64;

final BigInt _uint64Modulus = BigInt.one << 64;
final BigInt _maxUnsignedInt64 = _uint64Modulus - BigInt.one;
final BigInt _maxSignedInt64 = (BigInt.one << 63) - BigInt.one;

/// Call-scoped native JSON descriptor.
final class NativeJsonValue {
  const NativeJsonValue(this.pointer);

  /// Pointer to the root native JSON descriptor.
  final Pointer<raw.mln_json_value> pointer;
}

/// Materializes [value] into arena-owned native JSON descriptor storage.
NativeJsonValue nativeJsonValue(JsonValue value, Allocator allocator) {
  final pointer = allocator<raw.mln_json_value>();
  _writeJsonValue(pointer, value, allocator, 0);
  return NativeJsonValue(pointer);
}

void _writeJsonValue(
  Pointer<raw.mln_json_value> out,
  JsonValue value,
  Allocator allocator,
  int depth,
) {
  _checkJsonDepth(depth);
  out.ref.size = sizeOf<raw.mln_json_value>();

  switch (value) {
    case JsonNull():
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_NULL.value;
    case JsonBool(:final value):
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_BOOL.value;
      out.ref.data.bool_value = value;
    case JsonUInt(:final value):
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_UINT.value;
      out.ref.data.uint_value = _uint64ToNative(
        value,
        'JSON unsigned integer values',
      );
    case JsonInt(:final value):
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_INT.value;
      out.ref.data.int_value = value;
    case JsonDouble(:final value):
      if (!value.isFinite) {
        throwInvalidArgument('JSON double values must be finite');
      }
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_DOUBLE.value;
      out.ref.data.double_value = value;
    case JsonString(:final value):
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_STRING.value;
      out.ref.data.string_value = nativeStringView(value, allocator).value;
    case JsonArray(:final values):
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_ARRAY.value;
      final rawValues = values.isEmpty
          ? nullptr.cast<raw.mln_json_value>()
          : allocator<raw.mln_json_value>(values.length);
      for (var index = 0; index < values.length; index += 1) {
        _writeJsonValue(rawValues + index, values[index], allocator, depth + 1);
      }
      final array = Struct.create<raw.mln_json_array>();
      array.values = rawValues;
      array.value_count = values.length;
      out.ref.data.array_value = array;
    case JsonObject(:final members):
      out.ref.type = raw.mln_json_value_type.MLN_JSON_VALUE_TYPE_OBJECT.value;
      final rawMembers = members.isEmpty
          ? nullptr.cast<raw.mln_json_member>()
          : allocator<raw.mln_json_member>(members.length);
      for (var index = 0; index < members.length; index += 1) {
        final member = members[index];
        rawMembers[index].key = nativeStringView(member.key, allocator).value;
        final memberValue = allocator<raw.mln_json_value>();
        _writeJsonValue(memberValue, member.value, allocator, depth + 1);
        rawMembers[index].value = memberValue;
      }
      final object = Struct.create<raw.mln_json_object>();
      object.members = rawMembers;
      object.member_count = members.length;
      out.ref.data.object_value = object;
  }
}

/// Copies a borrowed native JSON descriptor into an owned Dart value.
JsonValue jsonValueFromNative(raw.mln_json_value value) =>
    _jsonValueFromNative(value, 0);

JsonValue _jsonValueFromNative(raw.mln_json_value value, int depth) {
  _checkJsonDepth(depth);
  switch (value.type) {
    case 0:
      return const JsonNull();
    case 1:
      return JsonBool(value.data.bool_value);
    case 2:
      return JsonUInt.fromBigInt(_uint64FromNative(value.data.uint_value));
    case 3:
      return JsonInt(value.data.int_value);
    case 4:
      return JsonDouble(value.data.double_value);
    case 5:
      return JsonString(_copyStringView(value.data.string_value));
    case 6:
      final array = value.data.array_value;
      return JsonArray([
        for (var index = 0; index < array.value_count; index += 1)
          _jsonValueFromNative(array.values[index], depth + 1),
      ]);
    case 7:
      final object = value.data.object_value;
      return JsonObject([
        for (var index = 0; index < object.member_count; index += 1)
          JsonMember(
            _copyStringView(object.members[index].key),
            _jsonValueFromNative(object.members[index].value.ref, depth + 1),
          ),
      ]);
    default:
      throwInvalidArgument('unknown native JSON value type: ${value.type}');
  }
}

String _copyStringView(raw.mln_string_view view) {
  if (view.data == nullptr || view.size == 0) {
    return '';
  }
  return view.data.cast<Utf8>().toDartString(length: view.size);
}

int _uint64ToNative(BigInt value, String name) {
  if (value < BigInt.zero || value > _maxUnsignedInt64) {
    throwInvalidArgument('$name must be between 0 and $_maxUnsignedInt64');
  }
  final signedBits = value > _maxSignedInt64 ? value - _uint64Modulus : value;
  return signedBits.toInt();
}

BigInt _uint64FromNative(int value) {
  final bits = BigInt.from(value);
  return value < 0 ? bits + _uint64Modulus : bits;
}

void _checkJsonDepth(int depth) {
  if (depth > maxJsonDescriptorDepth) {
    throwInvalidArgument(
      'JSON descriptors may not exceed depth $maxJsonDescriptorDepth',
    );
  }
}
