package org.maplibre.nativeffi.geo

/** GeoJSON feature identifier value. */
public sealed interface FeatureIdentifier {
  public data object Null : FeatureIdentifier

  /** Unsigned feature identifier (`uint64_t` in the C ABI) stored as a [Long] bit pattern. */
  public data class UInt(public val value: Long) : FeatureIdentifier

  /**
   * Signed feature identifier (`int64_t` in the C ABI). Name mirrors the C discriminant, not Kotlin
   * [kotlin.Int].
   */
  public data class Int(public val value: Long) : FeatureIdentifier

  public data class DoubleValue(public val value: Double) : FeatureIdentifier

  public data class StringValue(public val value: String) : FeatureIdentifier

  public companion object {
    public fun nullValue(): Null = Null

    public fun unsigned(value: Long): UInt = UInt(value)

    public fun of(value: Long): Int = Int(value)

    public fun of(value: Double): DoubleValue = DoubleValue(value)

    public fun of(value: String): StringValue = StringValue(value)
  }
}
