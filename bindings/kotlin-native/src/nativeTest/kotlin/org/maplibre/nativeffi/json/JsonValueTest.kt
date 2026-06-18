package org.maplibre.nativeffi.json

import kotlin.test.Test
import kotlin.test.assertEquals

class JsonValueTest {
  // BND-067: Kotlin stores uint64 JSON values in a signed carrier with documented bit patterns.

  @Test
  fun unsignedValuesUseDocumentedLongBitPatternCarrier() {
    val value = JsonValue.UInt(-1L)

    assertEquals(JsonValue.UInt(-1L), value)
    assertEquals(ULong.MAX_VALUE, value.value.toULong())
  }

  // BND-067: structured JSON keeps object member order and repeated member names.

  @Test
  fun objectValuesPreserveMemberOrderAndDuplicateKeys() {
    val value =
      JsonValue.ObjectValue(
        listOf(
          JsonValue.Member("name", JsonValue.StringValue("first")),
          JsonValue.Member("name", JsonValue.StringValue("second")),
        )
      )

    assertEquals(listOf("name", "name"), value.members.map { it.key })
    assertEquals(JsonValue.StringValue("first"), value.members[0].value)
    assertEquals(JsonValue.StringValue("second"), value.members[1].value)
    assertEquals(value, JsonValue.ObjectValue(value.members))
  }
}
