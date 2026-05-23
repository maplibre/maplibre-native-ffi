package org.maplibre.nativeffi.geo

/** GeoJSON feature identifier value. */
public sealed interface FeatureIdentifier {
  public data object Null : FeatureIdentifier

  public data class UInt(public val value: ULong) : FeatureIdentifier

  public data class IntValue(public val value: Long) : FeatureIdentifier

  public data class DoubleValue(public val value: Double) : FeatureIdentifier

  public data class StringValue(public val value: String) : FeatureIdentifier

  public companion object {
    public fun nullValue(): Null = Null

    public fun unsigned(value: ULong): UInt = UInt(value)

    public fun of(value: Long): IntValue = IntValue(value)

    public fun of(value: Double): DoubleValue = DoubleValue(value)

    public fun of(value: String): StringValue = StringValue(value)
  }
}
