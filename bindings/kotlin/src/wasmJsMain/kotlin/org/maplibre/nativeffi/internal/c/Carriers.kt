package org.maplibre.nativeffi.internal.c

import org.maplibre.nativeffi.internal.status.invalidArgument

// Common code carries pointers and sizes as Long, and wasm32 passes them as 32-bit words. Memory.kt
// reads an address zero-extended, so a word converts back the same way.

/** [value], a pointer or `size_t`, as the unsigned 32-bit word that wasm32 passes. */
internal fun word(value: Long): Int {
  if (value !in 0L..UInt.MAX_VALUE.toLong()) {
    throw invalidArgument("$value does not fit a 32-bit address or size")
  }
  return value.toInt()
}

/** [value], a `ptrdiff_t`, as the signed 32-bit word that wasm32 passes. */
internal fun signedWord(value: Long): Int {
  if (value !in Int.MIN_VALUE.toLong()..Int.MAX_VALUE.toLong()) {
    throw invalidArgument("$value does not fit a 32-bit signed size")
  }
  return value.toInt()
}

/** A pointer or `size_t` that wasm32 returned. */
internal fun unsignedWord(word: Int): Long = word.toUInt().toLong()
