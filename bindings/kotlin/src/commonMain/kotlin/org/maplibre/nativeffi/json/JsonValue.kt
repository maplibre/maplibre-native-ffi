package org.maplibre.nativeffi.json

/** Immutable JSON-like value tree used by Maplibre descriptors and copied results. */
public sealed interface JsonValue {
  public data object Null : JsonValue

  public data class Bool(public val value: Boolean) : JsonValue

  /**
   * Unsigned JSON integer (`uint64_t` in the C ABI) stored as a [Long] bit pattern.
   *
   * Compare and format as unsigned by converting [value] with [Long.toULong].
   */
  public data class UInt(public val value: Long) : JsonValue

  /**
   * Signed JSON integer (`int64_t` in the C ABI). Name mirrors the C discriminant, not Kotlin
   * [kotlin.Int].
   */
  public data class Int(public val value: Long) : JsonValue

  public data class DoubleValue(public val value: Double) : JsonValue

  public data class StringValue(public val value: String) : JsonValue

  public class Array(values: List<JsonValue>) : JsonValue {
    public val values: List<JsonValue> = values.toList()

    override fun equals(other: Any?): Boolean = other is Array && values == other.values

    override fun hashCode(): kotlin.Int = values.hashCode()

    override fun toString(): String = "Array(values=$values)"
  }

  public class ObjectValue(members: List<Member>) : JsonValue {
    public val members: List<Member> = members.toList()

    override fun equals(other: Any?): Boolean = other is ObjectValue && members == other.members

    override fun hashCode(): kotlin.Int = members.hashCode()

    override fun toString(): String = "ObjectValue(members=$members)"
  }

  public class Unknown
  internal constructor(public val rawType: kotlin.Int, public val rawSize: kotlin.Int) : JsonValue {
    override fun equals(other: Any?): Boolean =
      other is Unknown && rawType == other.rawType && rawSize == other.rawSize

    override fun hashCode(): kotlin.Int {
      var result = rawType
      result = 31 * result + rawSize
      return result
    }

    override fun toString(): String = "Unknown(rawType=$rawType, rawSize=$rawSize)"
  }

  /** Ordered JSON object member. Duplicate keys are preserved. */
  public data class Member(public val key: String, public val value: JsonValue)

  public companion object {
    public const val MAX_DESCRIPTOR_DEPTH: kotlin.Int = 64
  }
}
