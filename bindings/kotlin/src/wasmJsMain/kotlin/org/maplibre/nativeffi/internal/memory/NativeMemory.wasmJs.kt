@file:OptIn(ExperimentalWasmJsInterop::class, UnsafeWasmMemoryApi::class)

package org.maplibre.nativeffi.internal.memory

import kotlin.wasm.unsafe.UnsafeWasmMemoryApi
import kotlin.wasm.unsafe.withScopedMemoryAllocator
import org.maplibre.nativeffi.internal.c.unsignedWord
import org.maplibre.nativeffi.internal.c.word

/**
 * The browser module's heap, which is separate from Kotlin's own memory.
 *
 * Each scalar is one call into a `DataView` over the heap. A byte array crosses in one JavaScript
 * copy, staged through Kotlin's memory.
 */
internal actual object NativeMemory {
  actual val addressSize: Int = 4

  actual val scalar64Alignment: Int = 8

  actual fun allocate(size: Long): Long = allocateUninitialized(size).also { clear(it, size) }

  actual fun allocateUninitialized(size: Long): Long =
    unsignedWord(mln_kw_allocate(word(size))).also {
      if (it == 0L) throw IllegalStateException("cannot allocate $size bytes of native memory")
    }

  actual fun free(address: Long) = mln_kw_free(word(address))

  actual fun clear(address: Long, size: Long) {
    if (size > 0) fill(word(address), word(address + size))
  }

  actual fun getByte(address: Long): Byte = getInt8(word(address)).toByte()

  actual fun putByte(address: Long, value: Byte) = setInt8(word(address), value.toInt())

  actual fun getShort(address: Long): Short = getInt16(word(address)).toShort()

  actual fun putShort(address: Long, value: Short) = setInt16(word(address), value.toInt())

  actual fun getInt(address: Long): Int = getInt32(word(address))

  actual fun putInt(address: Long, value: Int) = setInt32(word(address), value)

  actual fun getLong(address: Long): Long = getBigInt64(word(address))

  actual fun putLong(address: Long, value: Long) = setBigInt64(word(address), value)

  actual fun getFloat(address: Long): Float = getFloat32(word(address))

  actual fun putFloat(address: Long, value: Float) = setFloat32(word(address), value)

  actual fun getDouble(address: Long): Double = getFloat64(word(address))

  actual fun putDouble(address: Long, value: Double) = setFloat64(word(address), value)

  actual fun getBytes(address: Long, count: Int): ByteArray {
    val bytes = ByteArray(count)
    if (count == 0) return bytes
    val source = word(address)
    withScopedMemoryAllocator { allocator ->
      val staging = allocator.allocate(count)
      copyFromHeap(kotlin.wasm.unsafe.wasmMemory, source, staging.address.toInt(), count)
      for (index in 0 until count) bytes[index] = (staging + index).loadByte()
    }
    return bytes
  }

  actual fun putBytes(address: Long, value: ByteArray) {
    if (value.isEmpty()) return
    val target = word(address)
    withScopedMemoryAllocator { allocator ->
      val staging = allocator.allocate(value.size)
      for (index in value.indices) (staging + index).storeByte(value[index])
      copyToHeap(kotlin.wasm.unsafe.wasmMemory, staging.address.toInt(), target, value.size)
    }
  }

  actual fun putUtf8(address: Long, value: String): Long = putUtf8(address, value, 0xfffd)

  actual fun stringLength(address: Long): Long {
    val start = word(address)
    return (indexOfNul(start) - start).toLong()
  }
}

private fun mln_kw_allocate(size: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_allocate(size)")

private fun mln_kw_free(address: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_free(address)")

private fun fill(start: Int, end: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].HEAPU8.fill(0, start, end)")

private fun indexOfNul(start: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].HEAPU8.indexOf(0, start)")

private fun getInt8(address: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.getInt8(address)")

private fun setInt8(address: Int, value: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.setInt8(address, value)")

private fun getInt16(address: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.getInt16(address, true)")

private fun setInt16(address: Int, value: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.setInt16(address, value, true)")

private fun getInt32(address: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.getInt32(address, true)")

private fun setInt32(address: Int, value: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.setInt32(address, value, true)")

private fun getBigInt64(address: Int): Long =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.getBigInt64(address, true)")

private fun setBigInt64(address: Int, value: Long): Unit =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.setBigInt64(address, value, true)",
  )

private fun getFloat32(address: Int): Float =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.getFloat32(address, true)")

private fun setFloat32(address: Int, value: Float): Unit =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.setFloat32(address, value, true)",
  )

private fun getFloat64(address: Int): Double =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.getFloat64(address, true)")

private fun setFloat64(address: Int, value: Double): Unit =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwView.setFloat64(address, value, true)",
  )

private fun copyFromHeap(memory: JsAny, source: Int, target: Int, count: Int): Unit =
  js(
    "new Uint8Array(memory.buffer, target, count).set(globalThis[Symbol.for('org.maplibre.nativeffi.c')].HEAPU8.subarray(source, source + count))",
  )

private fun copyToHeap(memory: JsAny, source: Int, target: Int, count: Int): Unit =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')].HEAPU8.set(new Uint8Array(memory.buffer, source, count), target)",
  )
