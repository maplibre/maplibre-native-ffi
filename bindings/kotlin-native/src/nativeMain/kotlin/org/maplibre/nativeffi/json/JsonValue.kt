package org.maplibre.nativeffi.json

/** Immutable JSON-like value tree used by Maplibre descriptors and copied results. */
public sealed interface JsonValue {
  public data object Null : JsonValue

  public data class Bool(public val value: Boolean) : JsonValue

  public data class UInt(public val value: ULong) : JsonValue

  public data class IntValue(public val value: Long) : JsonValue

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
    public const val MAX_DESCRIPTOR_DEPTH: Int = 64

    public fun nullValue(): Null = Null

    public fun of(value: Boolean): Bool = Bool(value)

    public fun unsigned(value: ULong): UInt = UInt(value)

    public fun of(value: Long): IntValue = IntValue(value)

    public fun of(value: Double): DoubleValue = DoubleValue(value)

    public fun of(value: String): StringValue = StringValue(value)

    public fun array(values: List<JsonValue>): Array = Array(values)

    public fun obj(members: List<Member>): ObjectValue = ObjectValue(members)

    public fun `object`(members: List<Member>): ObjectValue = obj(members)
  }
}
