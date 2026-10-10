package org.maplibre.nativeffi.render

import java.util.Locale
import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** Locale-sensitive style expressions, which Android evaluates with its platform services. */
class LocaleExpressionsAndroidTest {
  @Test
  fun formattingUsesTheRequestedLocaleCurrencyAndFractionLimits(): Unit = runSuspendTest {
    assertCases(
      """["==", ["number-format", ["get","amount"], {
        "locale": ["get","locale"], "currency": ["get","currency"],
        "min-fraction-digits": ["get","min"], "max-fraction-digits": ["get","max"]
      }], ["get","expected"]]""",
      """{"amount":1234.5,"locale":"de-DE","currency":"","min":2,"max":2,"expected":"1.234,50"}""",
      """{"amount":1234.5,"locale":"en-US","currency":"EUR","min":2,"max":2,"expected":"€1,234.50"}""",
      """{"amount":1234.5678,"locale":"en-US","currency":"USD","min":0,"max":1,"expected":"$1,234.6"}""",
      """{"amount":12.3,"locale":"en-US","currency":"USD","min":3,"max":3,"expected":"$12.300"}""",
      """{"amount":12.3,"locale":"en-US","currency":"","min":2,"max":4,"expected":"12.30"}""",
    )
  }

  @Test
  fun omittedFractionLimitsRetainDecimalAndCurrencyDefaults(): Unit = runSuspendTest {
    assertCases(
      """["==", ["number-format", ["get","amount"], {
        "locale":"en-US", "currency":["get","currency"]
      }], ["get","expected"]]""",
      """{"amount":1234.5,"currency":"","expected":"1,234.5"}""",
      """{"amount":1234.5,"currency":"USD","expected":"$1,234.50"}""",
      """{"amount":1234.56,"currency":"JPY","expected":"¥1,235"}""",
    )
    assertCases(
      """["==",["slice",["number-format",["get","amount"],{
        "locale":"en-US","currency":"KWD"
      }],-4],".500"]""",
      """{"amount":1234.5}""",
    )
    assertCases(
      """["==", ["number-format", ["get","amount"], {
        "locale":"en-US", "currency":["get","currency"], "min-fraction-digits":4
      }], ["get","expected"]]""",
      """{"amount":12.3,"currency":"","expected":"12.3000"}""",
      """{"amount":12.3,"currency":"USD","expected":"$12.3000"}""",
    )
    assertCases(
      """["==", ["number-format", ["get","amount"], {
        "locale":"en-US", "currency":"USD", "max-fraction-digits":0
      }], "\u002412"]""",
      """{"amount":12.3}""",
    )
  }

  @Test
  fun currencyApiFailuresRemainExpressionErrorsAndLaterCallsSucceed(): Unit = runSuspendTest {
    assertCases(
      """["==",["number-format",["get","amount"],{
        "locale":"en-US","currency":["get","currency"]
      }],"$12.30"]""",
      """{"amount":12.3,"currency":"invalid"}""",
      """{"amount":12.3,"currency":"USD"}""",
      expectedIndices = setOf(1),
    )
  }

  @Test
  fun constantFormattingIsEvaluatedWhileParsingTheStyle(): Unit = runSuspendTest {
    withMap {
      loadStyle(
        """{"version":8,"sources":{},"layers":[{
          "id":"background","type":"background","paint":{
            "background-opacity":["case",
              ["==",["number-format",1234.5,{"locale":"de-DE","min-fraction-digits":2,"max-fraction-digits":2}],"1.234,50"],
              0.25,0.75]
          }
        }]}"""
      )
      assertEquals(
        "0.25",
        map
          .getStyleLayerProperty("background", "background-opacity")
          .awaitWithin("the layer property")
          ?.decodeToString(),
      )
    }
  }

  @Test
  fun collationHonorsLocaleAndEverySensitivityCombination(): Unit = runSuspendTest {
    val cases = mutableListOf<String>()
    for (caseSensitive in listOf(false, true)) {
      for (accentSensitive in listOf(false, true)) {
        cases +=
          """{"lhs":"e","rhs":"E","case":$caseSensitive,"accent":$accentSensitive,"equal":${!caseSensitive}}"""
        cases +=
          """{"lhs":"e","rhs":"é","case":$caseSensitive,"accent":$accentSensitive,"equal":${!accentSensitive}}"""
        cases +=
          """{"lhs":"é","rhs":"É","case":$caseSensitive,"accent":$accentSensitive,"equal":${!caseSensitive}}"""
        cases +=
          """{"lhs":"é","rhs":"e\u0301","case":$caseSensitive,"accent":$accentSensitive,"equal":true}"""
        cases +=
          """{"lhs":"😀\u0000a","rhs":"😀\u0000b","case":$caseSensitive,"accent":$accentSensitive,"equal":false}"""
      }
    }
    assertCases(
      """["==", ["==",["get","lhs"],["get","rhs"], ["collator",{
        "locale":"en-US", "case-sensitive":["get","case"], "diacritic-sensitive":["get","accent"]
      }]], ["get","equal"]]""",
      *cases.toTypedArray(),
    )
    assertCases(
      """["==", ["<",["get","lhs"],["get","rhs"], ["collator",{
        "locale":["get","locale"],"case-sensitive":true,"diacritic-sensitive":false
      }]], ["get","less"]]""",
      """{"lhs":"z","rhs":"ä","locale":"sv-SE","less":true}""",
      """{"lhs":"z","rhs":"ä","locale":"de-DE","less":false}""",
    )
    assertCases(
      """["==",["get","lhs"],["get","rhs"],["collator",{"locale":"tr-TR"}]]""",
      """{"lhs":"I","rhs":"ı"}""",
      """{"lhs":"i","rhs":"İ"}""",
    )
  }

  @Test
  fun collationExtensionsUsePlatformTailoring(): Unit = runSuspendTest {
    assertCases(
      """["==",["==",["get","lhs"],["get","rhs"],["collator",{
        "locale":["get","locale"]
      }]],["get","equal"]]""",
      """{"lhs":"ä","rhs":"ae","locale":"de-DE-u-co-phonebk","equal":true}""",
      """{"lhs":"ä","rhs":"ae","locale":"de-DE","equal":false}""",
    )
    assertCases(
      """["==",["<",["get","lhs"],["get","rhs"],["collator",{
        "locale":["get","locale"]
      }]],["get","less"]]""",
      """{"lhs":"2","rhs":"10","locale":"en-US-u-kn-true","less":true}""",
      """{"lhs":"2","rhs":"10","locale":"en-US","less":false}""",
    )
    assertCases(
      """["in",["resolved-locale",["collator",{"locale":["get","locale"]}]],
        ["get","resolved"]]""",
      """{"locale":"de-DE-u-co-phonebk","resolved":["de-u-co-phonebk","de-DE-u-co-phonebk"]}""",
      """{"locale":"de-DE-u-co-foobar","resolved":["de","de-DE"]}""",
    )
  }

  @Test
  fun resolvedLocaleReportsTheSelectedLocaleAndSystemFallback(): Unit = runSuspendTest {
    val previous = Locale.getDefault()
    try {
      Locale.setDefault(Locale.US)
      assertCases(
        """["==", ["slice", ["resolved-locale", ["collator",{
          "locale":["get","locale"]
        }]], 0, 2], ["get","language"]]""",
        """{"locale":"sv-SE","language":"sv"}""",
        """{"locale":"tr-TR","language":"tr"}""",
        """{"locale":"xx-ZZ","language":"en"}""",
      )
      assertCases("""["==",["slice",["resolved-locale",["collator",{}]],0,2],"en"]""", """{}""")
    } finally {
      Locale.setDefault(previous)
    }
  }

  private suspend fun assertCases(
    filter: String,
    vararg properties: String,
    expectedIndices: Set<Int> = properties.indices.toSet(),
  ) {
    assertFilterSelects(filter, properties.toList(), expectedIndices)
  }
}
