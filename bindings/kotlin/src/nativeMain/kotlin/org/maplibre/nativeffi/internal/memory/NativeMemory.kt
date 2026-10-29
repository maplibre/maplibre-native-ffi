@file:OptIn(ExperimentalForeignApi::class)

package org.maplibre.nativeffi.internal.memory

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointerVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CVariable
import kotlinx.cinterop.DoubleVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.FloatVar
import kotlinx.cinterop.IntVar
import kotlinx.cinterop.LongVar
import kotlinx.cinterop.ShortVar
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.alignOf
import kotlinx.cinterop.convert
import kotlinx.cinterop.pointed
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toCPointer
import kotlinx.cinterop.toLong
import kotlinx.cinterop.usePinned
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.cinterop.mln_lat_lng
import platform.posix.calloc
import platform.posix.malloc
import platform.posix.memcpy
import platform.posix.memset
import platform.posix.strlen

/** Native memory through C pointers. */
internal actual object NativeMemory {
  actual val addressSize: Int = sizeOf<COpaquePointerVar>().toInt()

  // CStructVar preserves the C ABI alignment; Kotlin LongVar always reports eight.
  actual val scalar64Alignment: Int = alignOf<mln_lat_lng>()

  actual fun allocate(size: Long): Long =
    calloc(1.convert(), maxOf(size, 1L).convert()).toLong().also {
      check(it != 0L) { "cannot allocate $size bytes of native memory" }
    }

  actual fun free(address: Long) = platform.posix.free(address.toCPointer<ByteVar>())

  actual fun clear(address: Long, size: Long) {
    memset(address.toCPointer<ByteVar>(), 0, size.convert())
  }

  actual fun allocateUninitialized(size: Long): Long =
    malloc(maxOf(size, 1L).convert()).toLong().also {
      if (it == 0L) throw OutOfMemoryError("cannot allocate $size bytes of native memory")
    }

  private inline fun <reified T : CVariable> at(address: Long): CPointer<T> =
    requireNotNull(address.toCPointer<T>())

  actual fun getByte(address: Long): Byte = at<ByteVar>(address).pointed.value

  actual fun putByte(address: Long, value: Byte) {
    at<ByteVar>(address).pointed.value = value
  }

  actual fun getShort(address: Long): Short = at<ShortVar>(address).pointed.value

  actual fun putShort(address: Long, value: Short) {
    at<ShortVar>(address).pointed.value = value
  }

  actual fun getInt(address: Long): Int = at<IntVar>(address).pointed.value

  actual fun putInt(address: Long, value: Int) {
    at<IntVar>(address).pointed.value = value
  }

  actual fun getLong(address: Long): Long = at<LongVar>(address).pointed.value

  actual fun putLong(address: Long, value: Long) {
    at<LongVar>(address).pointed.value = value
  }

  actual fun getFloat(address: Long): Float = at<FloatVar>(address).pointed.value

  actual fun putFloat(address: Long, value: Float) {
    at<FloatVar>(address).pointed.value = value
  }

  actual fun getDouble(address: Long): Double = at<DoubleVar>(address).pointed.value

  actual fun putDouble(address: Long, value: Double) {
    at<DoubleVar>(address).pointed.value = value
  }

  actual fun getBytes(address: Long, count: Int): ByteArray = at<ByteVar>(address).readBytes(count)

  actual fun putBytes(address: Long, value: ByteArray) {
    if (value.isEmpty()) return
    value.usePinned { memcpy(at<ByteVar>(address), it.addressOf(0), value.size.convert()) }
  }

  actual fun putUtf8(address: Long, value: String): Long = putUtf8(address, value, 0xfffd)

  actual fun stringLength(address: Long): Long = strlen(at<ByteVar>(address)).toLong()
}
