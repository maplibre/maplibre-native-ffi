package org.maplibre.nativeffi.internal.memory

import java.lang.foreign.FunctionDescriptor
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout.ADDRESS
import java.lang.foreign.ValueLayout.JAVA_BYTE
import java.lang.foreign.ValueLayout.JAVA_DOUBLE_UNALIGNED
import java.lang.foreign.ValueLayout.JAVA_FLOAT_UNALIGNED
import java.lang.foreign.ValueLayout.JAVA_INT_UNALIGNED
import java.lang.foreign.ValueLayout.JAVA_LONG
import java.lang.foreign.ValueLayout.JAVA_LONG_UNALIGNED
import java.lang.foreign.ValueLayout.JAVA_SHORT_UNALIGNED

/** Native memory through one FFM segment that spans the address space. */
internal actual object NativeMemory {
  private val memory = MemorySegment.NULL.reinterpret(Long.MAX_VALUE)
  private val linker = Linker.nativeLinker()
  private val calloc =
    linker.downcallHandle(
      linker.defaultLookup().findOrThrow("calloc"),
      FunctionDescriptor.of(JAVA_LONG, JAVA_LONG, JAVA_LONG),
    )
  private val freeMemory =
    linker.downcallHandle(
      linker.defaultLookup().findOrThrow("free"),
      FunctionDescriptor.ofVoid(JAVA_LONG),
    )
  private val strlen =
    linker.downcallHandle(
      linker.defaultLookup().findOrThrow("strlen"),
      FunctionDescriptor.of(JAVA_LONG, JAVA_LONG),
    )

  actual val addressSize: Int = ADDRESS.byteSize().toInt()

  actual fun allocate(size: Long): Long {
    val address = calloc.invokeExact(1L, maxOf(size, 1L)) as Long
    if (address == 0L) throw OutOfMemoryError("cannot allocate $size bytes of native memory")
    return address
  }

  actual fun free(address: Long) {
    freeMemory.invoke(address)
  }

  actual fun getByte(address: Long): Byte = memory.get(JAVA_BYTE, address)

  actual fun putByte(address: Long, value: Byte) = memory.set(JAVA_BYTE, address, value)

  actual fun getShort(address: Long): Short = memory.get(JAVA_SHORT_UNALIGNED, address)

  actual fun putShort(address: Long, value: Short) =
    memory.set(JAVA_SHORT_UNALIGNED, address, value)

  actual fun getInt(address: Long): Int = memory.get(JAVA_INT_UNALIGNED, address)

  actual fun putInt(address: Long, value: Int) = memory.set(JAVA_INT_UNALIGNED, address, value)

  actual fun getLong(address: Long): Long = memory.get(JAVA_LONG_UNALIGNED, address)

  actual fun putLong(address: Long, value: Long) = memory.set(JAVA_LONG_UNALIGNED, address, value)

  actual fun getFloat(address: Long): Float = memory.get(JAVA_FLOAT_UNALIGNED, address)

  actual fun putFloat(address: Long, value: Float) =
    memory.set(JAVA_FLOAT_UNALIGNED, address, value)

  actual fun getDouble(address: Long): Double = memory.get(JAVA_DOUBLE_UNALIGNED, address)

  actual fun putDouble(address: Long, value: Double) =
    memory.set(JAVA_DOUBLE_UNALIGNED, address, value)

  actual fun getBytes(address: Long, count: Int): ByteArray =
    ByteArray(count).also { MemorySegment.copy(memory, JAVA_BYTE, address, it, 0, count) }

  actual fun putBytes(address: Long, value: ByteArray) =
    MemorySegment.copy(value, 0, memory, JAVA_BYTE, address, value.size)

  actual fun stringLength(address: Long): Long = strlen.invokeExact(address) as Long
}
