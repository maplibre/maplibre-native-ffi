package org.maplibre.nativeffi.internal.memory

/**
 * Typed reads and writes of C record fields, which generated codecs call at generated offsets.
 *
 * `size_t` and pointers take [NativeMemory.addressSize] bytes. A C `bool` is one byte.
 */

/** Picks a field offset or size by data model: [narrow] for ILP32 and [wide] for LP64. */
internal fun w(narrow: Int, wide: Int): Int = if (NativeMemory.addressSize == 8) wide else narrow

internal fun readBool(address: Long): Boolean = NativeMemory.getByte(address) != 0.toByte()

internal fun writeBool(address: Long, value: Boolean) {
  NativeMemory.putByte(address, if (value) 1 else 0)
}

internal fun readI8(address: Long): Byte = NativeMemory.getByte(address)

internal fun writeI8(address: Long, value: Byte) = NativeMemory.putByte(address, value)

internal fun readU8(address: Long): UByte = NativeMemory.getByte(address).toUByte()

internal fun writeU8(address: Long, value: UByte) = NativeMemory.putByte(address, value.toByte())

internal fun readI16(address: Long): Short = NativeMemory.getShort(address)

internal fun writeI16(address: Long, value: Short) = NativeMemory.putShort(address, value)

internal fun readU16(address: Long): UShort = NativeMemory.getShort(address).toUShort()

internal fun writeU16(address: Long, value: UShort) =
  NativeMemory.putShort(address, value.toShort())

internal fun readI32(address: Long): Int = NativeMemory.getInt(address)

internal fun writeI32(address: Long, value: Int) = NativeMemory.putInt(address, value)

internal fun readU32(address: Long): UInt = NativeMemory.getInt(address).toUInt()

internal fun writeU32(address: Long, value: UInt) = NativeMemory.putInt(address, value.toInt())

internal fun readI64(address: Long): Long = NativeMemory.getLong(address)

internal fun writeI64(address: Long, value: Long) = NativeMemory.putLong(address, value)

internal fun readU64(address: Long): ULong = NativeMemory.getLong(address).toULong()

internal fun writeU64(address: Long, value: ULong) = NativeMemory.putLong(address, value.toLong())

internal fun readF32(address: Long): Float = NativeMemory.getFloat(address)

internal fun writeF32(address: Long, value: Float) = NativeMemory.putFloat(address, value)

internal fun readF64(address: Long): Double = NativeMemory.getDouble(address)

internal fun writeF64(address: Long, value: Double) = NativeMemory.putDouble(address, value)

/** Reads a pointer, zero-extended from 32 bits on ILP32. */
internal fun readAddress(address: Long): Long =
  if (NativeMemory.addressSize == 8) NativeMemory.getLong(address)
  else NativeMemory.getInt(address).toLong() and 0xffff_ffffL

internal fun writeAddress(address: Long, value: Long) {
  if (NativeMemory.addressSize == 8) NativeMemory.putLong(address, value)
  else NativeMemory.putInt(address, value.toInt())
}

/** Reads a `size_t`. */
internal fun readSize(address: Long): ULong = readAddress(address).toULong()

internal fun writeSize(address: Long, value: ULong) = writeAddress(address, value.toLong())

/** Narrows a native count to a list index, refusing one a Kotlin collection cannot hold. */
internal fun count(value: ULong): Int {
  require(value <= Int.MAX_VALUE.toULong()) { "native count $value exceeds Int.MAX_VALUE" }
  return value.toInt()
}

internal fun count(value: Long): Int = count(value.toULong())

/** Copies [count] bytes from [pointer], which may be null only when [count] is zero. */
internal fun readBytes(pointer: Long, count: ULong): ByteArray {
  val size = count(count)
  if (size == 0) return byteArrayOf()
  require(pointer != 0L) { "native data pointer is null" }
  return NativeMemory.getBytes(pointer, size)
}

/** Copies a NUL-terminated UTF-8 string. */
internal fun readCString(pointer: Long): String {
  require(pointer != 0L) { "native string pointer is null" }
  return NativeMemory.getBytes(pointer, count(NativeMemory.stringLength(pointer))).decodeToString()
}

internal fun readCStringOrNull(pointer: Long): String? =
  if (pointer == 0L) null else readCString(pointer)

/** The bytes of the `mln_buffer_view` at [address]. */
internal fun readView(address: Long): ByteArray =
  readBytes(readAddress(address), readSize(address + NativeMemory.addressSize))

/** The bytes of the `mln_buffer_view` at [address], or null when its data pointer is null. */
internal fun readViewOrNull(address: Long): ByteArray? =
  if (readAddress(address) == 0L) null else readView(address)

internal fun readViewString(address: Long): String = readView(address).decodeToString()

internal fun readViewStringOrNull(address: Long): String? =
  readViewOrNull(address)?.decodeToString()

/**
 * Copies the [length]-byte window at [offset] of the [size]-byte item arena at [data], refusing a
 * window that reaches outside the arena.
 */
internal fun readItem(data: Long, size: ULong, offset: ULong, length: ULong): ByteArray {
  require(offset <= size && length <= size - offset) { "native item lies outside its arena" }
  return readBytes(data + offset.toLong(), length)
}

/** Copies [count] elements of [stride] bytes each, starting at [pointer]. */
internal inline fun <T> readArray(
  pointer: Long,
  count: ULong,
  stride: Long,
  read: (Long) -> T,
): List<T> {
  val size = count(count)
  if (size == 0) return emptyList()
  require(pointer != 0L) { "native array pointer is null" }
  return List(size) { index -> read(pointer + index * stride) }
}

/** Copies a strided array, whose native stride may exceed the element size this binding knows. */
internal inline fun <T> readStrided(
  pointer: Long,
  count: ULong,
  stride: ULong,
  size: Int,
  read: (Long) -> T,
): List<T> {
  require(stride >= size.toULong()) { "native stride $stride is below the element size $size" }
  return readArray(pointer, count, stride.toLong(), read)
}
