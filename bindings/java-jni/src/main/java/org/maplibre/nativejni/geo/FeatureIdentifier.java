package org.maplibre.nativejni.geo;

import java.util.Objects;

/** GeoJSON feature identifier value. */
public sealed interface FeatureIdentifier
    permits FeatureIdentifier.Null,
        FeatureIdentifier.UInt,
        FeatureIdentifier.Int,
        FeatureIdentifier.DoubleValue,
        FeatureIdentifier.StringValue,
        FeatureIdentifier.Unknown {
  static Null nullValue() {
    return Null.INSTANCE;
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

  static Unknown unknown(int rawType) {
    return new Unknown(rawType);
  }

  /** Singleton null identifier. */
  final class Null implements FeatureIdentifier {
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
      return "FeatureIdentifier.Null";
    }
  }

  /**
   * Unsigned 64-bit feature identifier stored in a Java {@code long} using the native bit pattern.
   *
   * <p>Use {@link Long#compareUnsigned(long, long)} and {@link Long#toUnsignedString(long)} when
   * interpreting this value as unsigned.
   */
  record UInt(long value) implements FeatureIdentifier {}

  record Int(long value) implements FeatureIdentifier {}

  record DoubleValue(double value) implements FeatureIdentifier {}

  record StringValue(String value) implements FeatureIdentifier {
    public StringValue {
      Objects.requireNonNull(value, "value");
    }
  }

  /** Feature identifier with an unknown native type tag returned by a newer C API. */
  record Unknown(int rawType) implements FeatureIdentifier {}
}
