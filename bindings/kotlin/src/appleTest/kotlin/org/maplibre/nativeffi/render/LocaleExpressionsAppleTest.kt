package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.withMap

/** Locale-sensitive style expressions, which Apple platforms evaluate with Foundation. */
class LocaleExpressionsAppleTest {
  @Test
  fun formattingRetainsDefaultsAndHonorsExplicitFractionLimits(): Unit = runSuspendTest {
    val cases =
      listOf(
        "" to "12.3",
        "\"currency\":\"USD\"" to "$12.30",
        "\"currency\":\"USD\",\"min-fraction-digits\":0,\"max-fraction-digits\":3" to "$12.3",
        "\"currency\":\"USD\",\"min-fraction-digits\":3,\"max-fraction-digits\":3" to "$12.300",
        "\"currency\":\"USD\",\"min-fraction-digits\":4" to "$12.3000",
        "\"currency\":\"USD\",\"max-fraction-digits\":0" to "$12",
        "\"min-fraction-digits\":4" to "12.3000",
        "\"max-fraction-digits\":0" to "12",
        "\"min-fraction-digits\":2,\"max-fraction-digits\":4" to "12.30",
      )
    withMap {
      loadStyle("""{"version":8,"sources":{},"layers":[{"id":"background","type":"background"}]}""")
      for ((options, expected) in cases) {
        val format = if (options.isEmpty()) "" else ",$options"
        map
          .setLayerProperty(
            "background",
            "background-opacity",
            """["case",["==",["number-format",12.3,{"locale":"en-US"$format}],"$expected"],0.25,0.75]"""
              .encodeToByteArray(),
          )
          .awaitCommitted()
        assertEquals(
          "0.25",
          map
            .getLayerProperty("background", "background-opacity")
            .awaitWithin("the layer property")
            ?.decodeToString(),
          options,
        )
      }
    }
  }

  @Test
  fun collationRejectsMalformedUTF8AndPreservesEmbeddedNulls(): Unit = runSuspendTest {
    val collator =
      """["collator",{"locale":"en-US","case-sensitive":false,"diacritic-sensitive":false}]"""
    assertMatches(
      """["==",["get","name"],"cafe",$collator]""",
      listOf("cafe", "café", INVALID_UTF8),
      setOf(0, 1),
    )
    assertMatches("""["==",["get","name"],"$INVALID_UTF8",$collator]""", listOf("cafe"), emptySet())
    assertMatches(
      """["==",["get","name"],"a\u0000b",$collator]""",
      listOf("""a\u0000b""", """a\u0000c"""),
      setOf(0),
    )
  }

  private suspend fun assertMatches(filter: String, names: List<String>, expected: Set<Int>) {
    assertFilterSelects(filter, names.map { """{"name":"$it"}""" }, expected, ::withInvalidUtf8)
  }

  /** Encodes [json], replacing each [INVALID_UTF8] marker with a byte that is never valid UTF-8. */
  private fun withInvalidUtf8(json: String): ByteArray =
    json
      .trimIndent()
      .split(INVALID_UTF8)
      .map { it.encodeToByteArray() }
      .reduce { encoded, next -> encoded + byteArrayOf(0xFF.toByte()) + next }

  private companion object {
    const val INVALID_UTF8 = "__INVALID_UTF8__"
  }
}
