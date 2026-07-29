/// JSON value types used by style, query, feature state, and descriptors.
library;

/// Ordered JSON object member. Duplicate keys are preserved.
final class JsonMember {
  /// Creates a JSON object member.
  const JsonMember(this.key, this.value);

  /// Member key.
  final String key;

  /// Member value.
  final JsonValue value;
}

/// Owned JSON-like value tree used by style, GeoJSON, and copied native values.
sealed class JsonValue {
  const JsonValue();
}

/// JSON null.
final class JsonNull extends JsonValue {
  /// Creates JSON null.
  const JsonNull();
}

/// JSON boolean.
final class JsonBool extends JsonValue {
  /// Creates a JSON boolean.
  const JsonBool(this.value);

  /// Boolean value.
  final bool value;
}

/// JSON unsigned integer.
final class JsonUInt extends JsonValue {
  /// Creates a JSON unsigned integer from a non-negative Dart [int].
  JsonUInt(int value) : value = BigInt.from(value);

  /// Creates a JSON unsigned integer across the full `uint64_t` domain.
  JsonUInt.fromBigInt(this.value);

  /// Unsigned value, represented as [BigInt] so all 64 bits are preserved.
  ///
  /// Use [BigInt.compareTo] for ordering and [BigInt.toString] or
  /// [BigInt.toRadixString] for formatting.
  final BigInt value;
}

/// JSON signed integer.
final class JsonInt extends JsonValue {
  /// Creates a JSON signed integer.
  const JsonInt(this.value);

  /// Integer value.
  final int value;
}

/// JSON double.
final class JsonDouble extends JsonValue {
  /// Creates a JSON double.
  const JsonDouble(this.value);

  /// Double value.
  final double value;
}

/// JSON string.
final class JsonString extends JsonValue {
  /// Creates a JSON string.
  const JsonString(this.value);

  /// String value.
  final String value;
}

/// JSON array.
final class JsonArray extends JsonValue {
  /// Creates a JSON array.
  JsonArray(List<JsonValue> values) : values = List.unmodifiable(values);

  /// Array values.
  final List<JsonValue> values;
}

/// JSON object.
final class JsonObject extends JsonValue {
  /// Creates a JSON object.
  JsonObject(List<JsonMember> members) : members = List.unmodifiable(members);

  /// Ordered object members.
  final List<JsonMember> members;
}
