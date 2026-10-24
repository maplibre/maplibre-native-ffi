package org.maplibre.nativeffi.internal.values

/** Names the [flags] set in [raw], joined by `|`, with any other bits in hex. */
internal fun flagLabel(raw: ULong, empty: String, flags: List<Pair<ULong, String>>): String {
  if (raw == 0uL) return empty
  var rest = raw
  val names = mutableListOf<String>()
  for ((bit, label) in flags) {
    if (raw and bit == bit) {
      names += label
      rest = rest and bit.inv()
    }
  }
  if (rest != 0uL) names += "0x" + rest.toString(16)
  return names.joinToString(" | ")
}
