package org.maplibre.nativeffi.internal.memory

internal fun putUtf8(address: Long, value: String, replacement: Int): Long {
  var output = address
  var index = 0
  while (index < value.length) {
    var code = value[index++].code
    if (code in 0xd800..0xdbff && index < value.length && value[index].code in 0xdc00..0xdfff) {
      code = 0x10000 + ((code - 0xd800) shl 10) + value[index++].code - 0xdc00
    } else if (code in 0xd800..0xdfff) {
      code = replacement
    }
    if (code < 0x80) {
      NativeMemory.putByte(output++, code.toByte())
    } else if (code < 0x800) {
      NativeMemory.putByte(output++, (0xc0 or (code shr 6)).toByte())
      NativeMemory.putByte(output++, (0x80 or (code and 0x3f)).toByte())
    } else if (code < 0x10000) {
      NativeMemory.putByte(output++, (0xe0 or (code shr 12)).toByte())
      NativeMemory.putByte(output++, (0x80 or ((code shr 6) and 0x3f)).toByte())
      NativeMemory.putByte(output++, (0x80 or (code and 0x3f)).toByte())
    } else {
      NativeMemory.putByte(output++, (0xf0 or (code shr 18)).toByte())
      NativeMemory.putByte(output++, (0x80 or ((code shr 12) and 0x3f)).toByte())
      NativeMemory.putByte(output++, (0x80 or ((code shr 6) and 0x3f)).toByte())
      NativeMemory.putByte(output++, (0x80 or (code and 0x3f)).toByte())
    }
  }
  return output - address
}
