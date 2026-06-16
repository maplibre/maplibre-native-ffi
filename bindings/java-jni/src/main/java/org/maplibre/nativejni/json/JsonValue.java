package org.maplibre.nativejni.json;

import java.util.List;
import java.util.Objects;

/** Immutable JSON-like value tree used by Maplibre descriptors and copied results. */
public sealed interface JsonValue
    permits JsonValue.Null,
        JsonValue.Bool,
        JsonValue.UInt,
        JsonValue.Int,
        JsonValue.DoubleValue,
        JsonValue.StringValue,
        JsonValue.Array,
        JsonValue.ObjectValue {
  int MAX_DESCRIPTOR_DEPTH = 64;

  static Null nullValue() {
    return Null.INSTANCE;
  }

  static Bool of(boolean value) {
    return new Bool(value);
  }

  static UInt unsigned(long value) {
    return new UInt(value);
  }

  static Int of(long value) {
    return new Int(value);
  }

  static DoubleValue of(double value) {
    return new DoubleValue(value);
  }

  static StringValue of(String value) {
    return new StringValue(value);
  }

  static Array array(List<JsonValue> values) {
    return new Array(values);
  }

  static ObjectValue object(List<Member> members) {
    return new ObjectValue(members);
  }

  /** Singleton JSON null value. */
  final class Null implements JsonValue {
    public static final Null INSTANCE = new Null();

    private Null() {}

    @Override
    public boolean equals(Object other) {
      return other instanceof Null;
    }

    @Override
    public int hashCode() {
      return 0;
    }

    @Override
    public String toString() {
      return "JsonValue.Null";
    }
  }

  record Bool(boolean value) implements JsonValue {}

  /**
   * Unsigned 64-bit JSON integer stored in a Java {@code long} using the native bit pattern.
   *
   * <p>Use {@link Long#compareUnsigned(long, long)} and {@link Long#toUnsignedString(long)} when
   * interpreting this value as unsigned.
   */
  record UInt(long value) implements JsonValue {}

  record Int(long value) implements JsonValue {}

  record DoubleValue(double value) implements JsonValue {}

  record StringValue(String value) implements JsonValue {
    public StringValue {
      Objects.requireNonNull(value, "value");
    }
  }

  record Array(List<JsonValue> values) implements JsonValue {
    public Array {
      values = List.copyOf(values);
    }
  }

  record ObjectValue(List<Member> members) implements JsonValue {
    public ObjectValue {
      members = List.copyOf(members);
    }
  }

  /** Ordered JSON object member. Duplicate keys are preserved. */
  record Member(String key, JsonValue value) {
    public Member {
      Objects.requireNonNull(key, "key");
      Objects.requireNonNull(value, "value");
    }
  }
}
