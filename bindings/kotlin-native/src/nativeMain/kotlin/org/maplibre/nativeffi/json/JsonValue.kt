package org.maplibre.nativeffi.json

/** Immutable JSON-like value tree used by Maplibre descriptors and copied results. */
public sealed interface JsonValue {
  public data object Null : JsonValue

  public data class Bool(public val value: Boolean) : JsonValue

  /** Unsigned JSON integer (`uint64_t` in the C ABI) stored as a non-negative [Long]. */
  public data class UInt(public val value: Long) : JsonValue {
    init {
      require(value >= 0) { "unsigned JSON value must be non-negative" }
    }
  }

  /**
   * Signed JSON integer (`int64_t` in the C ABI). Name mirrors the C discriminant, not Kotlin
   * [kotlin.Int].
   */
  public data class Int(public val value: Long) : JsonValue

  public data class DoubleValue(public val value: Double) : JsonValue

  public data class StringValue(public val value: String) : JsonValue

  public data class Array(public val values: List<JsonValue>) : JsonValue {
    public constructor(vararg values: JsonValue) : this(values.toList())
  }

  public data class ObjectValue(public val members: List<Member>) : JsonValue {
    public constructor(vararg members: Member) : this(members.toList())
  }

  /** Ordered JSON object member. Duplicate keys are preserved. */
  public data class Member(public val key: String, public val value: JsonValue)

  public companion object {
    public const val MAX_DESCRIPTOR_DEPTH: kotlin.Int = 64

    public fun nullValue(): Null = Null

    public fun of(value: Boolean): Bool = Bool(value)

    public fun unsigned(value: Long): UInt = UInt(value)

    public fun of(value: Long): Int = Int(value)

    public fun of(value: Double): DoubleValue = DoubleValue(value)

    public fun of(value: String): StringValue = StringValue(value)

    public fun array(values: List<JsonValue>): Array = Array(values)

    public fun `object`(members: List<Member>): ObjectValue = ObjectValue(members)
  }
}
