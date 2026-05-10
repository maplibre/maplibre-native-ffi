package org.maplibre.nativeffi;

import java.math.BigInteger;
import java.util.Objects;

/** GeoJSON feature identifier value. */
public sealed interface FeatureIdentifier
    permits FeatureIdentifier.Null,
        FeatureIdentifier.UInt,
        FeatureIdentifier.Int,
        FeatureIdentifier.DoubleValue,
        FeatureIdentifier.StringValue {
  BigInteger MAX_UINT64 = BigInteger.ONE.shiftLeft(64).subtract(BigInteger.ONE);

  static Null nullValue() {
    return Null.INSTANCE;
  }

  static UInt unsigned(long value) {
    return new UInt(new BigInteger(Long.toUnsignedString(value)));
  }

  static UInt unsigned(BigInteger value) {
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

  record UInt(BigInteger value) implements FeatureIdentifier {
    public UInt {
      Objects.requireNonNull(value, "value");
      if (value.signum() < 0 || value.compareTo(MAX_UINT64) > 0) {
        throw new IllegalArgumentException("unsigned feature identifier must be in [0, 2^64 - 1]");
      }
    }
  }

  record Int(long value) implements FeatureIdentifier {}

  record DoubleValue(double value) implements FeatureIdentifier {
    public DoubleValue {
      if (!Double.isFinite(value)) {
        throw new IllegalArgumentException("feature identifier double must be finite");
      }
    }
  }

  record StringValue(String value) implements FeatureIdentifier {
    public StringValue {
      Objects.requireNonNull(value, "value");
    }
  }
}
