package org.maplibre.nativeffi.geo

/** GeoJSON feature identifier value. */
public sealed interface FeatureIdentifier {
  public data object Null : FeatureIdentifier

  /**
   * Unsigned feature identifier (`uint64_t` in the C ABI) stored as a [Long] bit pattern.
   *
   * Compare and format as unsigned by converting [value] with [Long.toULong].
   */
  public data class UInt(public val value: Long) : FeatureIdentifier

  /**
   * Signed feature identifier (`int64_t` in the C ABI). Name mirrors the C discriminant, not Kotlin
   * [kotlin.Int].
   */
  public data class Int(public val value: Long) : FeatureIdentifier

  public data class DoubleValue(public val value: Double) : FeatureIdentifier

  public data class StringValue(public val value: String) : FeatureIdentifier

  public class Unknown internal constructor(public val rawType: kotlin.Int) : FeatureIdentifier {
    override fun equals(other: Any?): Boolean = other is Unknown && rawType == other.rawType

    override fun hashCode(): kotlin.Int = rawType

    override fun toString(): String = "Unknown(rawType=$rawType)"
  }
}
