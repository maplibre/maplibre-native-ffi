package org.maplibre.nativeffi.json

import kotlin.test.Test
import kotlin.test.assertEquals

class JsonValueTest {
  @Test
  fun objectValuesPreserveMemberOrderAndDuplicateKeys() {
    val value =
      JsonValue.`object`(
        listOf(
          JsonValue.Member("name", JsonValue.of("first")),
          JsonValue.Member("name", JsonValue.of("second")),
        )
      )

    assertEquals(listOf("name", "name"), value.members.map { it.key })
    assertEquals(JsonValue.StringValue("first"), value.members[0].value)
    assertEquals(JsonValue.StringValue("second"), value.members[1].value)
    assertEquals(value, JsonValue.`object`(value.members))
  }
}
