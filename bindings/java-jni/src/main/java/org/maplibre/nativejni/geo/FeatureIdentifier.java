package org.maplibre.nativejni.geo;

import java.util.Objects;

/** GeoJSON feature identifier value. */
public sealed interface FeatureIdentifier
    permits FeatureIdentifier.Null,
        FeatureIdentifier.UInt,
        FeatureIdentifier.Int,
        FeatureIdentifier.DoubleValue,
        FeatureIdentifier.StringValue {
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

  record UInt(long value) implements FeatureIdentifier {}

  record Int(long value) implements FeatureIdentifier {}

  record DoubleValue(double value) implements FeatureIdentifier {}

  record StringValue(String value) implements FeatureIdentifier {
    public StringValue {
      Objects.requireNonNull(value, "value");
    }
  }
}
