package org.maplibre.nativeffi.internal.memory

/**
 * Raw access to native memory at an address, which each platform implements over its own FFI.
 *
 * Generated code reads and writes C records through the typed accessors in `Memory.kt`, at offsets
 * the generator computed for the target's data model. Every address comes from native or from a
 * [NativeArena], so these functions trust it.
 */
internal expect object NativeMemory {
  /** The byte size of a C pointer and of `size_t`: 4 on Android ARM32 and 8 elsewhere. */
  val addressSize: Int

  /** Allocates [size] zeroed bytes aligned for any C type, throwing when native memory runs out. */
  fun allocate(size: Long): Long

  fun free(address: Long)

  fun getByte(address: Long): Byte

  fun putByte(address: Long, value: Byte)

  fun getShort(address: Long): Short

  fun putShort(address: Long, value: Short)

  fun getInt(address: Long): Int

  fun putInt(address: Long, value: Int)

  fun getLong(address: Long): Long

  fun putLong(address: Long, value: Long)

  fun getFloat(address: Long): Float

  fun putFloat(address: Long, value: Float)

  fun getDouble(address: Long): Double

  fun putDouble(address: Long, value: Double)

  /** Copies [count] bytes starting at [address]. */
  fun getBytes(address: Long, count: Int): ByteArray

  fun putBytes(address: Long, value: ByteArray)

  /** The length of the NUL-terminated string at [address], without its terminator. */
  fun stringLength(address: Long): Long
}
