package org.maplibre.nativeffi.render

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.query.QueriedFeature

internal const val QUERY_STYLE_JSON =
  """
      {
        "version": 8,
        "name": "kotlin-query-test",
        "sources": {
          "point": {
            "type": "geojson",
            "data": {
              "type": "FeatureCollection",
              "features": [
                {
                  "type": "Feature",
                  "id": "feature-1",
                  "geometry": {"type": "Point", "coordinates": [-122.4194, 37.7749]},
                  "properties": {"kind": "capital", "visible": true}
                }
              ]
            }
          }
        },
        "layers": [
          {"id": "background", "type": "background", "paint": {"background-color": "#d8f1ff"}},
          {"id": "point-circle", "type": "circle", "source": "point", "paint": {"circle-color": "#f97316", "circle-radius": 12}}
        ]
      }
      """

internal fun jsonBytes(value: String): ByteArray = value.trimIndent().encodeToByteArray()

internal suspend fun waitForQueriedFeature(
  session: RenderSessionHandle,
  attempts: Int = 500,
  query: () -> Deferred<List<QueriedFeature>>,
): QueriedFeature {
  repeat(attempts) {
    session.renderOneFrame()
    val feature = session.completeOnDriver(query()).firstOrNull()
    if (feature != null) return feature
  }
  error("query returned no features")
}

internal fun featureStringProperty(feature: ByteArray, key: String): String? =
  rawMember(feature, "properties")?.let { stringMember(it, key) }

internal fun firstFeature(collection: ByteArray): ByteArray? =
  rawMember(collection, "features")?.let(::firstArrayElement)

internal fun numberMember(value: ByteArray, key: String): Double? =
  rawMember(value, key)?.decodeToString()?.toDoubleOrNull()

internal fun stringMember(value: ByteArray, key: String): String? {
  val encoded = rawMember(value, key)?.decodeToString() ?: return null
  if (encoded.length < 2 || encoded.first() != '"' || encoded.last() != '"') return null
  return encoded.substring(1, encoded.lastIndex)
}

/** Extracts a top-level object member without introducing a JSON model into transit tests. */
internal fun rawMember(value: ByteArray, key: String): ByteArray? {
  val json = value.decodeToString()
  var cursor = skipWhitespace(json, 0)
  if (cursor >= json.length || json[cursor] != '{') return null
  cursor++
  while (true) {
    cursor = skipWhitespace(json, cursor)
    if (cursor >= json.length || json[cursor] == '}') return null
    if (json[cursor] != '"') return null
    val keyEnd = jsonStringEnd(json, cursor)
    val memberName = json.substring(cursor + 1, keyEnd - 1)
    cursor = skipWhitespace(json, keyEnd)
    if (cursor >= json.length || json[cursor] != ':') return null
    val valueStart = skipWhitespace(json, cursor + 1)
    val valueEnd = jsonValueEnd(json, valueStart)
    if (memberName == key) return json.substring(valueStart, valueEnd).encodeToByteArray()
    cursor = skipWhitespace(json, valueEnd)
    if (cursor >= json.length || json[cursor] != ',') return null
    cursor++
  }
}

private fun firstArrayElement(value: ByteArray): ByteArray? {
  val json = value.decodeToString()
  var cursor = skipWhitespace(json, 0)
  if (cursor >= json.length || json[cursor] != '[') return null
  cursor = skipWhitespace(json, cursor + 1)
  if (cursor >= json.length || json[cursor] == ']') return null
  return json.substring(cursor, jsonValueEnd(json, cursor)).encodeToByteArray()
}

private fun skipWhitespace(json: String, start: Int): Int {
  var cursor = start
  while (cursor < json.length && json[cursor].isWhitespace()) cursor++
  return cursor
}

private fun jsonStringEnd(json: String, start: Int): Int {
  var escaped = false
  for (cursor in start + 1 until json.length) {
    val character = json[cursor]
    if (escaped) {
      escaped = false
    } else if (character == '\\') {
      escaped = true
    } else if (character == '"') {
      return cursor + 1
    }
  }
  error("unterminated JSON string")
}

private fun jsonValueEnd(json: String, start: Int): Int {
  if (json[start] == '"') return jsonStringEnd(json, start)
  if (json[start] != '{' && json[start] != '[') {
    var cursor = start
    while (
      cursor < json.length &&
        !json[cursor].isWhitespace() &&
        json[cursor] != ',' &&
        json[cursor] != '}' &&
        json[cursor] != ']'
    ) {
      cursor++
    }
    return cursor
  }

  var depth = 0
  var cursor = start
  while (cursor < json.length) {
    when (json[cursor]) {
      '"' -> cursor = jsonStringEnd(json, cursor) - 1
      '{',
      '[' -> depth++
      '}',
      ']' -> {
        depth--
        if (depth == 0) return cursor + 1
      }
    }
    cursor++
  }
  error("unterminated JSON value")
}
